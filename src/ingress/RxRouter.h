#pragma once

#include "packet/Packet.h"
#include "packet/PacketParser.h"
#include "session/SessionManager.h"

#include <memory>
#include <cstddef>

class ShardManager;

class Packet;

class RxRouter {
public:
    RxRouter(ShardManager *shardManager, SessionManager *sessionManager);

    ~RxRouter();

    void handlePacket(std::unique_ptr <Packet> packet);

private:
    size_t selectShard(const uint64_t sessionId) const;

    PacketParser m_packetParser;
    ShardManager *m_shardManager;
    SessionManager *m_sessionManager;
};

