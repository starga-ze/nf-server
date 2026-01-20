#include "execution/shard/ShardContext.h"

#include "execution/login/LoginContext.h"
#include "execution/shard/ShardManager.h"

ShardContext::ShardContext(int shardIdx, ShardManager *shardManager, DbManager *dbManager)
        : m_shardIdx(shardIdx),
          m_shardManager(shardManager),
          m_dbManager(dbManager) {
    m_loginContext = std::make_unique<LoginContext>(shardIdx, shardManager, dbManager);
}

ShardContext::~ShardContext() {
}

void ShardContext::setTxRouter(TxRouter *txRouter) {
    m_loginContext->setTxRouter(txRouter);
}

LoginContext &ShardContext::loginContext() {
    return *m_loginContext;
}

