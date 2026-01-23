#include "ThreadManager.h"
#include "Logger.h"

#include <sys/prctl.h>
#include <cstring>
#include <unistd.h>

ThreadManager::~ThreadManager() {
    stopAll();
}


bool ThreadManager::setName(const std::string &name) {
    char thread_name[16];
    std::strncpy(thread_name, name.c_str(), sizeof(thread_name) - 1);
    thread_name[sizeof(thread_name) - 1] = '\0';

    if (prctl(PR_SET_NAME, thread_name, 0, 0, 0) != 0) {
        LOG_ERROR("Error setting thread name to {}", thread_name);
        return false;
    }
    return true;
}

void ThreadManager::addThread(const std::string &name, std::function<void()> originFunc,
                              std::function<void()> stopFunc) {
    /* Wrap for prctl */
    auto wrappedFunc = std::bind(&ThreadManager::threadWrapper, this, name, originFunc);

    ThreadInfo tInfo;
    tInfo.name = name;
    tInfo.stopFunc = stopFunc;

    /* Start the thread with the wrapped function */
    tInfo.thread = std::thread(wrappedFunc);

    {
        std::lock_guard <std::mutex> lock(m_mtx);
        m_threadList.push_back(std::move(tInfo));
    }
}

void ThreadManager::threadWrapper(const std::string &name, std::function<void()> func) {
    ThreadManager::setName(name);
    func();
}

void ThreadManager::stopAll() {
    std::vector <ThreadInfo> snapshot;
    {
        std::lock_guard <std::mutex> lock(m_mtx);

        if (m_threadList.empty()) {
            LOG_TRACE("Thread list is empty.");
            return;
        }

        snapshot = std::move(m_threadList);
        m_threadList.clear();
    }

    for (auto &tInfo: snapshot) {
        LOG_TRACE("Stopping thread '{}'", tInfo.name);
        tInfo.stopFunc();
    }

    auto self = std::this_thread::get_id();
    for (auto &tInfo: snapshot) {
        if (tInfo.thread.joinable()) {
            if (tInfo.thread.get_id() == self) {
                LOG_WARN("Skip joining self thread: {}", tInfo.name);
                continue;
            }
            tInfo.thread.join();
        }
    }
}

