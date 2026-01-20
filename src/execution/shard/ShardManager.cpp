#include "ShardManager.h"

#include "util/Logger.h"
#include "util/ThreadManager.h"
#include "db/DbManager.h"
#include "execution/shard/ShardWorker.h"
#include "execution/Event.h"
#include "execution/Action.h"

#include <thread>

ShardManager::ShardManager(size_t workerCount, ThreadManager *threadManager, DbManager *dbManager)
        : m_workerCount(workerCount),
          m_threadManager(threadManager),
          m_dbManager(dbManager) {
    initWorkers();
}

ShardManager::~ShardManager() {
    stop();
}

void ShardManager::initWorkers() {
    m_workers.resize(m_workerCount);

    for (size_t i = 0; i < m_workerCount; ++i) {
        if (not m_dbManager) {
            LOG_TRACE("DB manager is nullptr");
        }
        m_workers[i] = std::make_unique<ShardWorker>(i, this, m_dbManager);
    }
}

void ShardManager::start() {
    m_running.store(true, std::memory_order_release);

    startWorkers();

    // manager thread does nothing
    while (m_running.load(std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void ShardManager::stop() {
    m_running.store(false, std::memory_order_release);
}

void ShardManager::startWorkers() {
    m_workers.resize(m_workerCount);

    for (size_t i = 0; i < m_workerCount; ++i) {
        m_threadManager->addThread(
                "shard_worker_" + std::to_string(i),
                std::bind(&ShardWorker::processPacket, m_workers[i].get()),
                std::bind(&ShardWorker::stop, m_workers[i].get())
        );
    }
}

void ShardManager::dispatch(size_t shardIdx, std::unique_ptr <Event> event) {
    if (not event || shardIdx >= m_workers.size()) {
        return;
    }

    m_workers[shardIdx]->enqueueEvent(std::move(event));
}

void ShardManager::commit(size_t shardIdx, std::unique_ptr <Action> action) {
    if (not action || shardIdx >= m_workers.size()) {
        return;
    }
    m_workers[shardIdx]->enqueueAction(std::move(action));
}

size_t ShardManager::getWorkerCount() const {
    return m_workerCount;
}

ShardWorker *ShardManager::getWorker(size_t shardIdx) const {
    if (shardIdx >= m_workers.size()) {
        LOG_WARN("invalid shardIdx={}, workers={}", shardIdx, m_workers.size());
        return nullptr;
    }

    return m_workers[shardIdx].get();
}
