#pragma once

#include "ParsedPacketTypes.h"

#include <vector>
#include <cstdint>
#include <utility>

class ActionPacket {
public:
    ActionPacket(uint64_t sessionId, Opcode opcode);

    uint64_t getSessionId() const;

    Opcode getOpcode() const;

private:
    uint64_t m_sessionId;
    Opcode m_opcode;
};
