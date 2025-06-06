# Clickhouse-ODBC-Example
This is a command-line tool built with C++ that executes SQL statements on a ClickHouse database using the ODBC interface. It supports executing **DDL** or **DML** statements, reading SQL from a file, and multi-threaded execution for DML operations.

---

## ✅ Features

- ✅ Choose between **DDL** and **DML** execution mode via command-line
- ✅ Read SQL statements from an external file
- ✅ Support for multi-threaded execution of DML operations
- ✅ Control how many times each thread executes the SQL

---

## 🛠️ Build Instructions

### Dependencies

- ClickHouse ODBC driver (e.g., `clickhouse-odbc`)
- C++11 or later compiler
- `unixODBC` (Linux) or `odbc32` (Windows)

### Compile on Linux

```bash
g++ clickhouse_odbc_tool.cpp -o clickhouse_odbc_tool -lodbc -std=c++11 -pthread
```

### Usage

```
./clickhouse_odbc_tool <conn_str> <mode:DDL|DML> <sql_file> <threads> <repeat>
