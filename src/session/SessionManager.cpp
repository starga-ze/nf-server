#include "SessionManager.h"
#include "util/Logger.h"

SessionManager::SessionManager() = default;

Session *SessionManager::find(uint64_t sessionId) {
    std::lock_guard <std::mutex> lock(m_lock);

    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return nullptr;
    }

    return it->second.get();
}

Session *SessionManager::create(uint64_t sessionId) {
    std::lock_guard <std::mutex> lock(m_lock);

    auto it = m_sessions.find(sessionId);
    if (it != m_sessions.end()) {
        return it->second.get();
    }

    auto owned = std::make_unique<Session>(sessionId);
    Session *raw = owned.get();

    m_sessions.emplace(sessionId, std::move(owned));
    return raw;
}

