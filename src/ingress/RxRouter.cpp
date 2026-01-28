#include "RxRouter.h"
#include "util/Logger.h"
#include "ingress/EventFactory.h"
#include "shard/ShardManager.h"
#include "execution/Event.h"

RxRouter::RxRouter(ShardManager *shardManager, SessionManager *sessionManager) :
        m_shardManager(shardManager),
        m_sessionManager(sessionManager) {

}

RxRouter::~RxRouter() {

}

void RxRouter::handlePacket(std::unique_ptr <Packet> packet) {
    LOG_TRACE("RxRouter Dump\n{}", packet->dump());

    auto parsedPacket = m_packetParser.parse(std::move(packet));
    if (not parsedPacket) {
        return;
    }

    ParsedPacket &parsed = *parsedPacket;

    if (parsed.opcode() == Opcode::LOGIN_REQ and parsed.getSessionId() == 0) {
        if (not m_sessionManager->create(parsed)) {
            LOG_WARN("Session create failed");
            return;
        }
    }
    else {
        if (not m_sessionManager->checkAndBind(parsed)){
            LOG_WARN("Session check and bind failed");
            return;
        }
    }

    if (parsed.getSessionId() == 0)
    {
        LOG_WARN("Invalid SessionId");
        return;
    }
    uint64_t sessionId = parsed.getSessionId();

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

