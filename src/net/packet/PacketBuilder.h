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

    std::unique_ptr <Packet> build(std::unique_ptr <ActionPacket> actionPacket, const Session &session);

private:
    static sockaddr_in createSockAddr(uint32_t ip, uint16_t port);

    std::unique_ptr <Packet> buildLoginRes(const ActionPacket &action, const Session &session);

    std::vector <uint8_t> buildLoginResPayload(bool success);
};

