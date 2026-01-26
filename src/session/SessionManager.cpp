#include "SessionManager.h"
#include "util/Logger.h"

#include <sstream>
#include <iomanip>
#include <openssl/rand.h>

SessionManager::SessionManager() = default;

void SessionManager::start() {
    m_running.store(true, std::memory_order_release);

    while(m_running.load(std::memory_order_acquire)) {
        dump();
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

void SessionManager::stop() {
    m_running.store(false, std::memory_order_release);
}


bool SessionManager::checkAndBind(const ParsedPacket& parsed)
{
    const uint64_t sessionId = parsed.getSessionId();

    if (sessionId == 0) {
        LOG_WARN("Invalid sessionId=0");
        return false;
    }

    std::lock_guard<std::mutex> lock(m_lock);

    auto it = m_sessions.find(sessionId);

    if (it == m_sessions.end()) {
        LOG_WARN("Session not found, sid={}", sessionId);
        return false;
    }

    it->second->bind(parsed);
    return true;
}

bool SessionManager::create(ParsedPacket& parsed)
{
    const int fd = parsed.getFd();

    std::lock_guard<std::mutex> lock(m_lock);

    
    if (m_tlsFdToSessionId.count(fd) != 0) {
        LOG_WARN("Duplicate LOGIN_REQ, fd={}", fd);
        return false;
    }

    uint64_t sessionId = generateSecureSessionId();

    auto session = std::make_unique<Session>(sessionId);
    session->bind(parsed);
    session->setState(SessionState::PRE_AUTH);

    m_sessions.emplace(sessionId, std::move(session));
    m_tlsFdToSessionId.emplace(fd, sessionId);

    parsed.setSessionId(sessionId);

    LOG_INFO("Create session sid={}, fd={}", sessionId, fd);
    return true;
}

void SessionManager::erase(uint64_t sessionId)
{
    std::lock_guard<std::mutex> lock(m_lock);

    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return;
    }

    int fd = it->second->getTlsFd();
    
    m_tlsFdToSessionId.erase(fd);
    m_sessions.erase(sessionId);
}

bool SessionManager::getTxSnapshot(uint64_t sessionId, Opcode opcode, SessionTxSnapshot& out)
{
    std::lock_guard<std::mutex> lock(m_lock);

    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return false;
    }

    const Session& s = *it->second;

    switch(opcode) {
        case Opcode::LOGIN_RES_SUCCESS:
        case Opcode::LOGIN_RES_FAIL:
            out.protocol = Protocol::TLS;
            break;

        default:
            out.protocol = Protocol::UNKNOWN;
            return false;
    }

    out.tlsFd    = s.getTlsFd();
    out.tcpFd    = s.getTcpFd();
    out.udpFd    = s.getUdpFd();
    out.connInfo = s.getConnInfo();

    return true;
}

void SessionManager::setState(uint64_t sessionId, SessionState state)
{
    std::lock_guard<std::mutex> lock(m_lock);

    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        LOG_WARN("setState failed: session not found sid={}", sessionId);
        return;
    }

    it->second->setState(state);
}

uint64_t SessionManager::generateSecureSessionId()
{
    uint64_t sid = 0;

    if (RAND_bytes(reinterpret_cast<unsigned char*>(&sid), sizeof(sid)) != 1) {
        LOG_FATAL("RAND_bytes failed");
        std::abort();
    }

    if (sid == 0) {
        return generateSecureSessionId();
    }

    return sid;
}

void SessionManager::dump()
{
    std::lock_guard<std::mutex> lock(m_lock);

    std::ostringstream oss;

    constexpr int SID_W   = 22;
    constexpr int STATE_W = 10;
    constexpr int FD_W    = 8;

    oss << "[SESSION]\n";
    oss << std::left
        << std::setw(SID_W)   << "SID"
        << std::setw(STATE_W) << "STATE"
        << std::setw(FD_W)    << "TLS_FD"
        << std::setw(FD_W)    << "TCP_FD"
        << std::setw(FD_W)    << "UDP_FD"
        << "\n";

    oss << std::string(SID_W + STATE_W + FD_W * 3, '=') << "\n";

    if (m_sessions.empty()) {
        oss << "(no sessions)\n";
    }

    else {
        for (const auto& [sid, session] : m_sessions) {
            oss << std::left
                << std::setw(SID_W)   << sid
                << std::setw(STATE_W) << stateToStr(session->getState())
                << std::setw(FD_W)    << session->getTlsFd()
                << std::setw(FD_W)    << session->getTcpFd()
                << std::setw(FD_W)    << session->getUdpFd()
                << "\n";
        }
    }

    LOG_TRACE("Session Table Dump\n{}", oss.str());
}

const char* SessionManager::stateToStr(SessionState s) {
    switch (s) {
    case SessionState::PRE_AUTH: return "PRE_AUTH";
    case SessionState::AUTH:     return "AUTH";
    case SessionState::IN_WORLD: return "WORLD";
    case SessionState::CLOSED:   return "CLOSE";
    default:                     return "UNKNOWN";
    }
}

