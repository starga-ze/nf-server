#include "Channel.h"
#include "util/Logger.h"

Channel::Channel(uint32_t channelId)
        : m_channelId(channelId) {
    LOG_INFO("Channel created (channelId={})", m_channelId);
}

uint32_t Channel::getChannelId() const {
    return m_channelId;
}

void Channel::addMap(uint32_t mapId) {
    if (m_maps.find(mapId) != m_maps.end()) {
        LOG_WARN("Channel[{}]: map {} already exists", m_channelId, mapId);
        return;
    }

    m_maps.emplace(mapId, std::make_unique<Map>(mapId));
}

Map *Channel::getMap(uint32_t mapId) {
    auto it = m_maps.find(mapId);
    if (it == m_maps.end())
        return nullptr;

    return it->second.get();
}

void Channel::tick(float dtSec) {
    for (auto &[_, map]: m_maps) {
        map->tick(dtSec);
    }
}

