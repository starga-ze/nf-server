#pragma once

#include "packet/ParsedPacketTypes.h"

#include <memory>

class Action;

class ActionFactory {
public:
    static std::unique_ptr <Action> create(Opcode opcode, uint64_t sessionId);
};
