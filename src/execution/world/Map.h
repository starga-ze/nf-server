#pragma once

#include <cstdint>
#include <cstddef>
#include <unordered_map>

using PlayerId = uint64_t;

struct PlayerState {
    PlayerId playerId;
    int fd;
    float x, y, z;
};

class Map {
public:
    explicit Map(uint32_t mapId);

    uint32_t getMapId() const;

    void addPlayer(PlayerId playerId, int fd);

    void removePlayer(PlayerId playerId);

    void tick(float dtSec);

    std::size_t playerCount() const;

private:
    uint32_t m_mapId;

    std::unordered_map <PlayerId, PlayerState> m_players;
};

