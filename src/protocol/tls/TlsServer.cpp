#include "TlsServer.h"
#include "util/Logger.h"
#include "packet/Packet.h"
#include "ingress/RxRouter.h"
#include "util/ThreadManager.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>

#include <cstring>

#define TLS_HEADER_SIZE        (sizeof(CommonPacketHeader)) // 8 Byte
#define TLS_MAX_BODY_LEN       (64 * 1024) // 64 KB
#define TLS_RECV_CHUNK_SIZE    (4096)
#define TLS_MAX_RX_BUFFER_SIZE (TLS_HEADER_SIZE + TLS_MAX_BODY_LEN)
#define TLS_MAX_EVENTS         (64)

TlsServer::TlsServer(SSL_CTX* ctx,
                     RxRouter* rxRouter,
                     int workerCount,
                     ThreadManager* threadManager)
    : m_ctx(ctx),
      m_workerCount(workerCount),
      m_threadManager(threadManager),
      m_rxRouter(rxRouter) {
    init();
}

TlsServer::~TlsServer() {
    deinit();
}

bool TlsServer::init() {
    m_stopEventFd     = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    m_handoverEventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    m_txEventFd       = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    m_epFd            = epoll_create1(0);

    if (m_epFd < 0 || m_stopEventFd < 0 ||
        m_handoverEventFd < 0 || m_txEventFd < 0) {
        return false;
    }

    addToEpoll(m_stopEventFd, EPOLLIN);
    addToEpoll(m_handoverEventFd, EPOLLIN);
    addToEpoll(m_txEventFd, EPOLLIN);
    return true;
}

void TlsServer::deinit() {
    for (auto& kv : m_sslMap)
        SSL_free(kv.second);

    m_sslMap.clear();
    m_addrMap.clear();
    m_rxBuffer.clear();

    if (m_epFd >= 0) close(m_epFd);
    if (m_stopEventFd >= 0) close(m_stopEventFd);
    if (m_handoverEventFd >= 0) close(m_handoverEventFd);
    if (m_txEventFd >= 0) close(m_txEventFd);
}

void TlsServer::start() {
    m_running = true;
    startWorkers();

    epoll_event events[TLS_MAX_EVENTS];

    while (m_running) {
        int n = epoll_wait(m_epFd, events, TLS_MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        for (int i = 0; i < n; ++i)
            handleEvent(events[i]);
    }

    processHandoverQueue();
}

void TlsServer::stopReact() {
    m_running = false;
    m_cv.notify_all();
    uint64_t v = 1;
    write(m_stopEventFd, &v, sizeof(v));
}

void TlsServer::startWorkers() {
    for (int i = 0; i < m_workerCount; ++i) {
        m_threadManager->addThread(
            "tls_worker_" + std::to_string(i),
            std::bind(&TlsServer::processPacket, this),
            std::bind(&TlsServer::stopWorker, this));
    }
}

void TlsServer::stopWorker() {
    m_cv.notify_all();
}

void TlsServer::processPacket() {
    while (true) {
        std::unique_ptr<Packet> pkt;
        {
            std::unique_lock<std::mutex> lock(m_rxLock);
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

void TlsServer::handleEvent(const epoll_event& ev) {
    int fd = ev.data.fd;

    if (fd == m_stopEventFd)     return handleStopEvent();
    if (fd == m_handoverEventFd) return handleHandoverEvent();
    if (fd == m_txEventFd)       return handleTxEvent();

    if (ev.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
        handleClose(fd);
        return;
    }

    if (ev.events & EPOLLOUT)
        flushPendingForFd(fd, 256);

    if (!isHandshakeDone(fd)) {
        handleHandshake(fd);
        return;
    }

    if (ev.events & EPOLLIN)
        receivePacket(fd);
}

void TlsServer::handleStopEvent() {
    drainEventFd(m_stopEventFd);
    m_running = false;
    m_cv.notify_all();
}

void TlsServer::handleHandoverEvent() {
    drainEventFd(m_handoverEventFd);
    processHandoverQueue();
}

void TlsServer::handleTxEvent() {
    drainEventFd(m_txEventFd);
    flushAllPending(256);
}

void TlsServer::processHandoverQueue() {
    std::queue<HandoverItem> q;
    {
        std::lock_guard<std::mutex> lock(m_handoverLock);
        q.swap(m_handoverQueue);
    }

    while (!q.empty()) {
        auto item = q.front();
        q.pop();

        int fd = item.fd;

        SSL* ssl = SSL_new(m_ctx);
        if (!ssl) {
            close(fd);
            continue;
        }

        SSL_set_accept_state(ssl);
        SSL_set_fd(ssl, fd);

        m_sslMap.emplace(fd, ssl);
        m_addrMap.emplace(fd, item.connInfo);
        m_rxBuffer.emplace(fd, std::vector<uint8_t>{});

        addToEpoll(fd, EPOLLIN | EPOLLRDHUP);
    }
}

bool TlsServer::isHandshakeDone(int fd) const {
    auto it = m_sslMap.find(fd);
    return it != m_sslMap.end() &&
           SSL_is_init_finished(it->second);
}

void TlsServer::handleHandshake(int fd) {
    SSL* ssl = m_sslMap[fd];

    int ret = SSL_accept(ssl);
    if (ret == 1) {
        setInterest(fd, hasPendingTx(fd));
        receivePacket(fd);
        return;
    }

    int err = SSL_get_error(ssl, ret);
    if (err == SSL_ERROR_WANT_READ)  return;
    if (err == SSL_ERROR_WANT_WRITE) {
        setInterest(fd, true);
        return;
    }

    handleClose(fd);
}

void TlsServer::receivePacket(int fd) {
    SSL* ssl = m_sslMap[fd];
    auto& buf = m_rxBuffer.at(fd);

    while (true) {
        size_t oldSize = buf.size();
        buf.resize(oldSize + TLS_RECV_CHUNK_SIZE);

        int n = SSL_read(ssl, buf.data() + oldSize, TLS_RECV_CHUNK_SIZE);
        if (n > 0) {
            buf.resize(oldSize + (size_t)n);

            if (buf.size() > TLS_MAX_RX_BUFFER_SIZE) {
                handleClose(fd);
                return;
            }

            while (true) {
                if (buf.size() < TLS_HEADER_SIZE)
                    break;

                CommonPacketHeader hdr{};
                std::memcpy(&hdr, buf.data(), TLS_HEADER_SIZE);

                uint16_t bodyLen = ntohs(hdr.bodyLen);
                if (bodyLen > TLS_MAX_BODY_LEN) {
                    handleClose(fd);
                    return;
                }

                size_t frameLen = TLS_HEADER_SIZE + bodyLen;
                if (buf.size() < frameLen)
                    break;

                std::vector<uint8_t> payload(
                    buf.begin(), buf.begin() + frameLen);

                buf.erase(buf.begin(), buf.begin() + frameLen);

                auto& addr = m_addrMap[fd];
                auto pkt = std::make_unique<Packet>(
                    fd, Protocol::TLS,
                    std::move(payload),
                    addr.second,
                    addr.first);

                {
                    std::lock_guard<std::mutex> lock(m_rxLock);
                    m_rxQueue.push(std::move(pkt));
                }
                m_cv.notify_one();
            }
        } else {
            buf.resize(oldSize);

            int err = SSL_get_error(ssl, n);
            if (err == SSL_ERROR_WANT_READ)
                return;

            handleClose(fd);
            return;
        }
    }
}

void TlsServer::enqueueTx(std::unique_ptr<Packet> packet) {
    int fd = packet->getFd();
    {
        std::lock_guard<std::mutex> lock(m_txLock);
        m_txQueue[fd].push_back(std::move(packet));
    }
    uint64_t v = 1;
    write(m_txEventFd, &v, sizeof(v));
}

bool TlsServer::hasPendingTx(int fd) {
    std::lock_guard<std::mutex> lock(m_txLock);
    auto it = m_txQueue.find(fd);
    return it != m_txQueue.end() && !it->second.empty();
}

void TlsServer::flushAllPending(size_t budget) {
    std::vector<int> fds;
    {
        std::lock_guard<std::mutex> lock(m_txLock);
        for (auto& kv : m_txQueue)
            if (!kv.second.empty())
                fds.push_back(kv.first);
    }

    size_t used = 0;
    for (int fd : fds) {
        if (used >= budget) break;
        used += flushPendingForFd(fd, budget - used);
    }
}

size_t TlsServer::flushPendingForFd(int fd, size_t budget) {
    SSL* ssl = m_sslMap[fd];
    size_t used = 0;

    while (used < budget) {
        std::unique_ptr<Packet> pkt;
        {
            std::lock_guard<std::mutex> lock(m_txLock);
            auto it = m_txQueue.find(fd);
            if (it == m_txQueue.end() || it->second.empty()) {
                setInterest(fd, false);
                return used;
            }
            pkt = std::move(it->second.front());
            it->second.pop_front();
        }

        const auto& payload = pkt->getPayload();

        while (pkt->getTxOffset() < payload.size()){
            const uint8_t* p = payload.data() + pkt->getTxOffset();
            size_t bytes = payload.size() - pkt->getTxOffset();

            int ret = SSL_write(ssl, p, (int) bytes);
            if (ret > 0){
                pkt->updateTxOffset(size_t(ret));
                continue;
            }

            int err = SSL_get_error(ssl, ret);
            if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
                {
                    std::lock_guard<std::mutex> lock(m_txLock);
                    m_txQueue[fd].push_front(std::move(pkt));
                }
                setInterest(fd, true);
                return used + 1;
            }

            handleClose(fd);
            return used + 1;
        }
        used++;
    }
    setInterest(fd, hasPendingTx(fd));
    return used;
}

void TlsServer::handleClose(int fd) {
    epoll_ctl(m_epFd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);

    {
        std::lock_guard<std::mutex> lock(m_txLock);
        m_txQueue.erase(fd);
    }

    auto it = m_sslMap.find(fd);
    if (it != m_sslMap.end()) {
        SSL_free(it->second);
        m_sslMap.erase(it);
    }

    m_addrMap.erase(fd);
    m_rxBuffer.erase(fd);
}

bool TlsServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return flags >= 0 &&
           fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

void TlsServer::drainEventFd(int efd) {
    uint64_t v;
    while (read(efd, &v, sizeof(v)) > 0) {}
}

bool TlsServer::addToEpoll(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    return epoll_ctl(m_epFd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool TlsServer::modEpoll(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    return epoll_ctl(m_epFd, EPOLL_CTL_MOD, fd, &ev) == 0;
}

void TlsServer::setInterest(int fd, bool wantOut) {
    uint32_t ev = EPOLLIN | EPOLLRDHUP;
    if (wantOut) ev |= EPOLLOUT;
    modEpoll(fd, ev);
}

void TlsServer::handleTlsConnection(int fd, std::pair<sockaddr_in, sockaddr_in> connInfo){
    if (!setNonBlocking(fd)) {
        close(fd);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_handoverLock);
        m_handoverQueue.push(HandoverItem{fd, connInfo});
    }

    uint64_t v = 1;
    write(m_handoverEventFd, &v, sizeof(v));
}

