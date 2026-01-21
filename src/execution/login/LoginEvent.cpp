#include "execution/login/LoginEvent.h"
#include "execution/login/LoginContext.h"

LoginEvent::LoginEvent(
    uint64_t sessionId,
    std::vector<uint8_t> body,
    std::string_view id,
    std::string_view pw)
    : Event(sessionId)
    , m_body(std::move(body))
    , m_id(id)
    , m_pw(pw)
{
}

void LoginEvent::handleEvent(ShardContext& shardContext) {
    shardContext.loginContext().loginReqEvent(*this);
}

