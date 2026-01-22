#include "PacketBuilder.h"
#include "net/packet/Packet.h"
#include "session/SessionManager.h"
#include "util/Logger.h"

#include <cstring>

sockaddr_in PacketBuilder::createSockAddr(uint32_t ip, uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ip);
    addr.sin_port = htons(port);
    return addr;
}

std::unique_ptr <Packet> PacketBuilder::build(std::vector<uint8_t> payload, const SessionTxSnapshot& snap) {
    const ConnInfo& ci = snap.connInfo;

    // 논리적 src / dst addr 구성 (TCP/TLS/UDP 공통)
    sockaddr_in srcAddr = createSockAddr(ci.srcIp, ci.srcPort);
    sockaddr_in dstAddr = createSockAddr(ci.dstIp, ci.dstPort);

    switch (snap.protocol) {

    case Protocol::TCP:
    case Protocol::TLS:
        return std::make_unique<Packet>(
            snap.tcpFd,
            snap.protocol,
            std::move(payload),
            srcAddr,
            dstAddr
        );

    case Protocol::UDP:
        return std::make_unique<Packet>(
            snap.udpFd,
            Protocol::UDP,
            std::move(payload),
            srcAddr,   // ★ 논리 정보로 유지
            dstAddr
        );

    default:
        LOG_WARN("PacketBuilder: unsupported protocol");
        return nullptr;
    }

}


