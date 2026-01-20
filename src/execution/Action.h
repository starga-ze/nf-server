#pragma once

class ShardContext;

class Action {
public:
    virtual ~Action() = default;

    virtual void handleAction(ShardContext &shardContext) = 0;
};

