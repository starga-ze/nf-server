#pragma once

#include "net/packet/Packet.h"
#include "net/packet/ParsedPacketTypes.h"
#include "net/packet/EventPacket.h"

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

class ParsedPacket {
public:
    ParsedPacket(
            int fd,
            ConnInfo connInfo,
            PacketVersion version,
            Opcode opcode,
            uint32_t flags,
            uint64_t sessionId,
            std::vector <uint8_t> payload,
            size_t bodyOffset,
            size_t bodyLen
    );

    std::vector<uint8_t> takePayload();

    const uint8_t* bodyData() const;

    size_t bodySize() const;

    const std::vector<uint8_t>& payload() const;

    // getters
    int getFd() const;

    const ConnInfo &getConnInfo() const;

    PacketVersion version() const;

    Opcode opcode() const;

    uint32_t flags() const;

    uint64_t getSessionId() const;

    std::vector <uint8_t> extractBody();

private:
    int m_fd{0};
    ConnInfo m_connInfo;

    PacketVersion m_version{};
    Opcode m_opcode{};
    uint32_t m_flags{};
    uint64_t m_sessionId{};

    std::vector <uint8_t> m_payload;
    size_t m_bodyOffset;
    size_t m_bodyLen;
};

