// Session.h
#pragma once

#include "net/packet/ParsedPacket.h" // Protocol, ConnInfo
#include <cstdint>

enum class SessionState : uint8_t {
    PRE_AUTH,
    AUTH,
    IN_WORLD,
    CLOSED
};

class Session {
public:
    explicit Session(uint64_t sid)
        : m_sessionId(sid) {}

    void bind(const ParsedPacket& parsed) {
        m_connInfo = parsed.getConnInfo();

        switch (m_connInfo.protocol) {
        case Protocol::TCP:
        case Protocol::TLS:
            m_tcpFd = parsed.getFd();
            break;

        case Protocol::UDP:
            m_udpFd = parsed.getFd(); // server fd
            break;

        default:
            break;
        }
    }

    // getters (TxRouter용 – 필요 시)
    int getTcpFd() const { return m_tcpFd; }
    int getUdpFd() const { return m_udpFd; }
    const ConnInfo& getConnInfo() const { return m_connInfo; }

    SessionState getState() const { return m_state; }
    void setState(SessionState s) { m_state = s; }

private:
    uint64_t m_sessionId{0};

    SessionState m_state{SessionState::PRE_AUTH};

    // network
    int m_tcpFd{-1};        // TCP / TLS 공용
    int m_udpFd{-1};        // UDP server fd
    ConnInfo m_connInfo{};  // src/dst ip/port (UDP peer 포함)
};
 
