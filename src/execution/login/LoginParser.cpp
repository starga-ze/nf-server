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
    auto body = parsed.takeBody();
    if (body.empty()) {
        LOG_WARN("LOGIN_REQ: body is null");
        return nullptr;
    }

    const auto& buf = body;
    const size_t bodyLen = buf.size();
    size_t offset = 0;

    // id length
    if (offset + sizeof(uint16_t) > bodyLen)
        return nullptr;

    uint16_t idLen;
    std::memcpy(&idLen, buf.data() + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    if (offset + idLen > bodyLen) {
        LOG_WARN("Invalid id length");
        return nullptr;
    }

    std::string_view id(
        reinterpret_cast<const char*>(buf.data() + offset),
        idLen);
    offset += idLen;

    // pw length
    if (offset + sizeof(uint16_t) > bodyLen)
        return nullptr;

    uint16_t pwLen;
    std::memcpy(&pwLen, buf.data() + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    if (offset + pwLen > bodyLen) {
        LOG_WARN("Invalid pw length");
        return nullptr;
    }

    std::string_view pw(
        reinterpret_cast<const char*>(buf.data() + offset),
        pwLen);

    return std::make_unique<LoginEvent>(parsed.getSessionId(), std::move(body), id, pw);
}
