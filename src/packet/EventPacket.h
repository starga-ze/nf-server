#pragma once

#include <vector>
#include <cstdint>
#include "ParsedPacketTypes.h"

class EventPacket {
public:
    EventPacket(uint64_t sessionId, Opcode opcode, std::vector <uint8_t> body);

    uint64_t getSessionId() const;

    Opcode getOpcode() const;

    const std::vector <uint8_t> &getBody() const;

private:
    uint64_t m_sessionId;
    Opcode m_opcode;
    std::vector <uint8_t> m_body;
};
