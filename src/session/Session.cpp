#include "Session.h"

Session::Session(uint64_t sessionId)
        : m_sessionId(SessionId{sessionId}) {
}

SessionId Session::getSessionId() const {
    return m_sessionId;
}

SessionState Session::getState() const {
    return m_state;
}

int Session::getFd() const {
    return m_fd;
}

Protocol Session::getProtocol() const {
    return m_protocol;
}

void Session::setState(SessionState state) {
    m_state = state;
}

void Session::bindTcp(const ConnInfo &connInfo, int fd) {
    m_fd = fd;
    m_connInfo = connInfo;
    m_srcIp = connInfo.srcIp;
    m_dstIp = connInfo.dstIp;
    m_protocol = connInfo.protocol;
}

const ConnInfo &Session::getConnInfo() const {
    return m_connInfo;
}
