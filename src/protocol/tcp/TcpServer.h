#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>
#include <memory>

#include <netinet/in.h>
#include <sys/epoll.h>

class RxRouter;

class ThreadManager;

class Packet;

class TlsServer;

class TcpServer {
public:
    TcpServer(int port,
              RxRouter *rxRouter,
              int workerCount,
              ThreadManager *threadManager,
              std::shared_ptr <TlsServer> tlsServer);

    ~TcpServer();

    void start();

    void stopReact();

    void enqueueTx(std::unique_ptr <Packet> packet);

private:
    bool init();

    void deinit();

    void startWorkers();

    void stopWorker();

    void processPacket();

    void handleEvent(const epoll_event &ev);

    void handleTxEvent();

    void acceptConnection();

    void receivePacket(int fd);

    void closeConnection(int fd);

    bool hasPendingTx(int fd);

    void flushAllPending(size_t budget);

    size_t flushPendingForFd(int fd, size_t budget);

    bool isTlsClientHello(int fd);

    bool handoverToTls(int fd);

    bool setNonBlocking(int fd);

    void drainEventFd(int efd);

    bool addToEpoll(int fd, uint32_t events);

    bool modEpoll(int fd, uint32_t events);

    void setInterest(int fd, bool wantOut);

    int m_port;
    int m_sockFd{-1};
    int m_epFd{-1};
    int m_txEventFd{-1};

    sockaddr_in m_serverAddr{};

    RxRouter *m_rxRouter;
    ThreadManager *m_threadManager;
    std::shared_ptr <TlsServer> m_tlsServer;

    std::atomic<bool> m_running{false};
    int m_workerCount;

    std::unordered_map<int, sockaddr_in> m_clients;
    std::unordered_map<int, std::vector<uint8_t>> m_rxBuffer;

    std::mutex m_rxLock;
    std::condition_variable m_cv;
    std::queue <std::unique_ptr<Packet>> m_rxQueue;

    std::mutex m_txLock;
    std::unordered_map<int, std::deque<std::unique_ptr < Packet>>>
    m_txQueue;
};

