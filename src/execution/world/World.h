#pragma once

#include "execution/world/Channel.h"

#include <unordered_map>
#include <memory>
#include <atomic>

const int CHANNEL_COUNT = 5;
const int MAP_PER_CHANNEL = 10;
const double WORLD_TICK = 1.0 / 30.0;

class World {
public:
    World();

    ~World();

    void init();

    void start();
    void stop();

    Channel *getChannel(uint32_t channelId);

private:
    void tick(float dt);

private:
    std::unordered_map <uint32_t, std::unique_ptr<Channel>> m_channels;

    std::atomic<bool> m_running{false};
};

