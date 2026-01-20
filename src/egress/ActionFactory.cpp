#include "egress/ActionFactory.h"
#include "util/Logger.h"

#include "execution/login/LoginSuccessAction.h"
#include "execution/login/LoginFailAction.h"

std::unique_ptr <Action> ActionFactory::create(Opcode opcode, uint64_t sessionId) {
    /* TODO: implement serialize */
    switch (opcode) {
        case Opcode::LOGIN_RES_SUCCESS:
            return std::make_unique<LoginSuccessAction>(sessionId);

        case Opcode::LOGIN_RES_FAIL:
            return std::make_unique<LoginFailAction>(sessionId);

        default:
            LOG_WARN("Unhandled opcode {}", static_cast<int>(opcode));
            return nullptr;
    }
}
