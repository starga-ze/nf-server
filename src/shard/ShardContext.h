#pragma once

#include "db/DbManager.h"

#include <memory>

class ShardManager;
class TxRouter;
class DbManager;

class LoginContext;
class WorldContext;


class ShardContext {
public:
    ShardContext(int shardIdx, ShardManager *shardManager, DbManager *dbManager);

    ~ShardContext();

    LoginContext& loginContext();
    WorldContext& worldContext();

    void setTxRouter(TxRouter *txRouter);

private:
    ShardManager *m_shardManager;
    DbManager *m_dbManager;

    std::unique_ptr <LoginContext> m_loginContext;
    std::unique_ptr <WorldContext> m_worldContext;

    int m_shardIdx;
};

