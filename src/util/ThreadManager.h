#pragma once

#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>

struct ThreadInfo {
    std::string name;
    std::thread thread;
    std::function<void()> stopFunc;
};

class ThreadManager {
public:
    ThreadManager() = default;

    ~ThreadManager();

    static bool setName(const std::string &name);

    void addThread(const std::string &name, std::function<void()> originFunc,
                   std::function<void()> stopFunc);

    void stopAll();

private:
    void threadWrapper(const std::string &name, std::function<void()> func);

    std::vector <ThreadInfo> m_threadList;
    std::mutex m_mtx;
};
