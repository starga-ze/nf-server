#pragma once

#include <memory>
#include <vector>
#include <arpa/inet.h>

class ActionPacket;

class Session;

class Packet;

class PacketBuilder {
public:
    PacketBuilder() = default;

    ~PacketBuilder() = default;

    std::unique_ptr <Packet> build(const std::vector<uint8_t> payload, const Session &session);

private:
    static sockaddr_in createSockAddr(uint32_t ip, uint16_t port);
};
