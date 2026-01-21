#include "LoginContext.h"
#include "util/Logger.h"
#include "execution/shard/ShardManager.h"
#include "egress/ActionFactory.h"

#include "execution/login/LoginAction.h"
#include "execution/login/LoginEvent.h"

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

void LoginContext::loginReqEvent(const LoginEvent& ev) {
    const uint64_t sessionId = ev.sessionId();
    const std::string_view id = ev.id();
    const std::string_view pw = ev.pw();

    LOG_DEBUG("LOGIN_REQ received, [session={}, id='{}']", sessionId, id);

    std::unique_ptr<Action> action;

    if (m_enableDb) {
        if (!verify(std::string{id}, std::string{pw})) {
            LOG_WARN("Login failed. [session={}, id='{}']", sessionId, id);
            action = ActionFactory::create(Opcode::LOGIN_RES_FAIL, sessionId);
        } else {
            LOG_TRACE("Login success. [session={}, id='{}']", sessionId, id);
            action = ActionFactory::create(Opcode::LOGIN_RES_SUCCESS, sessionId);
        }
    } else {
        LOG_WARN("Verify process intentionally passed.");

        if (id == "test" && pw == "test") {
            LOG_DEBUG("Login Success");
            action = ActionFactory::create(Opcode::LOGIN_RES_SUCCESS, sessionId);
        } else {
            LOG_DEBUG("Login Failed");
            action = ActionFactory::create(Opcode::LOGIN_RES_FAIL, sessionId);
        }
    }

    if (!action) {
        LOG_ERROR("Action creation failed. [session={}]", sessionId);
        return;
    }

    m_shardManager->commit(m_shardIdx, std::move(action));
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
