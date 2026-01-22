#include "SessionManager.h"
#include "util/Logger.h"

#include <sstream>
#include <iomanip>

SessionManager::SessionManager() = default;

void SessionManager::start() {
    m_running.store(true, std::memory_order_release);

    while(m_running.load(std::memory_order_acquire)) {
        dump();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void SessionManager::stop() {
    m_running.store(false, std::memory_order_release);
}


bool SessionManager::checkAndBind(const ParsedPacket& parsed)
{
    const uint64_t sessionId = parsed.getSessionId();
    const Opcode opcode = parsed.opcode();

    if (sessionId == 0) {
        LOG_WARN("Invalid sessionId=0");
        return false;
    }

    std::lock_guard<std::mutex> lock(m_lock);

    auto it = m_sessions.find(sessionId);

    if (it == m_sessions.end()) {
        if (opcode != Opcode::LOGIN_REQ) {
            LOG_WARN("Session not found for non-login packet, sid={}", sessionId);
            return false;
        }

        auto session = std::make_unique<Session>(sessionId);
        session->bind(parsed);

        m_sessions.emplace(sessionId, std::move(session));
        LOG_DEBUG("Create pre-session sid={}", sessionId);
        return true;
    }

    it->second->bind(parsed);
    return true;
}

void SessionManager::erase(uint64_t sessionId)
{
    std::lock_guard<std::mutex> lock(m_lock);
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

    out.tcpFd    = s.getTcpFd();
    out.udpFd    = s.getUdpFd();
    out.connInfo = s.getConnInfo(); // 구조체 복사

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

void SessionManager::dump()
{
    std::lock_guard<std::mutex> lock(m_lock);

    std::ostringstream oss;

    constexpr int SID_W   = 22;
    constexpr int STATE_W = 10;
    constexpr int FD_W    = 8;

    oss << "\n[SESSION]\n";
    oss << std::left
        << std::setw(SID_W)   << "SID"
        << std::setw(STATE_W) << "STATE"
        << std::setw(FD_W)    << "TCP_FD"
        << std::setw(FD_W)    << "UDP_FD"
        << "\n";

    oss << std::string(SID_W + STATE_W + FD_W * 2, '=') << "\n";

    if (m_sessions.empty()) {
        oss << "(no sessions)\n";
        LOG_INFO("{}", oss.str());
        return;
    }

    for (const auto& [sid, session] : m_sessions) {
        oss << std::left
            << std::setw(SID_W)   << sid
            << std::setw(STATE_W) << stateToStr(session->getState())
            << std::setw(FD_W)    << session->getTcpFd()
            << std::setw(FD_W)    << session->getUdpFd()
            << "\n";
    }

    LOG_TRACE("{}", oss.str());
}

const char* SessionManager::stateToStr(SessionState s) {
    switch (s) {
    case SessionState::PRE_AUTH: return "PRE";
    case SessionState::AUTH:     return "AUTH";
    case SessionState::IN_WORLD: return "WORLD";
    case SessionState::CLOSED:   return "CLOSE";
    default:                     return "UNK";
    }
}

