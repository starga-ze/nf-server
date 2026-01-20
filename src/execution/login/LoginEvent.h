#pragma once

#include "execution/Event.h"
#include "net/packet/EventPacket.h"
#include "execution/shard/ShardContext.h"

#include <memory>

class LoginEvent final : public Event {
public:
    explicit LoginEvent(std::unique_ptr <EventPacket> pkt);

    void handleEvent(ShardContext &shardContext) override;

private:
    std::unique_ptr <EventPacket> m_pkt;
};

