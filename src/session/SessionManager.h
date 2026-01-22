#pragma once

#include "session/Session.h"
#include "net/packet/ParsedPacket.h"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>

struct SessionTxSnapshot {
    Protocol protocol{Protocol::UNKNOWN};
    int tcpFd{-1};
    int udpFd{-1};
    ConnInfo connInfo{};
};


class SessionManager {
public:
    SessionManager();

    void start();

    void stop();

    bool checkAndBind(const ParsedPacket& parsed);

    void erase(uint64_t sessionId);

    void setState(uint64_t sessionId, SessionState state);

    bool getTxSnapshot(uint64_t sessionId, Opcode opcode, SessionTxSnapshot& out);

private:
    void dump();
    static const char* stateToStr(SessionState s);
    std::atomic<bool> m_running {false};

    std::mutex m_lock;
    std::unordered_map <uint64_t, std::unique_ptr<Session>> m_sessions;
};
