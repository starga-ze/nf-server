#include "TxRouter.h"
#include "util/Logger.h"

#include "net/tls/TlsServer.h"
#include "net/tcp/TcpServer.h"
#include "net/udp/UdpServer.h"

TxRouter::TxRouter(TlsServer *tls, TcpServer *tcp, UdpServer *udp, SessionManager *sessionManager)
        : m_tlsServer(tls),
          m_tcpServer(tcp),
          m_udpServer(udp),
          m_sessionManager(sessionManager) {
}

void TxRouter::handlePacket(uint64_t sessionId, Opcode opcode, std::vector<uint8_t> payload) {
    SessionTxSnapshot snap;
    if (not m_sessionManager->getTxSnapshot(sessionId, opcode, snap)) {
        LOG_WARN("Tx snapshot failed, sid={}", sessionId);
        return;
    }

    if (opcode == Opcode::LOGIN_RES_SUCCESS) {
        m_sessionManager->setState(sessionId, SessionState::AUTH);
    }
    else if (opcode == Opcode::LOGIN_RES_FAIL) {
        m_sessionManager->erase(sessionId);
    }

    std::unique_ptr<Packet> packet = m_packetBuilder.build(std::move(payload), snap);
    if (not packet) {
        return;
    }

    LOG_TRACE("TxRouter Dump\n{}", packet->dump());

    const auto protocol = packet->getProtocol();

    switch (protocol) {
        case Protocol::TLS:
            if (m_tlsServer)
                m_tlsServer->enqueueTx(std::move(packet));
            break;

        case Protocol::TCP:
            if (m_tcpServer)
                m_tcpServer->enqueueTx(std::move(packet));
            break;

        case Protocol::UDP:
            if (m_udpServer)
                m_udpServer->enqueueTx(std::move(packet));
            break;

        default:
            LOG_WARN("Unsupported protocol");
            break;
    }
}
