#include "ShardWorker.h"
#include "ShardManager.h"
#include "util/Logger.h"

#include "execution/world/WorldContext.h"

ShardWorker::ShardWorker(size_t shardIdx, ShardManager *shardManager, DbManager *dbManager)
        : m_shardIdx(shardIdx) 
{
    m_shardContext = std::make_unique<ShardContext>(m_shardIdx, shardManager, dbManager);
}

void ShardWorker::processPacket() {
    m_running.store(true, std::memory_order_release);

    auto now = std::chrono::steady_clock::now();
    m_startTime     = now;
    m_lastTickTime  = now;
    m_nextTick      = std::chrono::steady_clock::now() + m_tickInterval;

    while (m_running.load(std::memory_order_acquire)) {
        std::unique_ptr <Event> event;

        {
            std::unique_lock <std::mutex> lock(m_eventLock);

            // wait until next tick OR event notify
            m_cv.wait_until(lock, m_nextTick);

            if (!m_running.load(std::memory_order_acquire))
                break;

            if (!m_eventQueue.empty()) {
                event = std::move(m_eventQueue.front());
                m_eventQueue.pop();
            }
        }

        now = std::chrono::steady_clock::now();

        // ---- event handling ----
        if (event) 
        {
            LOG_DEBUG("Shard idx:{}, handle event", m_shardIdx);
            event->handleEvent(*m_shardContext);
        }

        // ---- action handling ---- 
        {
            std::queue <std::unique_ptr<Action>> snapshot;
            {
                std::lock_guard <std::mutex> lock(m_actionLock);
                std::swap(snapshot, m_actionQueue);
            }

            while (not snapshot.empty()) {
                LOG_DEBUG("Shard idx:{}, handle action", m_shardIdx);
                snapshot.front()->handleAction(*m_shardContext);
                snapshot.pop();
            }
        }

        // ---- tick handling ----
        if (now >= m_nextTick) {
            onTick(now);

            m_nextTick += m_tickInterval;

            if (now > m_nextTick + m_tickInterval) {
                LOG_WARN("Shard idx:{}, tick drift detected, resync", m_shardIdx);
                m_nextTick = now + m_tickInterval;
            }
        }
    }
}

void ShardWorker::onTick(std::chrono::steady_clock::time_point now) {
    const auto deltaMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTickTime).count();
    const auto elapsedSinceStartMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime).count();

    m_lastTickTime = now;
    ++m_tickCount;

    // LOG_TRACE("Shard idx:{}, TICK #{} | delta={}ms | elapsed={}ms", m_shardIdx, m_tickCount, deltaMs, elapsedSinceStartMs);

    m_shardContext->worldContext().tick(deltaMs);
}

void ShardWorker::stop() {
    m_running.store(false, std::memory_order_release);
    m_cv.notify_one();
}

void ShardWorker::enqueueEvent(std::unique_ptr <Event> event) {
    {
        std::lock_guard <std::mutex> lock(m_eventLock);
        m_eventQueue.push(std::move(event));
    }
    m_cv.notify_one();
}

void ShardWorker::enqueueAction(std::unique_ptr <Action> action) {
    if (not action) {
        return;
    }

    {
        std::lock_guard <std::mutex> lock(m_actionLock);
        m_actionQueue.push(std::move(action));
    }
    m_cv.notify_one();
}

void ShardWorker::setTxRouter(TxRouter *txRouter) {
    m_shardContext->setTxRouter(txRouter);
}
