#include "execution/login/LoginEvent.h"

LoginEvent::LoginEvent(std::unique_ptr <EventPacket> pkt)
        : m_pkt(std::move(pkt)) {
}

void LoginEvent::handleEvent(ShardContext &shardContext) {
    shardContext.loginContext().handleEvent(std::move(m_pkt));
}

