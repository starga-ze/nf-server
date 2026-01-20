#include "util/Logger.h"
#include "ingress/EventFactory.h"
#include "execution/login/LoginEvent.h"
#include "net/packet/ParsedPacket.h"

std::unique_ptr <Event> EventFactory::create(ParsedPacket &parsed) {
    std::unique_ptr <EventPacket> pkt = parsed.prepare();

    /* TODO: implement deserialize */
    switch (pkt->getOpcode()) {
        case Opcode::LOGIN_REQ:
            return std::make_unique<LoginEvent>(std::move(pkt));

        default:
            LOG_WARN("Unhandled opcode {}", static_cast<int>(pkt->getOpcode()));
            return nullptr;
    }
}

