#include "execution/login/LoginAction.h"
#include "execution/login/LoginContext.h"

LoginSuccessAction::LoginSuccessAction(uint64_t sessionId, std::vector<uint8_t> payload) : 
    m_sessionId(sessionId),
    m_payload(std::move(payload))
{
}

void LoginSuccessAction::handleAction(ShardContext &shardContext) {
    shardContext.loginContext().loginSuccessAction(*this);
}

LoginFailAction::LoginFailAction(uint64_t sessionId, std::vector<uint8_t> payload) : 
    m_sessionId(sessionId),
    m_payload(std::move(payload))
{

}

void LoginFailAction::handleAction(ShardContext &shardContext) {
    shardContext.loginContext().loginFailAction(*this);
}
