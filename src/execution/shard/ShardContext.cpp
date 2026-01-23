#include "execution/shard/ShardContext.h"

#include "execution/login/LoginContext.h"
#include "execution/world/WorldContext.h"
#include "execution/shard/ShardManager.h"

ShardContext::ShardContext(int shardIdx, ShardManager *shardManager, DbManager *dbManager)
        : m_shardIdx(shardIdx),
          m_shardManager(shardManager),
          m_dbManager(dbManager) {
    m_loginContext = std::make_unique<LoginContext>(shardIdx, shardManager, dbManager);
    m_worldContext = std::make_unique<WorldContext>(shardIdx);
}

ShardContext::~ShardContext() {
}

void ShardContext::setTxRouter(TxRouter *txRouter) {
    m_loginContext->setTxRouter(txRouter);
}

LoginContext &ShardContext::loginContext() {
    return *m_loginContext;
}

WorldContext& ShardContext::worldContext() {
    return *m_worldContext;
}
