#pragma once

#include "db/DbManager.h"

#include <memory>

class ShardManager;
class LoginContext;
class TxRouter;

class ShardContext {
public:
    ShardContext(int shardIdx, ShardManager *shardManager, DbManager *dbManager);

    ~ShardContext();

    LoginContext &loginContext();

    void setTxRouter(TxRouter *txRouter);

private:
    ShardManager *m_shardManager;
    DbManager *m_dbManager;

    std::unique_ptr <LoginContext> m_loginContext;

    int m_shardIdx;
};

