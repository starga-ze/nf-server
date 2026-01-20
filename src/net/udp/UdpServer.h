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

#ifndef UDP_RECV_BUFFER_SIZE
#define UDP_RECV_BUFFER_SIZE (4 * 1024)
#endif

#ifndef UDP_MAX_EVENTS
#define UDP_MAX_EVENTS 64
#endif

class UdpServer {
public:
    UdpServer(int port,
              RxRouter *rxRouter,
              int workerCount,
              ThreadManager *threadManager);

    ~UdpServer();

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

    void handleStopEvent();

    void handleTxEvent();

    void receivePacket();

    void handleClose();

    bool hasPendingTx();

    void flushAllPending(size_t budgetItems);

    size_t flushPending(size_t budgetItems);

    bool setNonBlocking(int fd);

    void drainEventFd(int efd);

    bool addToEpoll(int fd, uint32_t events);

private:
    int m_port;
    int m_sockFd{-1};
    int m_epFd{-1};

    int m_stopEventFd{-1};
    int m_txEventFd{-1};

    sockaddr_in m_serverAddr{};

    RxRouter *m_rxRouter;
    ThreadManager *m_threadManager;

    std::atomic<bool> m_running{false};
    int m_workerCount;

    std::vector <uint8_t> m_buffer;

    std::mutex m_rxLock;
    std::condition_variable m_cv;
    std::queue <std::unique_ptr<Packet>> m_rxQueue;

    std::mutex m_txLock;
    std::deque <std::unique_ptr<Packet>> m_txQueue;
};

