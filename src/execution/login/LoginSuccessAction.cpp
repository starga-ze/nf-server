#include "execution/login/LoginSuccessAction.h"

LoginSuccessAction::LoginSuccessAction(uint64_t sessionId) :
        m_sessionId(sessionId) {
}

void LoginSuccessAction::handleAction(ShardContext &shardContext) {
    shardContext.loginContext().loginSuccessAction(m_sessionId);
}

