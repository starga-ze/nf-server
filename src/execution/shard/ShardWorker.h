#pragma once

#include "execution/shard/ShardContext.h"
#include "db/DbManager.h"
#include "execution/Action.h"
#include "execution/Event.h"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <memory>

class ShardWorker {
public:
    ShardWorker(size_t shardIdx, ShardManager *shardManager, DbManager *dbManager);

    ~ShardWorker() = default;

    void stop();

    void setTxRouter(TxRouter *txRouter);

    void processPacket();

    void enqueueEvent(std::unique_ptr <Event> pkt);

    void enqueueAction(std::unique_ptr <Action> action);

    ShardContext &shardContext() { return *m_shardContext; }

private:
    void onTick();

    std::unique_ptr <ShardContext> m_shardContext;

    std::atomic<bool> m_running{false};

    /* event & action */
    std::queue <std::unique_ptr<Event>> m_eventQueue;
    std::mutex m_eventLock;

    std::mutex m_actionLock;
    std::queue <std::unique_ptr<Action>> m_actionQueue;

    std::condition_variable m_cv;

    std::chrono::milliseconds m_tickInterval{3000};
    std::chrono::steady_clock::time_point m_nextTick;

    size_t m_shardIdx;
};

