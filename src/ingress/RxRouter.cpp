#include "RxRouter.h"
#include "util/Logger.h"
#include "ingress/EventFactory.h"
#include "execution/shard/ShardManager.h"
#include "execution/Event.h"

RxRouter::RxRouter(ShardManager *shardManager, SessionManager *sessionManager) :
        m_shardManager(shardManager),
        m_sessionManager(sessionManager) {

}

RxRouter::~RxRouter() {

}

void RxRouter::handlePacket(std::unique_ptr <Packet> packet) {
    LOG_TRACE("RxRouter Dump\n{}", packet->dump());

    /* Parser intentionlly create session id */
    auto parsedPacket = m_packetParser.parse(std::move(packet));
    if (not parsedPacket) {
        return;
    }

    ParsedPacket &parsed = *parsedPacket;

    // LOG_TRACE("RX Dump\n{}", parsed.dump());

    const uint64_t sessionId = parsed.getSessionId();

    Session *session = m_sessionManager->find(sessionId);
    if (not session) {
        session = m_sessionManager->create(sessionId);
    }

    session->bind(parsed.getConnInfo(), parsed.getFd());

    size_t shardIdx = selectShard(sessionId);

    auto event = EventFactory::create(parsed);

    if (not event) {
        return;
    }
    m_shardManager->dispatch(shardIdx, std::move(event));
}

size_t RxRouter::selectShard(const uint64_t sessionId) const {
    size_t workerCount = m_shardManager->getWorkerCount();

    size_t shardIdx = static_cast<size_t>((sessionId >> 32) % workerCount);

    return shardIdx;
}

