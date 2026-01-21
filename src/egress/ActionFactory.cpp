#include "egress/ActionFactory.h"
#include "util/Logger.h"

#include "execution/login/LoginAction.h"
#include "execution/login/LoginBuilder.h"

std::unique_ptr <Action> ActionFactory::create(Opcode opcode, uint64_t sessionId) {
    /* TODO: implement serialize */
    switch (opcode) {
        case Opcode::LOGIN_RES_SUCCESS:
        case Opcode::LOGIN_RES_FAIL:
            return LoginBuilder::serialize(opcode, sessionId);

        default:
            LOG_WARN("Unhandled opcode {}", static_cast<int>(opcode));
            return nullptr;
    }
}
