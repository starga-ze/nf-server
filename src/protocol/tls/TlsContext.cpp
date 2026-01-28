#include "TlsContext.h"
#include "util/Logger.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

TlsContext::TlsContext()
        : m_ctx(nullptr) {
}

TlsContext::~TlsContext() {
    if (m_ctx) {
        SSL_CTX_free(m_ctx);
        m_ctx = nullptr;
    }
}

void TlsContext::logOpenSslError(const char *msg) {
    unsigned long e = ERR_get_error();
    char buf[256];
    ERR_error_string_n(e, buf, sizeof(buf));
    LOG_FATAL("{}: {}", msg, buf);
}

bool TlsContext::init(const std::string &certPath, const std::string &keyPath) {
    if (OPENSSL_init_ssl(0, nullptr) == 0) {
        LOG_FATAL("OPENSSL_init_ssl failed");
        return false;
    }

    const SSL_METHOD *method = TLS_server_method();
    if (!method) {
        logOpenSslError("TLS_server_method failed");
        return false;
    }

    m_ctx = SSL_CTX_new(method);
    if (!m_ctx) {
        logOpenSslError("SSL_CTX_new failed");
        return false;
    }

    SSL_CTX_set_min_proto_version(m_ctx, TLS1_2_VERSION);

    if (SSL_CTX_use_certificate_file(m_ctx, certPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        logOpenSslError("SSL_CTX_use_certificate_file failed");
        SSL_CTX_free(m_ctx);
        m_ctx = nullptr;
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(m_ctx, keyPath.c_str(), SSL_FILETYPE_PEM) <= 0) {
        logOpenSslError("SSL_CTX_use_PrivateKey_file failed");
        SSL_CTX_free(m_ctx);
        m_ctx = nullptr;
        return false;
    }

    if (SSL_CTX_check_private_key(m_ctx) <= 0) {
        logOpenSslError("SSL_CTX_check_private_key failed");
        SSL_CTX_free(m_ctx);
        m_ctx = nullptr;
        return false;
    }

    LOG_INFO("TLS context loaded successfully (cert={}, key={})",
             certPath.c_str(), keyPath.c_str());

    return true;
}

