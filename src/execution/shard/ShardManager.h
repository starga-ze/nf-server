#pragma once

#include <vector>
#include <memory>
#include <atomic>

class ThreadManager;

class DbManager;

class ShardWorker;

class Event;

class Action;

class ShardManager {
public:
    ShardManager(size_t workerCount, ThreadManager *threadManager, DbManager *dbmanager);

    ~ShardManager();

    void start();

    void stop();

    void dispatch(size_t shardIdx, std::unique_ptr <Event> event);

    void commit(size_t shardIdx, std::unique_ptr <Action> action);

    size_t getWorkerCount() const;

    ShardWorker *getWorker(size_t shardIdx) const;

private:
    void initWorkers();

    void startWorkers();

    size_t m_workerCount;
    ThreadManager *m_threadManager;
    DbManager *m_dbManager;

    std::atomic<bool> m_running{false};

    std::vector <std::unique_ptr<ShardWorker>> m_workers;
};

