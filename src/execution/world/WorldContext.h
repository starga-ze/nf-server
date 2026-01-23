#pragma once

#include <cstdint>
#include <unordered_map>
#include <memory>

class Channel;

const int CHANNEL_COUNT = 5;
const int MAP_PER_CHANNEL = 10;

class WorldContext {
public:
    explicit WorldContext(int shardIdx);
    ~WorldContext();

    void tick(uint64_t deltaMs);

    Channel* getChannel(uint32_t channelId);

private:
    void init();

private:
    int m_shardIdx;
    std::unordered_map<uint32_t, std::unique_ptr<Channel>> m_channels;
};

