#include "PacketParser.h"
#include "util/Logger.h"

#include <cstring>
#include <arpa/inet.h>
#include <endian.h>

/*
 * note that /src/net/packet/ParsedPacketTypes.h
 *
 * CommonPacketHeader (16 bytes)
 * 
 * offset  size  field
 * 0       1     version
 * 1       1     opcode
 * 2       2     bodyLen (uint16, network order)
 * 4       8     sessionId (uint64, network order)
 * 12      4     flags (uint32, network order)
*/

std::optional <ParsedPacket> PacketParser::parse(std::unique_ptr <Packet> packet) const {
    if (!packet) {
        LOG_WARN("packet is null");
        return std::nullopt;
    }

    auto payload = packet->takePayload();
    
    constexpr size_t HEADER_SIZE = 16;

    if (payload.size() < HEADER_SIZE) {
        LOG_WARN("payload < header size");
        return std::nullopt;
    }

    const uint8_t *p = payload.data();

    PacketVersion version = static_cast<PacketVersion>(p[0]);

    Opcode opcode = static_cast<Opcode>(p[1]);

    uint16_t bodyLen;
    std::memcpy(&bodyLen, p + 2, sizeof(uint16_t));
    bodyLen = ntohs(bodyLen);

    uint64_t sessionId;
    std::memcpy(&sessionId, p + 4, sizeof(uint64_t));
    sessionId = be64toh(sessionId);

    uint32_t flags;
    std::memcpy(&flags, p + 12, sizeof(uint32_t));
    flags = ntohl(flags);

    if (payload.size() < HEADER_SIZE + bodyLen) {
        LOG_WARN("payload < header + bodyLen");
        return std::nullopt;
    }

    return ParsedPacket(packet->getFd(), packet->getConnInfo(), version, opcode,
            flags, sessionId, std::move(payload), HEADER_SIZE, bodyLen);
}

