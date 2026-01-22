#include "LoginBuilder.h"
#include "util/Logger.h"
#include "execution/login/LoginAction.h"

static constexpr uint8_t VERSION = 0x01;
static constexpr size_t HEADER_SIZE = 8;
static constexpr uint16_t BODY_LEN = 1;

std::unique_ptr<Action> LoginBuilder::serialize(Opcode opcode, uint64_t sessionId)
{
    switch(opcode)
    {
        case Opcode::LOGIN_RES_SUCCESS:
            return buildLoginResSuccess(opcode, sessionId);

        case Opcode::LOGIN_RES_FAIL:
            return buildLoginResFail(opcode, sessionId);

        default:
            return nullptr;
    }
}

std::unique_ptr<Action> LoginBuilder::buildLoginResSuccess(Opcode opcode, uint64_t sessionId)
{
    std::vector<uint8_t> payload;
    payload.resize(HEADER_SIZE + BODY_LEN);

    // version
    payload[0] = VERSION;

    // opcode
    payload[1] = static_cast<uint8_t>(opcode);

    // bodyLen (BE)
    uint16_t netLen = htons(BODY_LEN);
    std::memcpy(&payload[2], &netLen, sizeof(uint16_t));

    // flags (BE)
    uint32_t flags = 0;
    uint32_t netFlags = htonl(flags);
    std::memcpy(&payload[4], &netFlags, sizeof(uint32_t));

    // body
    payload[8] = 0x01;

    return std::make_unique<LoginSuccessAction>(sessionId, opcode, std::move(payload));
}

std::unique_ptr<Action> LoginBuilder::buildLoginResFail(Opcode opcode, uint64_t sessionId)
{
    std::vector<uint8_t> payload;
    payload.resize(HEADER_SIZE + BODY_LEN);

    // version
    payload[0] = VERSION;

    // opcode
    payload[1] = static_cast<uint8_t>(opcode);

    // bodyLen (BE)
    uint16_t netLen = htons(BODY_LEN);
    std::memcpy(&payload[2], &netLen, sizeof(uint16_t));

    // flags (BE)
    uint32_t flags = 0;
    uint32_t netFlags = htonl(flags);
    std::memcpy(&payload[4], &netFlags, sizeof(uint32_t));

    // body
    payload[8] = 0x01;

    return std::make_unique<LoginFailAction>(sessionId, opcode, std::move(payload));
}
