#include "UdpServer.h"
#include "util/Logger.h"

#include "ingress/RxRouter.h"
#include "util/ThreadManager.h"
#include "net/packet/Packet.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <arpa/inet.h>

#define UDP_RECV_CHUNK_SIZE     (2048)
#define UDP_MAX_RX_BUFFER_SIZE  (256 * 1024) // 256 KB
#define UDP_MAX_EVENTS          (64)


UdpServer::UdpServer(int port,
                     RxRouter *rxRouter,
                     int workerCount,
                     ThreadManager *threadManager)
        : m_port(port),
          m_rxRouter(rxRouter),
          m_threadManager(threadManager),
          m_workerCount(workerCount) {
    init();
}

UdpServer::~UdpServer() {
    deinit();
}

bool UdpServer::init() {
    m_stopEventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_stopEventFd < 0) {
        LOG_ERROR("UdpServer: stop eventfd create failed");
        return false;
    }

    m_txEventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_txEventFd < 0) {
        LOG_ERROR("UdpServer: tx eventfd create failed");
        close(m_stopEventFd);
        m_stopEventFd = -1;
        return false;
    }

    m_sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_sockFd < 0) {
        LOG_ERROR("UdpServer: socket create failed errno={}", errno);
        return false;
    }

    int opt = 1;
    setsockopt(m_sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 수신 버퍼
    int rcv = UDP_MAX_RX_BUFFER_SIZE;
    setsockopt(m_sockFd, SOL_SOCKET, SO_RCVBUF, &rcv, sizeof(rcv));

    if (!setNonBlocking(m_sockFd)) {
        LOG_ERROR("UdpServer: setNonBlocking failed errno={}", errno);
        return false;
    }

    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;
    m_serverAddr.sin_port = htons(m_port);

    if (bind(m_sockFd, (sockaddr * ) & m_serverAddr, sizeof(m_serverAddr)) != 0) {
        LOG_ERROR("UdpServer: bind failed errno={}", errno);
        return false;
    }

    m_epFd = epoll_create1(0);
    if (m_epFd < 0) {
        LOG_ERROR("UdpServer: epoll_create1 failed errno={}", errno);
        return false;
    }

    if (!addToEpoll(m_sockFd, EPOLLIN) ||
        !addToEpoll(m_stopEventFd, EPOLLIN) ||
        !addToEpoll(m_txEventFd, EPOLLIN)) {
        LOG_ERROR("UdpServer: epoll_ctl add failed errno={}", errno);
        return false;
    }

    return true;
}

void UdpServer::deinit() {
    {
        std::lock_guard <std::mutex> lock(m_txLock);
        m_txQueue.clear();
    }

    {
        std::lock_guard <std::mutex> lock(m_rxLock);
        while (!m_rxQueue.empty()) m_rxQueue.pop();
    }

    if (m_epFd >= 0) {
        close(m_epFd);
        m_epFd = -1;
    }
    if (m_sockFd >= 0) {
        close(m_sockFd);
        m_sockFd = -1;
    }
    if (m_stopEventFd >= 0) {
        close(m_stopEventFd);
        m_stopEventFd = -1;
    }
    if (m_txEventFd >= 0) {
        close(m_txEventFd);
        m_txEventFd = -1;
    }
}

void UdpServer::start() {
    m_running = true;
    startWorkers();

    epoll_event events[UDP_MAX_EVENTS];
    m_rxBuffer.resize(UDP_RECV_CHUNK_SIZE);

    while (m_running) {
        int n = epoll_wait(m_epFd, events, UDP_MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("UdpServer: epoll_wait failed errno={}", errno);
            break;
        }

        for (int i = 0; i < n; ++i) {
            handleEvent(events[i]);
        }
    }
}

void UdpServer::stopReact() {
    m_running = false;
    m_cv.notify_all();

    if (m_stopEventFd >= 0) {
        uint64_t v = 1;
        (void) write(m_stopEventFd, &v, sizeof(v));
    }
}

void UdpServer::startWorkers() {
    for (int i = 0; i < m_workerCount; ++i) {
        m_threadManager->addThread(
                "udp_worker_" + std::to_string(i),
                std::bind(&UdpServer::processPacket, this),
                std::bind(&UdpServer::stopWorker, this));
    }
}

void UdpServer::stopWorker() {
    m_cv.notify_all();
}

void UdpServer::processPacket() {
    while (true) {
        std::unique_ptr <Packet> pkt;
        {
            std::unique_lock <std::mutex> lock(m_rxLock);

            while (m_rxQueue.empty() && m_running)
                m_cv.wait(lock);

            if (!m_running && m_rxQueue.empty())
                break;

            pkt = std::move(m_rxQueue.front());
            m_rxQueue.pop();
        }

        m_rxRouter->handlePacket(std::move(pkt));
    }
}

void UdpServer::handleEvent(const epoll_event &ev) {
    const int fd = ev.data.fd;

    if (fd == m_stopEventFd) {
        handleStopEvent();
        return;
    }

    if (fd == m_txEventFd) {
        handleTxEvent();
        return;
    }

    if (fd == m_sockFd) {
        if (ev.events & (EPOLLERR | EPOLLHUP)) {
            LOG_ERROR("UdpServer: socket error/hup");
            handleClose();
            return;
        }

        if (ev.events & EPOLLIN) {
            receivePacket();
            return;
        }
    }
}

void UdpServer::handleStopEvent() {
    drainEventFd(m_stopEventFd);
    m_running = false;
    m_cv.notify_all();
}

void UdpServer::handleTxEvent() {
    drainEventFd(m_txEventFd);
    flushAllPending(256);
}

void UdpServer::receivePacket() {
    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);

    while (true) {
        ssize_t bytes = recvfrom(
                m_sockFd,
                m_rxBuffer.data(),
                m_rxBuffer.size(),
                0,
                reinterpret_cast<sockaddr *>(&clientAddr),
                &addrLen);

        if (bytes > 0) {
            std::vector <uint8_t> payload(m_rxBuffer.begin(), m_rxBuffer.begin() + bytes);

            auto pkt = std::make_unique<Packet>(
                    m_sockFd,
                    Protocol::UDP,
                    std::move(payload),
                    clientAddr,
                    m_serverAddr);

            {
                std::lock_guard <std::mutex> lock(m_rxLock);
                m_rxQueue.push(std::move(pkt));
            }
            m_cv.notify_one();

            clientAddr = sockaddr_in{};
            addrLen = sizeof(clientAddr);
            continue;
        }

        if (bytes == 0) {
            return;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;

        if (errno == EINTR)
            continue;

        LOG_ERROR("UdpServer: recvfrom failed errno={}", errno);
        return;
    }
}

void UdpServer::enqueueTx(std::unique_ptr <Packet> packet) {
    if (!packet) return;

    {
        std::lock_guard <std::mutex> lock(m_txLock);
        m_txQueue.push_back(std::move(packet));
    }

    uint64_t v = 1;
    (void) write(m_txEventFd, &v, sizeof(v));
}

bool UdpServer::hasPendingTx() {
    std::lock_guard <std::mutex> lock(m_txLock);
    return !m_txQueue.empty();
}

void UdpServer::flushAllPending(size_t budgetItems) {
    (void) flushPending(budgetItems);
}

size_t UdpServer::flushPending(size_t budgetItems) {
    size_t used = 0;

    while (used < budgetItems) {
        std::unique_ptr <Packet> pkt;
        {
            std::lock_guard <std::mutex> lock(m_txLock);
            if (m_txQueue.empty())
                return used;

            pkt = std::move(m_txQueue.front());
            m_txQueue.pop_front();
        }

        const auto &payload = pkt->getPayload();

        sockaddr_in dstAddr{};
        dstAddr.sin_family = AF_INET;
        dstAddr.sin_addr.s_addr = htonl(pkt->getDstIp());
        dstAddr.sin_port = htons(pkt->getDstPort());

        ssize_t ret = sendto(
                m_sockFd,
                payload.data(),
                payload.size(),
                0,
                reinterpret_cast<const sockaddr *>(&dstAddr),
                sizeof(dstAddr));

        if (ret >= 0) {
            used++;
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            {
                std::lock_guard <std::mutex> lock(m_txLock);
                m_txQueue.push_front(std::move(pkt));
            }

            uint64_t v = 1;
            (void) write(m_txEventFd, &v, sizeof(v));
            return used + 1;
        }

        if (errno == EINTR) {
            {
                std::lock_guard <std::mutex> lock(m_txLock);
                m_txQueue.push_front(std::move(pkt));
            }
            continue;
        }

        LOG_ERROR("UdpServer: sendto failed errno={}", errno);
        used++;
    }

    if (hasPendingTx()) {
        uint64_t v = 1;
        (void) write(m_txEventFd, &v, sizeof(v));
    }

    return used;
}

void UdpServer::handleClose() {
    m_running = false;
    m_cv.notify_all();
}

bool UdpServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

void UdpServer::drainEventFd(int efd) {
    uint64_t v;
    while (true) {
        ssize_t n = read(efd, &v, sizeof(v));
        if (n == (ssize_t)sizeof(v)) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        break;
    }
}

bool UdpServer::addToEpoll(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    return epoll_ctl(m_epFd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

