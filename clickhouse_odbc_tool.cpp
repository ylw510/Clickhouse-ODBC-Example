#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <stdexcept>

std::mutex io_mutex;

// 从文件读取连接字符串
std::string read_conn_str_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) throw std::runtime_error("无法打开连接字符串文件：" + filepath);
    
    std::string conn_str;
    std::getline(file, conn_str);
    
    if (conn_str.empty()) {
        throw std::runtime_error("连接字符串文件为空！");
    }
    
    return conn_str;
}

// 读取SQL文件
std::string read_sql_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file) throw std::runtime_error("无法打开SQL文件：" + filepath);
    return std::string((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
}

// 执行单条SQL语句
void execute_sql(const std::string& conn_str, const std::string& sql, int repeat, int thread_id) {
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;

    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    ret = SQLDriverConnect(dbc, NULL,
                           (SQLCHAR*)conn_str.c_str(), SQL_NTS,
                           NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    if (!SQL_SUCCEEDED(ret)) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "线程" << thread_id << ": 连接失败\n";
        return;
    }

    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    for (int i = 0; i < repeat; ++i) {
        ret = SQLExecDirect(stmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
        
        if (SQL_SUCCEEDED(ret)) {
            if (i % 1000 == 0) {
                std::lock_guard<std::mutex> lock(io_mutex);
                std::cout << "线程" << thread_id << ": 第" << (i+1) << "次执行成功\n";        
            }
        } else {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cerr << "线程" << thread_id << ": 第" << (i+1) << "次执行失败\n";
        }
    }//end for
    // 所有插入完成后提示
    {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "线程" << thread_id << ": 已完成全部 " << repeat << " 次插入。\n";
    }
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    SQLDisconnect(dbc);
    SQLFreeHandle(SQL_HANDLE_DBC, dbc);
    SQLFreeHandle(SQL_HANDLE_ENV, env);
}

// 主程序
int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "用法: " << argv[0] << " <conn_str> <mode:DDL|DML> <sql_file> <threads> <repeat>\n";
        return 1;
    }

    std::string conn_file = argv[1];
    std::string conn_str;
    try {
        conn_str = read_conn_str_from_file(conn_file);
    } catch (const std::exception& ex) {
        std::cerr << "读取连接字符串失败: " << ex.what() << '\n';
        return 1;
    }

    std::string mode = argv[2];
    std::string sql_file = argv[3];
    int threads = std::stoi(argv[4]);
    int repeat = std::stoi(argv[5]);

    std::string sql;
    try {
        sql = read_sql_from_file(sql_file);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    if (mode == "DDL") {
        std::cout << "执行DDL语句...\n";
        execute_sql(conn_str, sql, 1, 0);
    } else if (mode == "DML") {
        std::cout << "多线程执行DML语句...\n";
        std::vector<std::thread> thread_pool;
        for (int i = 0; i < threads; ++i) {
            thread_pool.emplace_back(execute_sql, conn_str, sql, repeat, i);
        }
        for (auto& t : thread_pool) t.join();
    } else {
        std::cerr << "无效模式: 应该为DDL或DML\n";
        return 1;
    }

    return 0;
}