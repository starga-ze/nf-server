#include "DbManager.h"
#include "util/Logger.h"

#include <cstring>

DbManager::~DbManager() {
    std::lock_guard <std::mutex> lock(m_mtx);

    while (!m_pool.empty()) {
        auto conn = m_pool.front();
        m_pool.pop();

        if (conn && conn->dbc != SQL_NULL_HDBC) {
            SQLDisconnect(conn->dbc);
            SQLFreeHandle(SQL_HANDLE_DBC, conn->dbc);
        }
    }

    if (m_env != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, m_env);
        m_env = SQL_NULL_HENV;
    }
}

bool DbManager::init(const DbConfig &config) {
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_env) != SQL_SUCCESS) {
        LOG_ERROR("SQLAllocHandle ENV failed");
        return false;
    }

    SQLSetEnvAttr(m_env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, 0);

    const std::string connStr = config.buildConnStr();

    for (size_t i = 0; i < config.poolSize; ++i) {
        SQLHDBC dbc = SQL_NULL_HDBC;

        if (SQLAllocHandle(SQL_HANDLE_DBC, m_env, &dbc) != SQL_SUCCESS) {
            LOG_ERROR("SQLAllocHandle DBC failed");
            return false;
        }

        SQLCHAR outConn[1024];
        SQLSMALLINT outLen = 0;

        SQLRETURN ret = SQLDriverConnect(
                dbc,
                nullptr,
                (SQLCHAR *) connStr.c_str(),
                SQL_NTS,
                outConn,
                sizeof(outConn),
                &outLen,
                SQL_DRIVER_NOPROMPT
        );

        if (!SQL_SUCCEEDED(ret)) {
            LOG_ERROR("[DB] SQLDriverConnect failed");
            SQLFreeHandle(SQL_HANDLE_DBC, dbc);
            return false;
        }

        auto conn = std::make_shared<DbConnection>();
        conn->dbc = dbc;

        m_pool.push(conn);

        LOG_INFO("Connection {} connected", i);
    }

    LOG_INFO("DbManager initialized, poolSize={}", config.poolSize);
    return true;
}

std::shared_ptr <DbConnection> DbManager::acquire() {
    std::lock_guard <std::mutex> lock(m_mtx);

    if (m_pool.empty()) {
        return nullptr;
    }
    auto conn = m_pool.front();
    m_pool.pop();
    return conn;
}

void DbManager::release(std::shared_ptr <DbConnection> conn) {
    if (not conn) {
        return;
    }
    std::lock_guard <std::mutex> lock(m_mtx);
    m_pool.push(conn);
}

std::optional <std::string> DbManager::getAccountPassword(const std::string &id) {
    auto conn = acquire();
    if (not conn || conn->dbc == SQL_NULL_HDBC) {
        LOG_ERROR("Acquire failed");
        return std::nullopt;
    }

    SQLHSTMT stmt = SQL_NULL_HSTMT;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, conn->dbc, &stmt);
    if (not SQL_SUCCEEDED(ret)) {
        LOG_ERROR("SQLAllocHandle STMT failed");
        release(conn);
        return std::nullopt;
    }

    const char *sql = "SELECT password FROM dbo.account WHERE username = ?";

    ret = SQLPrepare(stmt, (SQLCHAR *) sql, SQL_NTS);
    if (not SQL_SUCCEEDED(ret)) {
        LOG_ERROR("[DB] SQLPrepare failed");
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        release(conn);
        return std::nullopt;
    }

    SQLLEN idInd = (SQLLEN) id.size();
    ret = SQLBindParameter(
            stmt,
            1,
            SQL_PARAM_INPUT,
            SQL_C_CHAR,
            SQL_VARCHAR,
            id.size(),
            0,
            (SQLPOINTER) id.c_str(),
            0,
            &idInd
    );

    if (not SQL_SUCCEEDED(ret)) {
        LOG_ERROR("SQLBindParameter failed");
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        release(conn);
        return std::nullopt;
    }

    ret = SQLExecute(stmt);
    if (not SQL_SUCCEEDED(ret)) {
        LOG_ERROR("SQLExecute failed");
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        release(conn);
        return std::nullopt;
    }

    ret = SQLFetch(stmt);
    if (ret == SQL_NO_DATA) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        release(conn);
        return std::nullopt;
    }

    if (not SQL_SUCCEEDED(ret)) {
        LOG_ERROR("SQLFetch failed");
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        release(conn);
        return std::nullopt;
    }

    char pwBuf[256] = {};
    SQLLEN pwInd = 0;

    ret = SQLGetData(stmt, 1, SQL_C_CHAR, pwBuf, sizeof(pwBuf), &pwInd);

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    release(conn);

    if (not SQL_SUCCEEDED(ret) || pwInd == SQL_NULL_DATA) {
        LOG_ERROR("SQLGetData failed or NULL pw");
        return std::nullopt;
    }

    return std::string(pwBuf);
}

