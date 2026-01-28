#include "execution/Action.h"
#include "packet/ParsedPacketTypes.h"
#include <memory>

class LoginBuilder
{
public:
    LoginBuilder() = default;
    ~LoginBuilder() = default;

    static std::unique_ptr<Action> serialize(Opcode opcode, uint64_t sessionId);

private:
    static std::unique_ptr<Action> buildLoginResSuccess(Opcode opcode, uint64_t sessionId);
    static std::unique_ptr<Action> buildLoginResFail(Opcode opcode, uint64_t sessionId);
};
