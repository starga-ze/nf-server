#pragma once

#include "net/packet/Packet.h"
#include "net/packet/ParsedPacketTypes.h"

#include <cstddef>
#include <cstdint>

struct SessionId {
    uint64_t sessionId{0};
};

enum class SessionState : uint8_t {
    CONNECTED,
    LOGGED_IN,
    IN_WORLD,
    CLOSED
};

class Session {
public:
    Session(uint64_t sessionId);

    SessionId getSessionId() const;

    int getFd() const;

    Protocol getProtocol() const;

    SessionState getState() const;

    void setState(SessionState state);

    void bind(const ConnInfo &connInfo, int fd);

    const ConnInfo &getConnInfo() const;

private:
    SessionId m_sessionId;

    SessionState m_state{SessionState::CLOSED};

    ConnInfo m_connInfo;

    /* May need atomic -> TODO */
    uint32_t m_dstIp;
    uint32_t m_srcIp;
    Protocol m_protocol{Protocol::UNKNOWN};
    int m_fd{-1};
};

