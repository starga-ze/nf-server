#include "util/Logger.h"
#include "ingress/EventFactory.h"
#include "execution/login/LoginParser.h"
#include "net/packet/ParsedPacket.h"

std::unique_ptr <Event> EventFactory::create(ParsedPacket &parsed) {

    switch (parsed.opcode()) {
        case Opcode::LOGIN_REQ:
            return LoginParser::deserialize(parsed);

        default:
            LOG_WARN("Unhandled opcode {}", static_cast<int>(parsed.opcode()));
            return nullptr;
    }
}

