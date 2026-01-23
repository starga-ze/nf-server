#include "TlsClient.h"
#include "util/Logger.h"
#include "util/Random.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <thread>
#include <chrono>
#include <cstring>

static const uint8_t LOGIN_REQ_TEST_PACKET[28]
        {
                0x01, 0x10, 0x00, 0x0C, // ver + opcode + bodylen
                0x00, 0x00, 0x00, 0x00, // sessionId
                0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, // flags
                0x00, 0x04, 0x74, 0x65, // body
                0x73, 0x74, 0x00, 0x04,
                0x74, 0x65, 0x73, 0x74
        };

TlsClient::TlsClient(int id, int port)
        : m_id(id),
          m_port(port),
          m_tcp(id, port),
          m_ctx(nullptr),
          m_ssl(nullptr),
          m_running(false) {
}

TlsClient::~TlsClient() {
    stop();
}

void TlsClient::logError(const char *msg) {
    unsigned long e = ERR_get_error();
    if (e == 0) {
        LOG_ERROR("TlsClient[{}]: {} (no detail)", m_id, msg);
        return;
    }

    char buf[256];
    ERR_error_string_n(e, buf, sizeof(buf));
    LOG_ERROR("TlsClient[{}]: {}: {}", m_id, msg, buf);
}

bool TlsClient::initSsl(int fd) {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    m_ctx = SSL_CTX_new(TLS_client_method());
    if (!m_ctx) {
        logError("SSL_CTX_new failed");
        return false;
    }

    SSL_CTX_set_verify(m_ctx, SSL_VERIFY_NONE, nullptr);

    m_ssl = SSL_new(m_ctx);
    if (!m_ssl) {
        logError("SSL_new failed");
        return false;
    }

    if (SSL_set_fd(m_ssl, fd) != 1) {
        logError("SSL_set_fd failed");
        return false;
    }

    return true;
}

void TlsClient::start() {
    if (!m_tcp.init()) {
        LOG_ERROR("TlsClient[{}]: TCP init/connect failed", m_id);
        return;
    }

    int fd = m_tcp.getFd();
    LOG_INFO("TlsClient[{}]: TCP connected, fd={}", m_id, fd);

    if (!initSsl(fd)) {
        LOG_ERROR("TlsClient[{}]: initSsl failed", m_id);
        stop();
        return;
    }

    m_running = true;

    int ret = SSL_connect(m_ssl);
    if (ret != 1) {
        logError("SSL_connect failed");
        stop();
        return;
    }

    LOG_INFO("TlsClient[{}]: TLS Handshake complete!", m_id);

    while (m_running) {
        /*
        Random random;
        double sleepDuration = random.getRandomReal(10.0, 20.0);
        std::this_thread::sleep_for(std::chrono::duration<double>(sleepDuration));
        */

        int wr = SSL_write(m_ssl, LOGIN_REQ_TEST_PACKET, sizeof(LOGIN_REQ_TEST_PACKET));
        if (wr <= 0) {
            int err = SSL_get_error(m_ssl, wr);
            LOG_ERROR("TlsClient[{}]: SSL_write failed (err={})", m_id, err);
            break;
        }
 
        Random random;
        double sleepDuration = random.getRandomReal(1.0, 2.0);
        std::this_thread::sleep_for(std::chrono::duration<double>(sleepDuration));
        
    }

    stop();
}

void TlsClient::stop() {
    if (!m_running && !m_ssl && !m_ctx)
        return;

    m_running = false;

    if (m_ssl) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
        m_ssl = nullptr;
    }

    if (m_ctx) {
        SSL_CTX_free(m_ctx);
        m_ctx = nullptr;
    }

    m_tcp.stop();
}

