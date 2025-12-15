#include "DBHelper.h"
#include "Common.h"
#include <mysql.h>
#include <cstring>
#include <iostream>

//构造函数
DBHelper::DBHelper() : is_connected(false) {
    mysql_init(&mysql_conn);

    //连接超时
    unsigned int timeout = 30;
    mysql_options(&mysql_conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    last_error.errorCode = 0;
    last_error.errorMsg = "";
}

//析构函数
DBHelper::~DBHelper() {
    disconnect();
}

//单例实例获取接口
DBHelper& DBHelper::getInstance() {
    static DBHelper instance;
    return instance;
}

//内部错误处理函数
void DBHelper::setError(int errCode, const std::string& errMsg) {
    last_error.errorCode = errCode;
    last_error.errorMsg = errMsg;
    
    //只在有错误时才输出错误信息
    if (errCode != 0 && !errMsg.empty()) {
        std::cerr << "[DB Error] Code: " << errCode << ", Msg: " << errMsg << std::endl;
    }
}

//数据库连接函数实现
bool DBHelper::connect(const std::string& host,
    const std::string& user,
    const std::string& pwd,
    const std::string& dbName,
    unsigned int port) {
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_connected) return true;

    //缩短连接超时，避免长时间卡住
    unsigned int short_timeout = 5;
    mysql_options(&mysql_conn, MYSQL_OPT_CONNECT_TIMEOUT, &short_timeout);

    //优先：如果支持 MYSQL_OPT_SSL_MODE，则显式禁用 SSL
#if defined(MYSQL_OPT_SSL_MODE) && defined(MYSQL_SSL_MODE_DISABLED)
    {
        enum mysql_ssl_mode ssl_mode = MYSQL_SSL_MODE_DISABLED;
        if (mysql_options(&mysql_conn, MYSQL_OPT_SSL_MODE, &ssl_mode) != 0) {
            std::cerr << "[DB] Warning: setting MYSQL_OPT_SSL_MODE failed" << std::endl;
        }
        else {
            std::cerr << "[DB] Set MYSQL_OPT_SSL_MODE = MYSQL_SSL_MODE_DISABLED" << std::endl;
        }
    }
#else
    //否则尝试通过清空各 SSL 相关选项来避免客户端触发 SSL（有的老版本连接库会在检测到证书时启用）
    const char* empty = "";
#ifdef MYSQL_OPT_SSL_CA
    mysql_options(&mysql_conn, MYSQL_OPT_SSL_CA, (const void*)empty);
#endif
#ifdef MYSQL_OPT_SSL_CAPATH
    mysql_options(&mysql_conn, MYSQL_OPT_SSL_CAPATH, (const void*)empty);
#endif
#ifdef MYSQL_OPT_SSL_CERT
    mysql_options(&mysql_conn, MYSQL_OPT_SSL_CERT, (const void*)empty);
#endif
#ifdef MYSQL_OPT_SSL_KEY
    mysql_options(&mysql_conn, MYSQL_OPT_SSL_KEY, (const void*)empty);
#endif
#ifdef MYSQL_OPT_SSL_CIPHER
    mysql_options(&mysql_conn, MYSQL_OPT_SSL_CIPHER, (const void*)empty);
#endif
    std::cerr << "[DB] Attempted to clear SSL-related mysql_options (if supported by client lib)." << std::endl;
#endif

    unsigned long client_flags = 0;

    //尝试连接
    if (mysql_real_connect(&mysql_conn, host.c_str(), user.c_str(), pwd.c_str(), dbName.c_str(), port, nullptr, client_flags) == nullptr) {
        int err = mysql_errno(&mysql_conn);
        const char* emsg = mysql_error(&mysql_conn);
        std::cerr << "[DB] mysql_real_connect failed. err=" << err
            << " msg=" << (emsg ? emsg : "<null>") << std::endl;

        if (err == 2026) {
            std::cerr << "[DB] SSL connection error (2026). Server allows non-SSL; client attempted SSL / or SSL runtime mismatch." << std::endl;
            std::cerr << "[DB] Ensure libmysql.dll in exe folder is the intended one, and consider using matching OpenSSL (1.1) or upgrading libmysql." << std::endl;
        }

        setError(err, emsg ? emsg : "Unknown MySQL error");
        is_connected = false;
        return false;
    }

    //设置字符集
    if (mysql_set_character_set(&mysql_conn, DB_CHARSET) != 0) {
        int err = mysql_errno(&mysql_conn);
        const char* emsg = mysql_error(&mysql_conn);
        setError(err, std::string("设置字符集失败：") + (emsg ? emsg : "<null>"));
        mysql_close(&mysql_conn);
        is_connected = false;
        return false;
    }

    //设置自动提交，确保每次更新立即生效
    if (mysql_autocommit(&mysql_conn, 1) != 0) {
        int err = mysql_errno(&mysql_conn);
        const char* emsg = mysql_error(&mysql_conn);
        setError(err, std::string("设置自动提交失败：") + (emsg ? emsg : "<null>"));
        mysql_close(&mysql_conn);
        is_connected = false;
        return false;
    }

    is_connected = true;
    setError(0, "");
    std::cout << "数据库连接成功！" << std::endl;
    return true;
}

//断开数据库连接
void DBHelper::disconnect() {
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_connected) {
        mysql_close(&mysql_conn);
        is_connected = false;
        std::cout << "数据库连接已断开！" << std::endl;
    }

    mysql_init(&mysql_conn);
}

//更新类SQL实现 
int DBHelper::executeUpdate(const std::string& sql) {
    if (!is_connected && !reconnect()) return -1;

    std::lock_guard<std::mutex> lock(db_mutex);
    setError(0, "");

    if (mysql_query(&mysql_conn, sql.c_str()) != 0) {
        setError(mysql_errno(&mysql_conn), std::string("执行SQL失败：") + mysql_error(&mysql_conn) + " [SQL: " + sql + "]");
        return -1;
    }

    //获取受影响的行数
    int affectedRows = static_cast<int>(mysql_affected_rows(&mysql_conn));
    
    //如果是INSERT/UPDATE/DELETE操作，强制刷新数据库缓存
    if (affectedRows > 0) {
        std::string sqlLower = sql;
        std::transform(sqlLower.begin(), sqlLower.end(), sqlLower.begin(), ::tolower);
        
        //检查是否为数据修改操作
        if (sqlLower.find("insert") == 0 || sqlLower.find("update") == 0 || sqlLower.find("delete") == 0) {
            //强制提交事务以确保数据立即写入磁盘
            mysql_commit(&mysql_conn);
            
            //刷新表缓存，确保Navicat等工具能看到最新数据
            std::string flushSql = "FLUSH TABLES";
            mysql_query(&mysql_conn, flushSql.c_str());
            
            Sleep(200);
        }
    }
    
    return affectedRows;
}

//查询类SQL
DBResultset* DBHelper::executeQuery(const std::string& sql) {
    if (!is_connected && !reconnect()) return nullptr;

    std::lock_guard<std::mutex> lock(db_mutex);
    setError(0, "");
    
    //提交任何挂起的事务，确保查询到最新数据
    mysql_commit(&mysql_conn);

    if (mysql_query(&mysql_conn, sql.c_str()) != 0) {
        setError(mysql_errno(&mysql_conn), std::string("执行查询SQL失败：") + mysql_error(&mysql_conn) + " [SQL: " + sql + "]");
        return nullptr;
    }

    MYSQL_RES* mysql_result = mysql_store_result(&mysql_conn);
    if (mysql_result == nullptr) {
        if (mysql_field_count(&mysql_conn) == 0) {
            setError(0, "查询无数据返回");
            return nullptr;
        }
        else {
            setError(mysql_errno(&mysql_conn), std::string("获取结果集失败：") + mysql_error(&mysql_conn));
            return nullptr;
        }
    }

    DBResultset* result = new (std::nothrow) DBResultset();
    if (result == nullptr) {
        setError(-1, "内存分配失败：创建DBResultset对象失败");
        mysql_free_result(mysql_result);
        return nullptr;
    }

    uint32_t field_count = mysql_num_fields(mysql_result);
    MYSQL_FIELD* fields = mysql_fetch_fields(mysql_result);
    for (uint32_t i = 0; i < field_count; ++i) {
        result->fields.push_back(fields[i].name);
    }

    MYSQL_ROW mysql_row;
    while ((mysql_row = mysql_fetch_row(mysql_result)) != nullptr) {
        std::map<std::string, std::string> row_map;
        unsigned long* field_lengths = mysql_fetch_lengths(mysql_result);

        for (uint32_t i = 0; i < field_count; ++i) {
            if (mysql_row[i] == nullptr) {
                row_map[result->fields[i]] = "";
            }
            else {
                row_map[result->fields[i]] = std::string(mysql_row[i], field_lengths[i]);
            }
        }

        result->rows.push_back(row_map);
    }

    result->rowCount = mysql_num_rows(mysql_result);
    result->fieldCount = field_count;
    mysql_free_result(mysql_result);
    return result;
}

//释放查询结果集实现
void DBHelper::freeResultset(DBResultset* result) {
    if (result != nullptr) {
        result->clear();
        delete result;
    }
}

//获取最后一次错误信息实现
DBErrorInfo DBHelper::getLastError() const {
    return last_error;
}

//检查数据库连接状态实现
bool DBHelper::isConnected() const {
    return is_connected;
}

//重新连接数据库实现
bool DBHelper::reconnect() {
    std::lock_guard<std::mutex> lock(db_mutex);

    if (is_connected) {
        mysql_close(&mysql_conn);
        mysql_init(&mysql_conn);
        is_connected = false;
    }

    return false;
}

//提交事务实现
bool DBHelper::commitTransaction() {
    if (!is_connected && !reconnect()) return false;
    
    std::lock_guard<std::mutex> lock(db_mutex);
    setError(0, "");
    
    if (mysql_commit(&mysql_conn) != 0) {
        setError(mysql_errno(&mysql_conn), std::string("提交事务失败：") + mysql_error(&mysql_conn));
        return false;
    }
    
    return true;
}

//回滚事务实现
bool DBHelper::rollbackTransaction() {
    if (!is_connected && !reconnect()) return false;
    
    std::lock_guard<std::mutex> lock(db_mutex);
    setError(0, "");
    
    if (mysql_rollback(&mysql_conn) != 0) {
        setError(mysql_errno(&mysql_conn), std::string("回滚事务失败：") + mysql_error(&mysql_conn));
        return false;
    }
    
    return true;
}