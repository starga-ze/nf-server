#include "TcpClient.h"
#include "util/Logger.h"
#include "util/Random.h"
#include "util/ThreadManager.h"

#include <cstring>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

static const uint8_t TEST_PACKET[36]
        {
                0x01, 0x20, 0x00, 0x1C,
                0x00, 0x00, 0x00, 0x00,
                0x01, 0x00, 0x00, 0x00,
                0x64, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,
                0x01, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x80, 0x3F,
                0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x80, 0xBF
        };

TcpClient::TcpClient(int id, int serverPort) :
        m_id(id),
        m_serverPort(serverPort),
        m_sock(-1),
        m_running(false) {
}

TcpClient::~TcpClient() {
    stop();
}

bool TcpClient::init() {
    if (m_sock >= 0) {
        return true;
    }

    if (!createSocket()) {
        return false;
    }

    if (!connectServer()) {
        close(m_sock);
        m_sock = -1;
        return false;
    }

    return true;
}

bool TcpClient::createSocket() {
    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sock < 0) {
        return false;
    }
    return true;
}

bool TcpClient::connectServer() {
    std::memset(&m_serverAddr, 0, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(m_serverPort);

    if (inet_pton(AF_INET, "127.0.0.1", &m_serverAddr.sin_addr) <= 0) {
        return false;
    }

    if (connect(m_sock, (struct sockaddr *) &m_serverAddr, sizeof(m_serverAddr)) < 0) {
        return false;
    }
    return true;
}

void TcpClient::start() {
    if (not init()) {
        LOG_ERROR("TcpClient[{}]: init/connect failed", m_id);
        return;
    }

    m_running = true;

    while (m_running) {
        ssize_t sent = send(m_sock, TEST_PACKET, sizeof(TEST_PACKET), 0);
        if (sent < 0) {
            LOG_ERROR("Failed to send message from client[{}]", m_id);
            break;
        }

        Random random;
        double sleepDuration = random.getRandomReal(1.0, 5.0);
        std::this_thread::sleep_for(std::chrono::duration<double>(sleepDuration));
    }

    stop();
}

void TcpClient::stop() {
    if (!m_running && m_sock < 0) {
        return;
    }

    m_running = false;

    if (m_sock >= 0) {
        close(m_sock);
        m_sock = -1;
    }
}

int TcpClient::getFd() const {
    return m_sock;
}

