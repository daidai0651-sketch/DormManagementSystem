#ifndef DBHELPER_H
#define DBHELPER_H

#include <mysql.h>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <mutex>  

//数据库相关常量
constexpr const char* DB_DEFAULT_HOST = "localhost";    //主机
constexpr const char* DB_DEFAULT_USER = "root";         //用户名
constexpr const char* DB_DEFAULT_PWD = "root";          // 默认数据库密码
constexpr const char* DB_DEFAULT_NAME = "Dorm_management"; //数据库名
constexpr unsigned int DB_DEFAULT_PORT = 3306;          //数据库端口
constexpr const char* DB_CHARSET = "gb2312";           //数据库字符集

//数据库错误信息结构体
struct DBErrorInfo {
    int errorCode;        //MySQL错误码
    std::string errorMsg; //错误描述

    DBErrorInfo() : errorCode(0), errorMsg("") {}
};

//数据库查询结果集结构体
struct DBResultset {
    std::vector<std::string> fields;                          // 字段名列表
    std::vector<std::map<std::string, std::string>> rows;     // 行数据（字段名→值映射）
    uint64_t rowCount;                                        // 结果集总行数
    uint32_t fieldCount;                                      // 结果集总列数

    //构造函数初始化
    DBResultset() : rowCount(0), fieldCount(0) {}

    //清空结果集
    void clear() {
        fields.clear();
        rows.clear();
        rowCount = 0;
        fieldCount = 0;
    }
};

//数据库操作封装类
class DBHelper {
private:
    MYSQL mysql_conn;          // MySQL连接句柄
    DBErrorInfo last_error;    // 最后一次错误信息
    std::mutex db_mutex;       // 互斥锁（保证SQL执行线程安全）
    bool is_connected;         // 连接状态标记

    //构造、析构
    DBHelper();
    ~DBHelper();

    // 禁止拷贝构造和赋值运算符（单例模式）
    DBHelper(const DBHelper&) = delete;
    DBHelper& operator=(const DBHelper&) = delete;

    //禁止移动构造和移动赋值
    DBHelper(DBHelper&&) = delete;
    DBHelper& operator=(DBHelper&&) = delete;

    // 内部错误处理函数
    void setError(int errCode, const std::string& errMsg);

public:
    //单例实例获取接口
    static DBHelper& getInstance();

    //数据库连接函数
    bool connect(const std::string& host = DB_DEFAULT_HOST,
        const std::string& user = DB_DEFAULT_USER,
        const std::string& pwd = DB_DEFAULT_PWD,
        const std::string& dbName = DB_DEFAULT_NAME,
        unsigned int port = DB_DEFAULT_PORT);

    //断开数据库连接
    void disconnect();
    //执行更新类SQL，比如INSERT/UPDATE/DELETE
    int executeUpdate(const std::string& sql);

    //执行查询类SQL如SELECT
    DBResultset* executeQuery(const std::string& sql);
    //释放查询结果集
    void freeResultset(DBResultset* result);
    //获取最后一次错误信息
    DBErrorInfo getLastError() const;
    //检查数据库连接状态
    bool isConnected() const;
    //重新连接数据库（连接断开时调用）
    bool reconnect();
    //提交事务
    bool commitTransaction();
    //回滚事务
    bool rollbackTransaction();
};

#endif
