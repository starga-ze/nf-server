#pragma once

#include <memory>

class ParsedPacket;

class Event;

class EventFactory {
public:
    static std::unique_ptr <Event> create(ParsedPacket &parsed);
};

