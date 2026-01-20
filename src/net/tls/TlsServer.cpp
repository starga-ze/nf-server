#include "TlsServer.h"
#include "util/Logger.h"

#include "net/packet/Packet.h"
#include "ingress/RxRouter.h"
#include "util/ThreadManager.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>

#include <vector>

#ifndef TLS_MAX_EVENTS
#define TLS_MAX_EVENTS 64
#endif

#ifndef TLS_RECV_BUFFER_SIZE
#define TLS_RECV_BUFFER_SIZE 4096
#endif

TlsServer::TlsServer(SSL_CTX *ctx,
                     RxRouter *rxRouter,
                     int workerCount,
                     ThreadManager *threadManager)
        : m_ctx(ctx),
          m_epFd(-1),
          m_stopEventFd(-1),
          m_handoverEventFd(-1),
          m_txEventFd(-1),
          m_running(false),
          m_workerCount(workerCount),
          m_threadManager(threadManager),
          m_rxRouter(rxRouter) {
    init();
}

TlsServer::~TlsServer() {
    deinit();
}

bool TlsServer::init() {
    m_stopEventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_stopEventFd < 0) {
        LOG_ERROR("TlsServer: stop eventfd create failed");
        return false;
    }

    m_handoverEventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_handoverEventFd < 0) {
        LOG_ERROR("TlsServer: handover eventfd create failed");
        close(m_stopEventFd);
        m_stopEventFd = -1;
        return false;
    }

    m_txEventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_txEventFd < 0) {
        LOG_ERROR("TlsServer: tx eventfd create failed");
        close(m_stopEventFd);
        close(m_handoverEventFd);
        m_stopEventFd = -1;
        m_handoverEventFd = -1;
        return false;
    }

    m_epFd = epoll_create1(0);
    if (m_epFd < 0) {
        LOG_ERROR("TlsServer: epoll_create1 failed");
        close(m_stopEventFd);
        close(m_handoverEventFd);
        close(m_txEventFd);
        m_stopEventFd = -1;
        m_handoverEventFd = -1;
        m_txEventFd = -1;
        return false;
    }

    if (!addToEpoll(m_stopEventFd, EPOLLIN) ||
        !addToEpoll(m_handoverEventFd, EPOLLIN) ||
        !addToEpoll(m_txEventFd, EPOLLIN)) {
        LOG_ERROR("TlsServer: epoll_ctl add eventfd failed");
        return false;
    }

    return true;
}

void TlsServer::deinit() {
    for (auto &kv: m_sslMap) {
        SSL_free(kv.second);
    }
    m_sslMap.clear();
    m_addrMap.clear();

    {
        std::lock_guard <std::mutex> lock(m_txLock);
        m_txQueue.clear();
    }

    if (m_epFd >= 0) {
        close(m_epFd);
        m_epFd = -1;
    }
    if (m_stopEventFd >= 0) {
        close(m_stopEventFd);
        m_stopEventFd = -1;
    }
    if (m_handoverEventFd >= 0) {
        close(m_handoverEventFd);
        m_handoverEventFd = -1;
    }
    if (m_txEventFd >= 0) {
        close(m_txEventFd);
        m_txEventFd = -1;
    }
}

void TlsServer::start() {
    m_running = true;
    startWorkers();

    epoll_event events[TLS_MAX_EVENTS];
    m_buffer.resize(TLS_RECV_BUFFER_SIZE);

    while (m_running) {
        int n = epoll_wait(m_epFd, events, TLS_MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("TlsServer: epoll_wait failed errno={}", errno);
            break;
        }

        for (int i = 0; i < n; ++i) {
            handleEvent(events[i]);
        }
    }

    processHandoverQueue();
}

void TlsServer::stopReact() {
    m_running = false;
    m_cv.notify_all();

    if (m_stopEventFd >= 0) {
        uint64_t v = 1;
        if (write(m_stopEventFd, &v, sizeof(v)) < 0) {
            LOG_FATAL("TlsServer: stop eventfd write failed, errno={}", errno);
        }
    }
}

void TlsServer::handleTlsConnection(int fd, std::pair <sockaddr_in, sockaddr_in> connInfo) {
    LOG_INFO("TlsServer: Received FD {} for TLS upgrade", fd);

    if (!setNonBlocking(fd)) {
        LOG_WARN("TlsServer: setNonBlocking failed fd={}, errno={}", fd, errno);
    }

    {
        std::lock_guard <std::mutex> lock(m_handoverLock);
        m_handoverQueue.push(HandoverItem{fd, connInfo});
    }

    uint64_t v = 1;
    (void) write(m_handoverEventFd, &v, sizeof(v));
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
        std::unique_ptr <Packet> packet;
        {
            std::unique_lock <std::mutex> lock(m_rxLock);

            while (m_rxQueue.empty() && m_running) {
                m_cv.wait(lock);
            }

            if (!m_running && m_rxQueue.empty()) {
                break;
            }

            packet = std::move(m_rxQueue.front());
            m_rxQueue.pop();
        }

        m_rxRouter->handlePacket(std::move(packet));
    }
}

void TlsServer::handleEvent(const epoll_event &ev) {
    const int fd = ev.data.fd;

    if (fd == m_stopEventFd) {
        handleStopEvent();
        return;
    }
    if (fd == m_handoverEventFd) {
        handleHandoverEvent();
        return;
    }
    if (fd == m_txEventFd) {
        handleTxEvent();
        return;
    }

    if (ev.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
        handleClose(fd);
        return;
    }

    if (ev.events & EPOLLOUT) {
        flushPendingForFd(fd, 256);
    }

    if (!isHandshakeDone(fd)) {
        handleHandshake(fd);
        return;
    }

    if (ev.events & EPOLLIN) {
        receivePacket(fd);
    }
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
    std::queue <HandoverItem> snapshot;
    {
        std::lock_guard <std::mutex> lock(m_handoverLock);
        snapshot.swap(m_handoverQueue);
    }

    while (!snapshot.empty()) {
        HandoverItem item = std::move(snapshot.front());
        snapshot.pop();

        const int fd = item.fd;

        if (m_sslMap.find(fd) != m_sslMap.end()) {
            LOG_WARN("TLS [{}]: already exists in sslMap", fd);
            continue;
        }

        SSL *ssl = SSL_new(m_ctx);
        if (!ssl) {
            LOG_ERROR("TLS [{}]: SSL_new failed", fd);
            close(fd);
            continue;
        }

        SSL_set_accept_state(ssl);
        SSL_set_fd(ssl, fd);

        m_sslMap.emplace(fd, ssl);
        m_addrMap.emplace(fd, item.connInfo);

        if (!addToEpoll(fd, EPOLLIN | EPOLLRDHUP)) {
            LOG_ERROR("TLS [{}]: epoll_ctl ADD failed", fd);
            SSL_free(ssl);
            m_sslMap.erase(fd);
            m_addrMap.erase(fd);
            close(fd);
            continue;
        }
    }
}

bool TlsServer::isHandshakeDone(int fd) const {
    auto it = m_sslMap.find(fd);
    if (it == m_sslMap.end() || it->second == nullptr)
        return false;
    return SSL_is_init_finished(it->second);
}

void TlsServer::handleHandshake(int fd) {
    auto it = m_sslMap.find(fd);
    if (it == m_sslMap.end() || it->second == nullptr) {
        handleClose(fd);
        return;
    }

    SSL *ssl = it->second;

    int ret = SSL_accept(ssl);
    if (ret == 1) {
        LOG_INFO("TLS [{}]: Handshake SUCCESS", fd);

        setInterest(fd, hasPendingTx(fd));

        receivePacket(fd);
        return;
    }

    int err = SSL_get_error(ssl, ret);
    if (err == SSL_ERROR_WANT_READ) {
        setInterest(fd, hasPendingTx(fd));
        return;
    }
    if (err == SSL_ERROR_WANT_WRITE) {
        setInterest(fd, true);
        return;
    }

    LOG_ERROR("TLS [{}]: Handshake FAILED (err={})", fd, err);
    handleClose(fd);
}

void TlsServer::receivePacket(int fd) {
    auto it = m_sslMap.find(fd);
    if (it == m_sslMap.end() || it->second == nullptr) {
        handleClose(fd);
        return;
    }

    SSL *ssl = it->second;

    while (true) {
        ssize_t bytes = SSL_read(ssl, m_buffer.data(), m_buffer.size());

        if (bytes > 0) {
            auto addrIt = m_addrMap.find(fd);
            if (addrIt == m_addrMap.end()) {
                LOG_ERROR("TLS [{}]: addrMap missing", fd);
                handleClose(fd);
                return;
            }

            std::vector <uint8_t> payload(m_buffer.begin(), m_buffer.begin() + bytes);

            const auto &serverAddr = addrIt->second.first;
            const auto &clientAddr = addrIt->second.second;

            auto packet = std::make_unique<Packet>(
                    fd, Protocol::TLS, std::move(payload), clientAddr, serverAddr);

            {
                std::lock_guard <std::mutex> lock(m_rxLock);
                m_rxQueue.push(std::move(packet));
            }
            m_cv.notify_one();

            if (SSL_pending(ssl) > 0)
                continue;

            return;
        }

        if (bytes == 0) {
            LOG_INFO("TLS [{}]: Client closed", fd);
            handleClose(fd);
            return;
        }

        int err = SSL_get_error(ssl, (int) bytes);
        if (err == SSL_ERROR_WANT_READ) {
            return;
        }

        if (err == SSL_ERROR_WANT_WRITE) {
            LOG_ERROR("TLS [{}]: unexpected WANT_WRITE during SSL_read (renegotiation not supported)", fd);
            handleClose(fd);
            return;
        }

        LOG_ERROR("TLS [{}]: SSL_read failed (err={})", fd, err);
        handleClose(fd);
        return;
    }
}

void TlsServer::enqueueTx(std::unique_ptr <Packet> packet) {
    if (!packet) return;

    const int fd = packet->getFd();

    {
        std::lock_guard <std::mutex> lock(m_txLock);
        m_txQueue[fd].push_back(std::move(packet));
    }

    uint64_t v = 1;
    (void) write(m_txEventFd, &v, sizeof(v));
}

bool TlsServer::hasPendingTx(int fd) {
    std::lock_guard <std::mutex> lock(m_txLock);
    auto it = m_txQueue.find(fd);
    return (it != m_txQueue.end() && !it->second.empty());
}

void TlsServer::flushAllPending(size_t budgetItems) {
    std::vector<int> fds;
    {
        std::lock_guard <std::mutex> lock(m_txLock);
        fds.reserve(m_txQueue.size());
        for (auto &kv: m_txQueue) {
            if (!kv.second.empty())
                fds.push_back(kv.first);
        }
    }

    size_t used = 0;
    for (int fd: fds) {
        if (used >= budgetItems) break;
        used += flushPendingForFd(fd, budgetItems - used);
    }
}

size_t TlsServer::flushPendingForFd(int fd, size_t budgetItems) {
    auto it = m_sslMap.find(fd);
    if (it == m_sslMap.end() || it->second == nullptr) {
        std::lock_guard <std::mutex> lock(m_txLock);
        m_txQueue.erase(fd);
        return 0;
    }

    SSL *ssl = it->second;

    size_t used = 0;
    while (used < budgetItems) {
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
            int ret = SSL_write(ssl, p, (int) bytes);
            if (ret > 0) {
                pkt->updateTxOffset((size_t) ret);
                p += ret;
                bytes -= (size_t) ret;
                continue;
            }

            int err = SSL_get_error(ssl, ret);
            if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
                {
                    std::lock_guard <std::mutex> lock(m_txLock);
                    m_txQueue[fd].push_front(std::move(pkt));
                }
                setInterest(fd, true);
                return used + 1;
            }

            LOG_ERROR("TLS [{}]: SSL_write failed (err={})", fd, err);
            handleClose(fd);
            return used + 1;
        }

        used++;
    }

    if (hasPendingTx(fd))
        setInterest(fd, true);
    else
        setInterest(fd, false);

    return used;
}

void TlsServer::handleClose(int fd) {
    epoll_ctl(m_epFd, EPOLL_CTL_DEL, fd, nullptr);

    {
        std::lock_guard <std::mutex> lock(m_txLock);
        m_txQueue.erase(fd);
    }

    auto it = m_sslMap.find(fd);
    if (it != m_sslMap.end()) {
        SSL_free(it->second);
        m_sslMap.erase(it);
    }

    m_addrMap.erase(fd);
    close(fd);
}

bool TlsServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0);
}

void TlsServer::drainEventFd(int efd) {
    uint64_t v;
    while (true) {
        ssize_t n = read(efd, &v, sizeof(v));
        if (n == (ssize_t)sizeof(v)) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        break;
    }
}

bool TlsServer::addToEpoll(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    return (epoll_ctl(m_epFd, EPOLL_CTL_ADD, fd, &ev) == 0);
}

bool TlsServer::modEpoll(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    return (epoll_ctl(m_epFd, EPOLL_CTL_MOD, fd, &ev) == 0);
}

void TlsServer::setInterest(int fd, bool wantOut) {
    uint32_t ev = EPOLLIN | EPOLLRDHUP;
    if (wantOut) ev |= EPOLLOUT;
    (void) modEpoll(fd, ev);
}

