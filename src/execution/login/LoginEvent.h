#pragma once

#include "execution/Event.h"
#include "execution/shard/ShardContext.h"

#include <string_view>
#include <vector>

class LoginEvent : public Event 
{
protected:
    LoginEvent(uint64_t sessionId);
};

class LoginReqEvent final : public LoginEvent 
{
public:
    LoginReqEvent(
        uint64_t sessionId,
        std::vector<uint8_t> payload,
        std::string_view id,
        std::string_view pw
    );

    void handleEvent(ShardContext& shardContext) override;

    std::string_view id() const { return m_id; }
    std::string_view pw() const { return m_pw; }
    

private:
    std::vector<uint8_t> m_payload;
    std::string_view    m_id;
    std::string_view    m_pw;
};

