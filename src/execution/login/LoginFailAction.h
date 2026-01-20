#pragma once

#include "execution/Action.h"
#include "execution/shard/ShardContext.h"

#include <cstdint>

class LoginFailAction final : public Action {
public:
    explicit LoginFailAction(uint64_t sessionId);

    void handleAction(ShardContext &shardContext) override;

private:
    uint64_t m_sessionId;
};
