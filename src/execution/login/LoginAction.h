#pragma once

#include "execution/Action.h"
#include "execution/shard/ShardContext.h"

#include <cstdint>

class LoginAction : public Action {
public:
    LoginAction() = default;
    virtual ~LoginAction() = default;

};


class LoginSuccessAction final : public LoginAction {
public:
    LoginSuccessAction(uint64_t sessionId, Opcode opcode, std::vector<uint8_t> payload);

    void handleAction(ShardContext &shardContext) override;

    const std::vector<uint8_t> takePayload() { return std::move(m_payload); }

    uint64_t sessionId() const { return m_sessionId; }

    Opcode opcode() const { return m_opcode; }

private:
    Opcode m_opcode;
    uint64_t m_sessionId;
    std::vector<uint8_t> m_payload;
};


class LoginFailAction final : public LoginAction {
public:
    LoginFailAction(uint64_t sessionId, Opcode opcode, std::vector<uint8_t> payload);

    void handleAction(ShardContext &shardContext) override;

    const std::vector<uint8_t> takePayload() { return std::move(m_payload); }

    uint64_t sessionId() const { return m_sessionId; }

    Opcode opcode() const { return m_opcode; }

private:
    Opcode m_opcode;
    uint64_t m_sessionId;
    std::vector<uint8_t> m_payload;
};



