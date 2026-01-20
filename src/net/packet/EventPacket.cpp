#include "EventPacket.h"

#include <utility>

EventPacket::EventPacket(uint64_t sessionId,
                         Opcode opcode,
                         std::vector <uint8_t> body)
        : m_sessionId(sessionId),
          m_opcode(opcode),
          m_body(std::move(body)) {
}

uint64_t EventPacket::getSessionId() const {
    return m_sessionId;
}

Opcode EventPacket::getOpcode() const {
    return m_opcode;
}

const std::vector <uint8_t> &EventPacket::getBody() const {
    return m_body;
}

