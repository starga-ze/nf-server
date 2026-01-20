#pragma once

#include "net/packet/Packet.h"
#include "net/packet/ParsedPacket.h"

#include <optional>
#include <memory>

class PacketParser {
public:
    PacketParser() = default;

    std::optional <ParsedPacket> parse(std::unique_ptr <Packet> packet) const;

private:
    uint64_t fnv1aMix(uint64_t hash, uint64_t value) const;

    uint64_t resolveSessionId(const Packet &packet) const;
};

