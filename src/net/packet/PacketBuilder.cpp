#include "PacketBuilder.h"
#include "net/packet/Packet.h"
#include "session/Session.h"
#include "util/Logger.h"

#include <cstring>


sockaddr_in PacketBuilder::createSockAddr(uint32_t ip, uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ip);
    addr.sin_port = htons(port);
    return addr;
}

std::unique_ptr <Packet> PacketBuilder::build(std::vector<uint8_t> payload, const Session &session) {
    const Protocol protocol = session.getProtocol();

    switch (protocol) {
        case Protocol::TCP:
        case Protocol::TLS: {
            const ConnInfo &connInfo = session.getConnInfo();

            sockaddr_in srcAddr = createSockAddr(connInfo.dstIp, connInfo.dstPort);
            sockaddr_in dstAddr = createSockAddr(connInfo.srcIp, connInfo.srcPort);

            return std::make_unique<Packet>(
                    session.getFd(),
                    protocol,
                    std::move(payload),
                    srcAddr,
                    dstAddr
            );
        }
            /*
            case Protocol::UDP:
            {
                return std::make_unique<Packet>(
                    session.getFd(),
                    session.getProtocol(),
                    std::move(payload),
                    session.getDstAddr(),
                    session.getDstLen()
                );
            }
            */
        default:
            LOG_WARN("PacketBuilder: unsupported protocol");
            return nullptr;
    }
}


