#pragma once

#include <arpa/inet.h>
#include <atomic>

class UdpClient {
public:
    UdpClient(int id, int serverPort);

    ~UdpClient();

    void start();

    void stop();

private:
    bool init();

    bool createSocket();

    bool connectServer();

    int m_id;
    int m_sock;
    int m_serverPort;
    struct sockaddr_in m_serverAddr;
    std::atomic<bool> m_running;
};
