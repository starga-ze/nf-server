#pragma once

#include "db/DbConfig.h"

#include <sql.h>
#include <sqlext.h>
#include <queue>
#include <mutex>
#include <memory>
#include <string>
#include <optional>

struct DbConnection {
    SQLHDBC dbc = nullptr;
};

class DbManager {
public:
    DbManager() = default;

    ~DbManager();

    bool init(const DbConfig &config);

    std::shared_ptr <DbConnection> acquire();

    void release(std::shared_ptr <DbConnection> conn);

    std::optional <std::string> getAccountPassword(const std::string &id);

private:
    SQLHENV m_env = SQL_NULL_HENV;

    std::queue <std::shared_ptr<DbConnection>> m_pool;
    std::mutex m_mtx;
};

