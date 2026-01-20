#pragma once

#include "execution/Action.h"
#include "execution/shard/ShardContext.h"

#include <cstdint>

class LoginSuccessAction final : public Action {
public:
    explicit LoginSuccessAction(uint64_t sessionId);

    void handleAction(ShardContext &shardContext) override;

private:
    uint64_t m_sessionId;
};

