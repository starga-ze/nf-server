#include "execution/login/LoginAction.h"
#include "execution/login/LoginContext.h"

LoginSuccessAction::LoginSuccessAction(uint64_t sessionId) :
        m_sessionId(sessionId) {
}

void LoginSuccessAction::handleAction(ShardContext &shardContext) {
    shardContext.loginContext().loginSuccessAction(m_sessionId);
}

LoginFailAction::LoginFailAction(uint64_t sessionId) :
        m_sessionId(sessionId) {

}

void LoginFailAction::handleAction(ShardContext &shardContext) {
    shardContext.loginContext().loginFailAction(m_sessionId);
}
