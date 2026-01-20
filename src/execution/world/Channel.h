#pragma once

#include "execution/world/Map.h"

#include <cstdint>
#include <unordered_map>
#include <memory>

class Channel {
public:
    explicit Channel(uint32_t channelId);

    uint32_t getChannelId() const;

    void addMap(uint32_t mapId);

    Map *getMap(uint32_t mapId);

    void tick(float dt);

private:
    uint32_t m_channelId;

    std::unordered_map <uint32_t, std::unique_ptr<Map>> m_maps;
};

