#include "ActionPacket.h"

#include <utility>

ActionPacket::ActionPacket(uint64_t sessionId,
                           Opcode opcode)
        : m_sessionId(sessionId),
          m_opcode(opcode) {
}

uint64_t ActionPacket::getSessionId() const {
    return m_sessionId;
}

Opcode ActionPacket::getOpcode() const {
    return m_opcode;
}

