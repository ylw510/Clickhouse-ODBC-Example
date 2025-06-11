#pragma once

#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <stdexcept>

class ODBCConnectionPool {
public:
    ODBCConnectionPool(const std::string& dsn, int pool_size = 5)
        : dsn_(dsn), pool_size_(pool_size), env_(nullptr) {
        for (int i = 0; i < pool_size_; ++i) {
            SQLHDBC conn = createConnection();
            if (conn) {
                pool_.push(conn);
            } else {
                throw std::runtime_error("Failed to create ODBC connection");
            }
        }
    }

    ~ODBCConnectionPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!pool_.empty()) {
            SQLHDBC conn = pool_.front();
            pool_.pop();
            SQLDisconnect(conn);
            SQLFreeHandle(SQL_HANDLE_DBC, conn);
        }
        if (env_) {
            SQLFreeHandle(SQL_HANDLE_ENV, env_);
            env_ = nullptr;
        }
    }

    SQLHDBC getConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [&] { return !pool_.empty(); });

        SQLHDBC conn = pool_.front();
        pool_.pop();
        return conn;
    }

    void returnConnection(SQLHDBC conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push(conn);
        cond_.notify_one();
    }

private:
    SQLHDBC createConnection() {
        if (!env_) {
            // 初始化环境句柄只做一次
            if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_) != SQL_SUCCESS) {
                throw std::runtime_error("Error allocating ODBC environment handle");
            }

            if (SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0) != SQL_SUCCESS) {
                SQLFreeHandle(SQL_HANDLE_ENV, env_);
                env_ = nullptr;
                throw std::runtime_error("Error setting ODBC version");
            }
        }

        SQLHDBC dbc;
        if (SQLAllocHandle(SQL_HANDLE_DBC, env_, &dbc) != SQL_SUCCESS) {
            throw std::runtime_error("Error allocating ODBC connection handle");
        }

        // SQLRETURN ret = SQLConnect(dbc,
        //                            (SQLCHAR*)dsn_.c_str(), SQL_NTS,
        //                            NULL, 0, NULL, 0);
        SQLRETURN ret = SQLDriverConnect(dbc,
                                        NULL,
                                        (SQLCHAR*)dsn_.c_str(),
                                        SQL_NTS,
                                        NULL,
                                        0,
                                        NULL,
                                        SQL_DRIVER_COMPLETE);

        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            SQLFreeHandle(SQL_HANDLE_DBC, dbc);
            throw std::runtime_error("Failed to connect to DSN: " + dsn_);
        }

        return dbc;
    }

private:
    std::string dsn_;
    int pool_size_;
    std::queue<SQLHDBC> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    SQLHENV env_;  // Reused for all connections
};
