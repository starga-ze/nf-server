#pragma once

#include <cstdint>

class ShardContext;

class Event {
public:
    Event(uint64_t sessionId) : m_sessionId(sessionId) {}
    virtual ~Event() = default;

    uint64_t sessionId() const { return m_sessionId; }

    virtual void handleEvent(ShardContext &shardContext) = 0;

private:
    uint64_t m_sessionId;
};

