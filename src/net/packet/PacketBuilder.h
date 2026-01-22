#pragma once

#include <memory>
#include <vector>
#include <arpa/inet.h>

class ActionPacket;

struct SessionTxSnapshot;

class Packet;

class PacketBuilder {
public:
    PacketBuilder() = default;

    ~PacketBuilder() = default;

    std::unique_ptr <Packet> build(std::vector<uint8_t> payload, const SessionTxSnapshot& snap);

private:
    static sockaddr_in createSockAddr(uint32_t ip, uint16_t port);
};
