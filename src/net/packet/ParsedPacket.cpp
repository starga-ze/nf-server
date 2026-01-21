#include "ParsedPacket.h"

#include <sstream>
#include <iomanip>
#include <arpa/inet.h>

ParsedPacket::ParsedPacket(int fd,
                           ConnInfo connInfo,
                           PacketVersion version,
                           Opcode opcode,
                           uint32_t flags,
                           uint64_t sessionId,
                           std::vector <uint8_t> payload,
                           size_t bodyOffset,
                           size_t bodyLen):
        m_fd(fd),
        m_connInfo(connInfo),
        m_version(version),
        m_opcode(opcode),
        m_flags(flags),
        m_sessionId(sessionId),
        m_payload(std::move(payload)),
        m_bodyOffset(bodyOffset),
        m_bodyLen(bodyLen)
{
}

std::vector<uint8_t> ParsedPacket::takePayload() {
    return std::move(m_payload);
}

const uint8_t* ParsedPacket::bodyData() const {
    return m_payload.data() + m_bodyOffset;
}

size_t ParsedPacket::bodySize() const {
    return m_bodyLen;
}

const std::vector<uint8_t>& ParsedPacket::payload() const {
    return m_payload;
}

int ParsedPacket::getFd() const {
    return m_fd;
}

const ConnInfo &ParsedPacket::getConnInfo() const {
    return m_connInfo;
}

PacketVersion ParsedPacket::version() const {
    return m_version;
}

Opcode ParsedPacket::opcode() const {
    return m_opcode;
}

uint32_t ParsedPacket::flags() const {
    return m_flags;
}

uint64_t ParsedPacket::getSessionId() const {
    return m_sessionId;
}
