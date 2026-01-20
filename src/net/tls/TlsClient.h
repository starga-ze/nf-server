#pragma once

#include "net/tcp/TcpClient.h"

#include <openssl/ssl.h>
#include <atomic>

class TlsClient {
public:
    TlsClient(int id, int port);

    ~TlsClient();

    void start();

    void stop();

private:
    bool initSsl(int fd);

    void logError(const char *msg);

    int m_id;
    int m_port;

    TcpClient m_tcp;

    SSL_CTX *m_ctx;
    SSL *m_ssl;

    std::atomic<bool> m_running;
};


