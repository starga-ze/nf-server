#pragma once

#include "egress/TxRouter.h"
#include "net/packet/ParsedPacket.h"
#include "db/DbManager.h"
#include "execution/login/LoginAction.h"
#include "execution/login/LoginEvent.h"


#include <memory>
#include <cstdint>

class ShardManager;
class LoginReqEvent;
class LoginFailAction;
class LoginSuccessAction;

class LoginContext {
public:
    LoginContext(int shardIdx, ShardManager *shardManager, DbManager *dbManger);

    ~LoginContext() = default;

    void loginReqEvent(const LoginReqEvent& ev);

    void loginSuccessAction(LoginSuccessAction& ac);

    void loginFailAction(LoginFailAction& ac);

    void setTxRouter(TxRouter *txRouter);

private:
    bool verify(const std::string &id, const std::string &pw);

    ShardManager *m_shardManager;
    DbManager *m_dbManager;
    TxRouter *m_txRouter;

    bool m_enableDb;
    int m_shardIdx;
};

