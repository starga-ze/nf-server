#pragma once

#include <cstdint>

class ShardContext;

class Event {
public:
    virtual ~Event() = default;

    virtual void handleEvent(ShardContext &shardContext) = 0;
};

