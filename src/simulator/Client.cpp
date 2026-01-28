#include "Client.h"
#include "util/Logger.h"

#include <thread>
#include <chrono>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdint>

Client::Client(int id, int udpServerPort, int tcpServerPort)
    : m_id(id),
      m_udpServerPort(udpServerPort),
      m_tcpServerPort(tcpServerPort)
{
    init();
}

Client::~Client()
{
    stop();
}

void Client::init()
{
    m_udpClient = std::make_unique<UdpClient>(m_id, m_udpServerPort);
    m_tcpClient = std::make_unique<TcpClient>(m_id, m_tcpServerPort);
    m_tlsClient = std::make_unique<TlsClient>(m_id, m_tcpServerPort);
}

void Client::start()
{
    if (not m_tlsClient->connect())
    {
        LOG_FATAL("TLS connect failed");
        return;
    }

    if (not m_tcpClient->connect())
    {
        LOG_FATAL("TCP connect failed");
        return;
    }

    loginPhase();

    m_running = true;
    while(m_running)
    {
        lobbyPhase();
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

void Client::loginPhase()
{
    auto pkt = buildLoginReq();

    if (!m_tlsClient->send(pkt.data(), pkt.size())) {
        LOG_FATAL("LOGIN_REQ send failed");
        return;
    }

    uint8_t buf[1024];
    ssize_t n;

    while (true) {
        n = m_tlsClient->recv(buf, sizeof(buf));
        if (n > 0)
            break;
        if (n < 0) {
            LOG_FATAL("TLS recv fatal");
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    uint64_t sessionId = 0;
    std::memcpy(&sessionId, buf + 4, sizeof(sessionId));
    m_sessionId = be64toh(sessionId);
    return;
}

void Client::lobbyPhase()
{
    auto pkt = buildEnterLobbyReq();

    if (not m_tcpClient->send(pkt.data(), pkt.size())) {
        LOG_FATAL("ENTER_LOBBY_REQ send failed");
        return;
    }
}

void Client::stop()
{
    m_running.store(false);
    m_tlsClient->stop();
    m_tcpClient->stop();
    m_udpClient->stop();
}

std::vector<uint8_t> Client::buildLoginReq()
{
    const char* id = "test";
    const char* pw = "test";

    uint16_t bodyLen =
        2 + std::strlen(id) +
        2 + std::strlen(pw);

    std::vector<uint8_t> pkt(16 + bodyLen);

    pkt[0] = 0x01;  
    pkt[1] = 0x10;  
    pkt[2] = bodyLen >> 8;
    pkt[3] = bodyLen & 0xFF;

    uint64_t sid = 0;
    std::memcpy(pkt.data() + 4, &sid, sizeof(sid));

    std::memset(pkt.data() + 12, 0, 4);

    size_t off = 16;

    uint16_t idLen = std::strlen(id);
    pkt[off++] = idLen >> 8;
    pkt[off++] = idLen & 0xFF;
    std::memcpy(pkt.data() + off, id, idLen);
    off += idLen;

    uint16_t pwLen = std::strlen(pw);
    pkt[off++] = pwLen >> 8;
    pkt[off++] = pwLen & 0xFF;
    std::memcpy(pkt.data() + off, pw, pwLen);

    return pkt;
}

std::vector<uint8_t> Client::buildEnterLobbyReq()
{
    std::vector<uint8_t> pkt(16);

    pkt[0] = 0x01;                    
    pkt[1] = 0x20;  
    pkt[2] = 0x00;                    
    pkt[3] = 0x00;  

    uint64_t sessionId = htobe64(m_sessionId);
    std::memcpy(pkt.data() + 4, &sessionId, sizeof(sessionId));

    std::memset(pkt.data() + 12, 0, 4);

    return pkt;
}

void Client::dumpHex(const uint8_t* buf, size_t len)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (size_t i = 0; i < len; i++) {
        oss << std::setw(2) << (int)buf[i] << ' ';
        if ((i + 1) % 16 == 0)
            oss << '\n';
    }

    LOG_TRACE("Client rx dump (len={}):\n{}", len, oss.str());
}

