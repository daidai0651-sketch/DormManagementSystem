#ifndef ADMINMANAGER_H
#define ADMINMANAGER_H

#include "Common.h"
#include "DBHelper.h"
#include <vector>

//管理员结构体
struct Admin {
    std::string adminId;    //账号
    std::string adminName;  //姓名
    std::string adminPwd;   //密码

    // 构造函数（默认初始化空字符串）
    Admin() : adminId(""), adminName(""), adminPwd("") {}

    // 带参数构造函数
    Admin(const std::string& id, const std::string& name, const std::string& pwd)
        : adminId(id), adminName(name), adminPwd(pwd) {
    }
};

//管理员业务逻辑封装类
class AdminManager {
public:
    //登录验证
    bool verifyLogin(const std::string& adminId, const std::string& pwd);

    //根据账号查询管理员信息
    Admin getAdminById(const std::string& adminId);

    //获取最后一次操作错误信息
    std::string getLastError() const;

private:
    std::string lastError; // 存储最后一次操作错误描述

    //校验管理员结构体数据合法性，添加/修改
    bool validateAdmin(const Admin& admin);
};

#endif
