#include "UdpClient.h"
#include "util/Logger.h"
#include "util/Random.h"
#include "util/ThreadManager.h"

#include <cstring>
#include <thread>
#include <chrono>
#include <unistd.h>

static const uint8_t TEST_PACKET[36]
        {
                0x01, 0x30, 0x00, 0x1C,
                0x00, 0x00, 0x00, 0x00,
                0x01, 0x00, 0x00, 0x00,
                0x64, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,
                0x01, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x80, 0x3F,
                0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x80, 0xBF
        };


UdpClient::UdpClient(int id, int serverPort) :
        m_id(id),
        m_serverPort(serverPort) {
    init();
}

UdpClient::~UdpClient() {
    stop();
}

bool UdpClient::init() {
    if (not createSocket()) {
        return false;
    }

    if (not connectServer()) {
        return false;
    }

    return true;
}

bool UdpClient::createSocket() {
    m_sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (m_sock < 0) {
        return false;
    }
    return true;
}

bool UdpClient::connectServer() {
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(m_serverPort);

    if (inet_pton(AF_INET, "127.0.0.1", &m_serverAddr.sin_addr) <= 0) {
        return false;
    }
    return true;
}

void UdpClient::start() {
    m_running = true;

    while (m_running) {
        int sent = sendto(m_sock,
                          TEST_PACKET,
                          sizeof(TEST_PACKET),
                          0,
                          (struct sockaddr *) &m_serverAddr,
                          sizeof(m_serverAddr));

        Random random;
        double sleepDuration = random.getRandomReal(1.0, 5.0);
        std::this_thread::sleep_for(std::chrono::duration<double>(sleepDuration));
    }

    close(m_sock);
}

void UdpClient::stop() {
    m_running = false;

    if (m_sock >= 0) {
        close(m_sock);
    }
}
