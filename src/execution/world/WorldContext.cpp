#include "WorldContext.h"
#include "execution/world/Channel.h"
#include "util/Logger.h"

WorldContext::WorldContext(int shardIdx)
    : m_shardIdx(shardIdx) {
    init();
}

WorldContext::~WorldContext() {
    
}

void WorldContext::init() {
    LOG_INFO("WorldContext init (shard={})", m_shardIdx);

    for (uint32_t ch = 1; ch <= CHANNEL_COUNT; ++ch) {
        auto channel = std::make_unique<Channel>(ch);

        for (uint32_t mapId = 1; mapId <= MAP_PER_CHANNEL; ++mapId) {
            channel->addMap(mapId);
        }

        m_channels.emplace(ch, std::move(channel));
    }
}

void WorldContext::tick(uint64_t deltaMs) {
    for (auto& [_, channel] : m_channels) {
        channel->tick(deltaMs);
    }
}

Channel* WorldContext::getChannel(uint32_t channelId) {
    auto it = m_channels.find(channelId);
    return (it == m_channels.end()) ? nullptr : it->second.get();
}

