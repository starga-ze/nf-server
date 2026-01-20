#include "World.h"
#include "util/Logger.h"

#include <chrono>
#include <thread>

World::World() {
    init();
}

World::~World() {
    stop();
}

void World::init() {
    LOG_INFO("World init start");

    for (uint32_t ch = 1; ch <= CHANNEL_COUNT; ++ch) {
        auto channel = std::make_unique<Channel>(ch);

        for (uint32_t mapId = 1; mapId <= MAP_PER_CHANNEL; ++mapId) {
            channel->addMap(mapId);
        }

        m_channels.emplace(ch, std::move(channel));
    }

    LOG_INFO("World init done ({} channels, {} maps/channel)",
             CHANNEL_COUNT, MAP_PER_CHANNEL);
}

void World::start() {
    using clock = std::chrono::steady_clock;
    using namespace std::chrono;

    LOG_INFO("World tick thread started ({} Hz)", 1.0 / WORLD_TICK);

    m_running = true;

    const auto tickDuration =
            duration_cast<clock::duration>(duration<double>(WORLD_TICK));

    while (m_running) {
        auto start = clock::now();

        tick(WORLD_TICK);

        auto elapsed = clock::now() - start;
        if (elapsed < tickDuration) {
            std::this_thread::sleep_for(tickDuration - elapsed);
        }
    }
}

void World::stop() {
    m_running = false;
}

void World::tick(float dt) {
    for (auto &[_, channel]: m_channels) {
        //LOG_TRACE("Channel id : {} process tick.", channel->getChannelId());
        channel->tick(dt);
    }
}

Channel *World::getChannel(uint32_t channelId) {
    auto it = m_channels.find(channelId);
    if (it == m_channels.end())
        return nullptr;

    return it->second.get();
}

