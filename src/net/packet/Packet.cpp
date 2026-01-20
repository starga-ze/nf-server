#include "Packet.h"
#include "util/Logger.h"

#include <sstream>
#include <arpa/inet.h>
#include <iomanip>
#include <algorithm>

Packet::Packet(int fd,
               Protocol proto,
               std::vector <uint8_t> payload,
               const sockaddr_in &srcAddr,
               const sockaddr_in &dstAddr) :
        m_fd(fd),
        m_payload(std::move(payload)) {
    m_connInfo.protocol = proto;
    m_connInfo.srcIp = ntohl(srcAddr.sin_addr.s_addr);
    m_connInfo.srcPort = ntohs(srcAddr.sin_port);
    m_connInfo.dstIp = ntohl(dstAddr.sin_addr.s_addr);
    m_connInfo.dstPort = ntohs(dstAddr.sin_port);
}

Packet::~Packet() {
}

std::string Packet::dump() const {
    std::ostringstream oss;

    auto protoToStr = [](Protocol p) {
        switch (p) {
            case Protocol::TCP:
                return "TCP";
            case Protocol::UDP:
                return "UDP";
            case Protocol::TLS:
                return "TLS";
            default:
                return "UNKNOWN";
        }
    };

    auto ipToStr = [](uint32_t ip) {
        in_addr addr{};
        addr.s_addr = htonl(ip);

        char buf[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &addr, buf, sizeof(buf));
        return std::string(buf);
    };

    oss << "[PACKET]\n";
    oss << "  Protocol : " << protoToStr(m_connInfo.protocol) << '\n';
    oss << "  FD       : " << m_fd << '\n';

    oss << "  Src      : "
        << ipToStr(m_connInfo.srcIp)
        << ":" << m_connInfo.srcPort << '\n';

    oss << "  Dst      : "
        << ipToStr(m_connInfo.dstIp)
        << ":" << m_connInfo.dstPort << '\n';

    oss << "  Payload  : " << m_payload.size() << " bytes\n";

    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < m_payload.size(); ++i) {
        if (i % 16 == 0)
            oss << "    ";

        oss << std::setw(2)
            << static_cast<int>(m_payload[i]) << ' ';

        if (i % 16 == 15 || i + 1 == m_payload.size())
            oss << '\n';
    }

    return oss.str();
}

const ConnInfo &Packet::getConnInfo() const {
    return m_connInfo;
}

Protocol Packet::getProtocol() const {
    return m_connInfo.protocol;
}

uint32_t Packet::getSrcIp() const {
    return m_connInfo.srcIp;
}

uint16_t Packet::getSrcPort() const {
    return m_connInfo.srcPort;
}

uint32_t Packet::getDstIp() const {
    return m_connInfo.dstIp;
}

uint16_t Packet::getDstPort() const {
    return m_connInfo.dstPort;
}

const std::vector <uint8_t> &Packet::getPayload() const {
    return m_payload;
}

int Packet::getFd() const {
    return m_fd;
}

size_t Packet::getTxOffset() const {
    return m_txOffset;
}

void Packet::updateTxOffset(size_t bytes) {
    m_txOffset += bytes;

    if (m_txOffset > m_payload.size()) {
        LOG_WARN("Tx Offset is limited");
        m_txOffset = m_payload.size();
    }
}
