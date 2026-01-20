#include "execution/login/LoginFailAction.h"

LoginFailAction::LoginFailAction(uint64_t sessionId) :
        m_sessionId(sessionId) {

}

void LoginFailAction::handleAction(ShardContext &shardContext) {
    shardContext.loginContext().loginFailAction(m_sessionId);
}
