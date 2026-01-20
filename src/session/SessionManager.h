#pragma once

#include "session/Session.h"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <mutex>

class SessionManager {
public:
    SessionManager();

    Session *find(const uint64_t sessionId);

    Session *create(const uint64_t sessionId);

private:
    std::mutex m_lock;
    std::unordered_map <uint64_t, std::unique_ptr<Session>> m_sessions;
};
