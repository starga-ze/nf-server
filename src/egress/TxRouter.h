#pragma once

#include "session/SessionManager.h"
#include "net/packet/Packet.h"
#include "net/packet/PacketBuilder.h"

#include <memory>
#include <cstdint>

class TlsServer;

class TcpServer;

class UdpServer;

class TxRouter {
public:
    TxRouter(TlsServer *tls, TcpServer *tcp, UdpServer *udp, SessionManager *sessionManager);

    void handlePacket(uint64_t sessionId, Opcode opcode, std::vector<uint8_t> payload);

private:
    PacketBuilder m_packetBuilder;
    TlsServer *m_tlsServer;
    TcpServer *m_tcpServer;
    UdpServer *m_udpServer;
    SessionManager *m_sessionManager;
};
