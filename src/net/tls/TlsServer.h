#pragma once

#include <openssl/ssl.h>

#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <deque>
#include <unordered_map>
#include <vector>
#include <utility>
#include <atomic>

#include <sys/epoll.h>
#include <netinet/in.h>

class RxRouter;

class ThreadManager;

class Packet;

class TlsServer {
public:
    struct HandoverItem {
        int fd = -1;
        std::pair <sockaddr_in, sockaddr_in> connInfo;
    };

    TlsServer(SSL_CTX *ctx, RxRouter *rxRouter, int workerCount, ThreadManager *threadManager);

    ~TlsServer();

    void start();

    void stopReact();

    void handleTlsConnection(int fd, std::pair <sockaddr_in, sockaddr_in> connInfo);

    void enqueueTx(std::unique_ptr <Packet> packet);

private:
    bool init();

    void deinit();

    void startWorkers();

    void stopWorker();

    void processPacket();

    void handleEvent(const epoll_event &ev);

    void handleStopEvent();

    void handleHandoverEvent();

    void handleTxEvent();

    void processHandoverQueue();

    bool isHandshakeDone(int fd) const;

    void handleHandshake(int fd);

    void receivePacket(int fd);

    void handleClose(int fd);

    bool setNonBlocking(int fd);

    void drainEventFd(int efd);

    bool addToEpoll(int fd, uint32_t events);

    bool modEpoll(int fd, uint32_t events);

    void setInterest(int fd, bool wantOut);

    bool hasPendingTx(int fd);

    void flushAllPending(size_t budgetItems);

    size_t flushPendingForFd(int fd, size_t budgetItems);

    SSL_CTX *m_ctx;

    int m_epFd;
    int m_stopEventFd;
    int m_handoverEventFd;
    int m_txEventFd;

    std::atomic<bool> m_running{false};
    int m_workerCount;

    ThreadManager *m_threadManager;
    RxRouter *m_rxRouter;

    std::vector <uint8_t> m_buffer;

    std::unordered_map<int, SSL *> m_sslMap;
    std::unordered_map<int, std::pair<sockaddr_in, sockaddr_in>> m_addrMap;

    std::mutex m_rxLock;
    std::condition_variable m_cv;
    std::queue <std::unique_ptr<Packet>> m_rxQueue;

    std::mutex m_handoverLock;
    std::queue <HandoverItem> m_handoverQueue;

    std::mutex m_txLock;
    std::unordered_map<int, std::deque<std::unique_ptr < Packet>>>
    m_txQueue;
};


