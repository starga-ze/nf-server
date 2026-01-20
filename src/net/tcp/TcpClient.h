#pragma once

#include <arpa/inet.h>
#include <atomic>

class TcpClient {
public:
    TcpClient(int id, int serverPort);

    ~TcpClient();

    void start();

    void stop();

    bool init();

    int getFd() const;

private:
    bool createSocket();

    bool connectServer();

    int m_id;
    int m_sock;
    int m_serverPort;
    struct sockaddr_in m_serverAddr;
    std::atomic<bool> m_running;
};

