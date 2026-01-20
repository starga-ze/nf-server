#include "TcpServer.h"
#include "util/Logger.h"
#include "ingress/RxRouter.h"
#include "util/ThreadManager.h"
#include "net/packet/Packet.h"
#include "net/tls/TlsServer.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <arpa/inet.h>

#ifndef TCP_MAX_EVENTS
#define TCP_MAX_EVENTS 64
#endif

#ifndef TCP_RECV_BUFFER_SIZE
#define TCP_RECV_BUFFER_SIZE 8192
#endif

TcpServer::TcpServer(int port, RxRouter *rxRouter, int workerCount, ThreadManager *threadManager,
                     std::shared_ptr <TlsServer> tlsServer)
        : m_port(port),
          m_rxRouter(rxRouter),
          m_workerCount(workerCount),
          m_threadManager(threadManager),
          m_tlsServer(std::move(tlsServer)) {
    init();
}

TcpServer::~TcpServer() {
    deinit();
}

bool TcpServer::init() {
    m_sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockFd < 0)
        return false;

    int opt = 1;
    setsockopt(m_sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    setNonBlocking(m_sockFd);

    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;
    m_serverAddr.sin_port = htons(m_port);

    if (bind(m_sockFd, (sockaddr * ) & m_serverAddr, sizeof(m_serverAddr)) != 0)
        return false;

    if (listen(m_sockFd, SOMAXCONN) != 0)
        return false;

    m_epFd = epoll_create1(0);
    if (m_epFd < 0)
        return false;

    m_txEventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_txEventFd < 0)
        return false;

    addToEpoll(m_sockFd, EPOLLIN);
    addToEpoll(m_txEventFd, EPOLLIN);

    return true;
}

void TcpServer::deinit() {
    for (auto &kv: m_clients)
        close(kv.first);
    m_clients.clear();

    if (m_sockFd >= 0) close(m_sockFd);
    if (m_epFd >= 0) close(m_epFd);
    if (m_txEventFd >= 0) close(m_txEventFd);
}

void TcpServer::start() {
    m_running = true;
    startWorkers();

    epoll_event events[TCP_MAX_EVENTS];
    m_buffer.resize(TCP_RECV_BUFFER_SIZE);

    while (m_running) {
        int n = epoll_wait(m_epFd, events, TCP_MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < n; ++i)
            handleEvent(events[i]);
    }
}

void TcpServer::stopReact() {
    m_running = false;
    m_cv.notify_all();

    uint64_t v = 1;
    write(m_txEventFd, &v, sizeof(v));
}

void TcpServer::startWorkers() {
    for (int i = 0; i < m_workerCount; ++i) {
        m_threadManager->addThread(
                "tcp_worker_" + std::to_string(i),
                std::bind(&TcpServer::processPacket, this),
                std::bind(&TcpServer::stopWorker, this));
    }
}

void TcpServer::stopWorker() {
    m_cv.notify_all();
}

void TcpServer::processPacket() {
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

void TcpServer::handleEvent(const epoll_event &ev) {
    int fd = ev.data.fd;

    if (fd == m_txEventFd) {
        drainEventFd(m_txEventFd);
        flushAllPending(256);
        return;
    }

    if (ev.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
        closeConnection(fd);
        return;
    }

    if (fd == m_sockFd) {
        acceptConnection();
        return;
    }

    if (ev.events & EPOLLOUT)
        flushPendingForFd(fd, 256);

    if (ev.events & EPOLLIN)
        receivePacket(fd);
}

void TcpServer::acceptConnection() {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int fd = accept(m_sockFd, (sockaddr * ) & clientAddr, &len);

        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            continue;
        }

        setNonBlocking(fd);
        addToEpoll(fd, EPOLLIN | EPOLLRDHUP);
        m_clients.emplace(fd, clientAddr);
    }
}

void TcpServer::receivePacket(int fd) {
    if (isTlsClientHello(fd) && handoverToTls(fd))
        return;

    while (true) {
        ssize_t n = recv(fd, m_buffer.data(), m_buffer.size(), 0);
        if (n > 0) {
            auto it = m_clients.find(fd);
            if (it == m_clients.end())
                return;

            std::vector <uint8_t> payload(m_buffer.begin(), m_buffer.begin() + n);
            auto pkt = std::make_unique<Packet>(
                    fd, Protocol::TCP, std::move(payload), it->second, m_serverAddr);

            {
                std::lock_guard <std::mutex> lock(m_rxLock);
                m_rxQueue.push(std::move(pkt));
            }
            m_cv.notify_one();
        } else if (n == 0) {
            closeConnection(fd);
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            closeConnection(fd);
            return;
        }
    }
}

void TcpServer::enqueueTx(std::unique_ptr <Packet> packet) {
    if (!packet) return;

    int fd = packet->getFd();
    {
        std::lock_guard <std::mutex> lock(m_txLock);
        m_txQueue[fd].push_back(std::move(packet));
    }

    uint64_t v = 1;
    write(m_txEventFd, &v, sizeof(v));
}

void TcpServer::flushAllPending(size_t budget) {
    std::vector<int> fds;
    {
        std::lock_guard <std::mutex> lock(m_txLock);
        for (auto &kv: m_txQueue)
            if (!kv.second.empty())
                fds.push_back(kv.first);
    }

    size_t used = 0;
    for (int fd: fds) {
        if (used >= budget) break;
        used += flushPendingForFd(fd, budget - used);
    }
}

size_t TcpServer::flushPendingForFd(int fd, size_t budget) {
    size_t used = 0;

    while (used < budget) {
        std::unique_ptr <Packet> pkt;
        {
            std::lock_guard <std::mutex> lock(m_txLock);
            auto it = m_txQueue.find(fd);
            if (it == m_txQueue.end() || it->second.empty()) {
                setInterest(fd, false);
                return used;
            }
            pkt = std::move(it->second.front());
            it->second.pop_front();
        }

        const auto &payload = pkt->getPayload();
        size_t offset = pkt->getTxOffset();

        const uint8_t *p = payload.data() + offset;
        size_t bytes = payload.size() - offset;

        while (bytes > 0) {
            ssize_t ret = send(fd, p, bytes, MSG_NOSIGNAL);
            if (ret > 0) {
                pkt->updateTxOffset(ret);
                p += ret;
                bytes -= ret;
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::lock_guard <std::mutex> lock(m_txLock);
                m_txQueue[fd].push_front(std::move(pkt));
                setInterest(fd, true);
                return used + 1;
            }

            closeConnection(fd);
            return used + 1;
        }

        used++;
    }

    setInterest(fd, hasPendingTx(fd));
    return used;
}

bool TcpServer::hasPendingTx(int fd) {
    std::lock_guard <std::mutex> lock(m_txLock);
    auto it = m_txQueue.find(fd);
    return it != m_txQueue.end() && !it->second.empty();
}

void TcpServer::closeConnection(int fd) {
    epoll_ctl(m_epFd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    m_clients.erase(fd);

    std::lock_guard <std::mutex> lock(m_txLock);
    m_txQueue.erase(fd);
}

bool TcpServer::isTlsClientHello(int fd) {
    uint8_t buf[5];
    ssize_t n = recv(fd, buf, sizeof(buf), MSG_PEEK);
    if (n < 5) return false;
    return buf[0] == 0x16 && buf[1] == 0x03;
}

bool TcpServer::handoverToTls(int fd) {
    epoll_ctl(m_epFd, EPOLL_CTL_DEL, fd, nullptr);

    auto it = m_clients.find(fd);
    if (it == m_clients.end())
        return false;

    sockaddr_in clientAddr = it->second;
    m_clients.erase(it);

    if (m_tlsServer) {
        m_tlsServer->handleTlsConnection(fd, {m_serverAddr, clientAddr});
        return true;
    }

    close(fd);
    return false;
}

bool TcpServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

void TcpServer::drainEventFd(int efd) {
    uint64_t v;
    while (read(efd, &v, sizeof(v)) > 0) {}
}

bool TcpServer::addToEpoll(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    return epoll_ctl(m_epFd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool TcpServer::modEpoll(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    return epoll_ctl(m_epFd, EPOLL_CTL_MOD, fd, &ev) == 0;
}

void TcpServer::setInterest(int fd, bool wantOut) {
    uint32_t ev = EPOLLIN | EPOLLRDHUP;
    if (wantOut) ev |= EPOLLOUT;
    modEpoll(fd, ev);
}

