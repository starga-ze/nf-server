#include "LoginParser.h"
#include "util/Logger.h"
#include "execution/login/LoginEvent.h"

#include <cstring>

std::unique_ptr<Event> LoginParser::deserialize(ParsedPacket& parsed)
{
    switch(parsed.opcode())
    {
        case Opcode::LOGIN_REQ:
            return parseLoginReq(parsed);
        default:
            return nullptr;
    }
}

std::unique_ptr<Event> LoginParser::parseLoginReq(ParsedPacket& parsed)
{
    auto payload = parsed.takePayload();

    constexpr size_t HEADER_SIZE = 8;
    const size_t bodyLen = parsed.bodySize();

    if (payload.size() < HEADER_SIZE || payload.size() < HEADER_SIZE + bodyLen) {
        LOG_WARN("LOGIN_REQ invalid payload size: payload={}, bodyLen={}",
                 payload.size(), bodyLen);
        return nullptr;
    }

    const uint8_t* buf = payload.data() + HEADER_SIZE;
    size_t len = bodyLen;

    size_t offset = 0;

    if (len < sizeof(uint16_t)) {
        LOG_WARN("LOGIN_REQ body too short for idLen");
        return nullptr;
    }

    uint16_t idLen;
    std::memcpy(&idLen, buf + offset, sizeof(uint16_t));
    idLen = ntohs(idLen);
    offset += sizeof(uint16_t);

    if (offset + idLen > len) {
        LOG_WARN("LOGIN_REQ invalid idLen: idLen={}, len={}", idLen, len);
        return nullptr;
    }

    std::string_view id(reinterpret_cast<const char*>(buf + offset), idLen);
    offset += idLen;

    if (offset + sizeof(uint16_t) > len) {
        LOG_WARN("LOGIN_REQ body too short for pwLen");
        return nullptr;
    }

    uint16_t pwLen;
    std::memcpy(&pwLen, buf + offset, sizeof(uint16_t));
    pwLen = ntohs(pwLen);
    offset += sizeof(uint16_t);

    if (offset + pwLen > len) {
        LOG_WARN("LOGIN_REQ invalid pwLen: pwLen={}, len={}", pwLen, len);
        return nullptr;
    }

    std::string_view pw(reinterpret_cast<const char*>(buf + offset), pwLen);

    return std::make_unique<LoginEvent>(
        parsed.getSessionId(),
        std::move(payload),
        id,
        pw
    );
}
