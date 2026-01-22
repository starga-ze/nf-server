#pragma once

#include "net/packet/ParsedPacketTypes.h"

class ShardContext;

class Action {
public:
    Action() = default;
    virtual ~Action() = default;

    virtual void handleAction(ShardContext &shardContext) = 0;
};

