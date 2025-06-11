cmake_minimum_required(VERSION 3.10)
project(clickhouse_odbc_tool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(clickhouse_odbc_tool
    clickhouse_odbc_tool.cpp
)

# 包含头文件目录（当前目录）
target_include_directories(clickhouse_odbc_tool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 链接 ODBC 库
# Unix/Linux 通常是 odbc 或 unixODBC
find_package(ODBC REQUIRED)
if(ODBC_FOUND)
    target_include_directories(clickhouse_odbc_tool PRIVATE ${ODBC_INCLUDE_DIRS})
    target_link_libraries(clickhouse_odbc_tool PRIVATE ${ODBC_LIBRARIES})
else()
    message(FATAL_ERROR "ODBC library not found")
endif()

# Linux 下 pthread 线程库
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(clickhouse_odbc_tool PRIVATE Threads::Threads)
endif()
