#pragma once

#include "packet/Packet.h"
#include "packet/ParsedPacket.h"

#include <optional>
#include <memory>

class PacketParser {
public:
    PacketParser() = default;

    std::optional <ParsedPacket> parse(std::unique_ptr <Packet> packet) const;
};

