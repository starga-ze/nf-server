#pragma once

#include "util/Macro.h"
#include "util/ThreadManager.h"

#include "db/DbManager.h"
#include "execution/shard/ShardManager.h"
#include "session/SessionManager.h"

#include "net/udp/UdpServer.h"
#include "net/udp/UdpClient.h"
#include "net/tcp/TcpServer.h"
#include "net/tcp/TcpClient.h"
#include "net/tls/TlsServer.h"
#include "net/tls/TlsClient.h"
#include "net/tls/TlsContext.h"

#include "ingress/RxRouter.h"
#include "egress/TxRouter.h"

#include <memory>
#include <vector>
#include <atomic>

class Core {
public:
    Core() = default;

    ~Core() = default;

    void run();

    void shutdown();

    void setFlag(bool enableDb);

private:
    bool initializeRuntime();

    bool initializeTlsContext();

    void handleSignal();

    static void signalHandler(int signum);

    bool initDatabase();

    bool initThreadManager();

    bool initShardManager();

    bool initSessionManager();

    void initializeIngress();

    void initializeEgress();

    void initializeClients();

    void startThreads();

    void waitForShutdown();

    std::unique_ptr <TlsContext> m_tlsContext;

    std::unique_ptr <DbManager> m_dbManager;
    std::unique_ptr <ThreadManager> m_threadManager;
    std::unique_ptr <ShardManager> m_shardManager;
    std::unique_ptr <SessionManager> m_sessionManager;

    std::unique_ptr <RxRouter> m_rxRouter;
    std::unique_ptr <TxRouter> m_txRouter;

    std::unique_ptr <UdpServer> m_udpServer;
    std::unique_ptr <TcpServer> m_tcpServer;
    std::shared_ptr <TlsServer> m_tlsServer;

    std::vector <std::unique_ptr<UdpClient>> m_udpClientList;
    std::vector <std::unique_ptr<TcpClient>> m_tcpClientList;
    std::vector <std::unique_ptr<TlsClient>> m_tlsClientList;

    bool m_enableDb = false;

    int m_shardWorkerThread = 0;

    int m_tcpClients = 0;
    int m_udpClients = 0;
    int m_tlsClients = 0;

    int m_tcpServerWorkerThread = 0;
    int m_udpServerWorkerThread = 0;
    int m_tlsServerWorkerThread = 0;

    int m_tcpServerPort = 0;
    int m_udpServerPort = 0;

    static std::atomic<bool> m_running;
};

