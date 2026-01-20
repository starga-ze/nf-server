#include "Map.h"
#include "util/Logger.h"

Map::Map(uint32_t mapId)
        : m_mapId(mapId) {
    LOG_INFO("Map created (mapId={})", m_mapId);
}

uint32_t Map::getMapId() const {
    return m_mapId;
}

void Map::addPlayer(PlayerId playerId, int fd) {
    PlayerState st{};
    st.playerId = playerId;
    st.fd = fd;
    st.x = st.y = st.z = 0.0f;

    m_players[playerId] = st;

    LOG_INFO("Map[{}]: player {} joined (fd={})", m_mapId, playerId, fd);
}

void Map::removePlayer(PlayerId playerId) {
    m_players.erase(playerId);
    LOG_INFO("Map[{}]: player {} left", m_mapId, playerId);
}

void Map::tick(float /*dt*/) {
    // TODO:
    // - movement snapshot
    // - AOI update
    // - periodic broadcast
}

size_t Map::playerCount() const {
    return m_players.size();
}

