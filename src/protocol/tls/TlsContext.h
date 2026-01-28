#pragma once

#include <string>
#include <openssl/ssl.h>

class TlsContext {
public:
    TlsContext();

    ~TlsContext();

    bool init(const std::string &certPath, const std::string &keyPath);

    SSL_CTX *get() const { return m_ctx; }

private:
    void logOpenSslError(const char *msg);

    SSL_CTX *m_ctx;
};

