#include "LoginContext.h"
#include "util/Logger.h"
#include "execution/shard/ShardManager.h"
#include "egress/ActionFactory.h"
#include "execution/login/LoginSuccessAction.h"
#include "execution/login/LoginFailAction.h"

#include <cstring>
#include <string>

LoginContext::LoginContext(int shardIdx, ShardManager *shardManager, DbManager *dbManager) {
    m_enableDb = false;
    if (dbManager) {
        m_enableDb = true;
        m_dbManager = dbManager;
    }
    m_shardManager = shardManager;
    m_shardIdx = shardIdx;
}

void LoginContext::handleEvent(std::unique_ptr <EventPacket> pkt) {
    if (not pkt) {
        return;
    }

    const auto &body = pkt->getBody();
    size_t bodyLen = body.size();
    size_t offset = 0;

    if (offset + sizeof(uint16_t) > bodyLen) {
        return;
    }

    uint16_t idLen;
    std::memcpy(&idLen, body.data() + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    if (offset + idLen > bodyLen) {
        LOG_WARN("Invalid id length");
        return;
    }
    std::string id(reinterpret_cast<const char *>(body.data() + offset), idLen);
    offset += idLen;

    if (offset + sizeof(uint16_t) > bodyLen) return;

    uint16_t pwLen;
    std::memcpy(&pwLen, body.data() + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    if (offset + pwLen > bodyLen) {
        LOG_WARN("Invalid pw length");
        return;
    }
    std::string pw(reinterpret_cast<const char *>(body.data() + offset), pwLen);

    const uint64_t sessionId = pkt->getSessionId();
    LOG_DEBUG("LOGIN_REQ received, [session = {}, id = '{}', pw = '{}']", sessionId, id, pw);

    std::unique_ptr <Action> action;

    if (m_enableDb) {
        if (!verify(id, pw)) {
            LOG_WARN("Login failed. [session = {}, id = '{}']", sessionId, id);
            return;
        }
        LOG_TRACE("Login success. [session = {}, id = '{}']", sessionId, id);
    } else {
        LOG_WARN("Verify process intentionally passed.");

        if (id == "test" and pw == "test") {
            LOG_DEBUG("Login Success");
            action = ActionFactory::create(Opcode::LOGIN_RES_SUCCESS, sessionId);
        } else {
            LOG_DEBUG("Login Failed");
            action = ActionFactory::create(Opcode::LOGIN_RES_FAIL, sessionId);
        }
        m_shardManager->commit(m_shardIdx, std::move(action));
    }
}

void LoginContext::loginSuccessAction(uint64_t sessionId) {
    LOG_DEBUG("LOGIN_RES_SUCCESS send, [session = {}]", sessionId);
    if (not m_txRouter) {
        LOG_FATAL("TxRouter is nullptr");
    }

    auto pkt = std::make_unique<ActionPacket>(sessionId, Opcode::LOGIN_RES_SUCCESS);

    m_txRouter->handlePacket(std::move(pkt));
}

void LoginContext::loginFailAction(uint64_t sessionId) {
    LOG_DEBUG("LOGIN_RES_FAIL send, [session = {}]", sessionId);
    if (not m_txRouter) {
        LOG_FATAL("TxRouter is nullptr");
    }

    auto pkt = std::make_unique<ActionPacket>(sessionId, Opcode::LOGIN_RES_FAIL);

    m_txRouter->handlePacket(std::move(pkt));
}

void LoginContext::setTxRouter(TxRouter *txRouter) {
    m_txRouter = txRouter;
}

bool LoginContext::verify(const std::string &id, const std::string &pw) {
    auto optDbPw = m_dbManager->getAccountPassword(id);

    if (!optDbPw) {
        LOG_INFO("Account not found (id={})", id);
        return false;
    }

    if (*optDbPw != pw) {
        LOG_INFO("Wrong password (id={})", id);
        return false;
    }

    return true;
}
