#include "PacketBuilder.h"
#include "net/packet/Packet.h"
#include "net/packet/ActionPacket.h"
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


std::unique_ptr <Packet> PacketBuilder::build(std::unique_ptr <ActionPacket> actionPacket,
                                              const Session &session) {
    if (!actionPacket)
        return nullptr;

    const Opcode opcode = actionPacket->getOpcode();

    switch (opcode) {
        case Opcode::LOGIN_RES_SUCCESS:
        case Opcode::LOGIN_RES_FAIL:
            return buildLoginRes(*actionPacket, session);

        default:
            LOG_WARN("PacketBuilder: unhandled opcode {}",
                     static_cast<int>(opcode));
            return nullptr;
    }
}

std::unique_ptr <Packet> PacketBuilder::buildLoginRes(const ActionPacket &action,
                                                      const Session &session) {
    const bool success =
            (action.getOpcode() == Opcode::LOGIN_RES_SUCCESS);

    auto payload = buildLoginResPayload(success);

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

std::vector <uint8_t> PacketBuilder::buildLoginResPayload(bool success) {
    const uint16_t bodyLenHost = sizeof(uint8_t);
    const uint32_t flagsHost = 0;

    CommonPacketHeader header;
    header.version = PacketVersion::V1;
    header.opcode = Opcode::LOGIN_RES_SUCCESS;
    header.bodyLen = htons(bodyLenHost);
    header.flags = htonl(flagsHost);

    std::vector <uint8_t> payload;
    payload.resize(sizeof(CommonPacketHeader) + bodyLenHost);

    std::memcpy(payload.data(), &header, sizeof(CommonPacketHeader));
    payload[sizeof(CommonPacketHeader)] = success ? 0x01 : 0x00;

    return payload;
}
