#include "PacketParser.h"
#include "util/Logger.h"

#include <cstring>
#include <arpa/inet.h>

static const uint64_t FNV_OFFSET_BASIS = 1469598103934665603ULL;
static const uint64_t FNV_PRIME = 1099511628211ULL;
/*
 * Note that /src/net/packet/ParsedPacketTypes.h
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

    uint32_t flags;
    std::memcpy(&flags, p + 4, sizeof(uint32_t));
    flags = ntohl(flags);

    if (payload.size() < HEADER_SIZE + bodyLen) {
        LOG_WARN("payload < header + bodyLen");
        return std::nullopt;
    }

    if (opcode == Opcode::LOGIN_REQ) {
        if (sessionId == 0) {
            return ParsedPacket(packet->getFd(), packet->getConnInfo(), version, opcode,
                    flags, resolvePreSessionId(*packet), std::move(payload), HEADER_SIZE, bodyLen);
        }

        else {
            LOG_WARN("LOGIN_REQ with non-zero sessionId");
            return std::nullopt;
        }
    } else {
        if (sessionId == 0) {
            LOG_WARN("Non-login REQ packet without sessionId");
            return std::nullopt;
        }
    }

    return ParsedPacket(packet->getFd(), packet->getConnInfo(), version, opcode,
            flags, sessionId, std::move(payload), HEADER_SIZE, bodyLen);
}

uint64_t PacketParser::fnv1aMix(uint64_t hash, uint64_t value) const {
    hash ^= value;
    hash *= FNV_PRIME;
    return hash;
}

uint64_t PacketParser::resolvePreSessionId(const Packet &packet) const {
    uint64_t hash = FNV_OFFSET_BASIS;

    hash = fnv1aMix(hash, static_cast<uint64_t>(packet.getProtocol()));
    hash = fnv1aMix(hash, static_cast<uint64_t>(packet.getSrcIp()));
    hash = fnv1aMix(hash, static_cast<uint64_t>(packet.getSrcPort()));
    hash = fnv1aMix(hash, static_cast<uint64_t>(packet.getDstIp()));
    hash = fnv1aMix(hash, static_cast<uint64_t>(packet.getDstPort()));

    return hash;
}
