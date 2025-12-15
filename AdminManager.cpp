#include "AdminManager.h"
#include "Common.h"
#include "DBHelper.h"
#include <sstream>  
#include <stdexcept>

//登录验证
bool AdminManager::verifyLogin(const std::string& adminId, const std::string& pwd) {
    lastError.clear();

    std::string trimmedId = Common::trim(adminId);
    std::string trimmedPwd = Common::trim(pwd);
    if (trimmedId.empty() || trimmedPwd.empty()) {
        lastError = "账号或密码不能为空";
        return false;
    }
    if (!Common::isValidAdminID(trimmedId)) {
        lastError = "账号格式错误";
        return false;
    }

    //SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT admin_pwd FROM admin WHERE admin_id = '" << trimmedId << "'";
    std::string sql = sqlStream.str(); 

    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "登录失败：" + dbErr.errorMsg;
        return false;
    }

    bool loginSuccess = false;
    if (result->rowCount == 0) {
        lastError = "账号不存在！";
    }
    else if (result->rowCount == 1) {
        std::string dbPwd = result->rows[0]["admin_pwd"];
        loginSuccess = (dbPwd == trimmedPwd);
        if (!loginSuccess) lastError = "密码错误！";
    }
 

    DBHelper::getInstance().freeResultset(result);
    return loginSuccess;
}

//根据账号查询管理员信息
Admin AdminManager::getAdminById(const std::string& adminId) {
    lastError.clear();
    Admin emptyAdmin;

    std::string trimmedId = Common::trim(adminId);
    if (trimmedId.empty() || !Common::isValidAdminID(trimmedId)) {
        lastError = "账号格式错误！";
        return emptyAdmin;
    }

    std::ostringstream sqlStream;
    sqlStream << "SELECT admin_id, admin_name, admin_pwd FROM admin WHERE admin_id = '" << trimmedId << "'";
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return emptyAdmin;
    }

    Admin admin;
    if (result->rowCount == 1) {
        const auto& row = result->rows[0];
        admin.adminId = row.at("admin_id");
        admin.adminName = row.at("admin_name");
        admin.adminPwd = row.at("admin_pwd");
    }
    else {
        lastError = "未查询到该管理员！";
    }

    DBHelper::getInstance().freeResultset(result);
    return admin;
}

//获取最后一次错误
std::string AdminManager::getLastError() const {
    return lastError;
}

//私有校验函数
bool AdminManager::validateAdmin(const Admin& admin) {
    std::string trimmedId = Common::trim(admin.adminId);
    if (trimmedId.empty() || !Common::isValidAdminID(trimmedId)) {
        lastError = "账号格式错误（4-20位字母+数字组合）！";
        return false;
    }
    return true;
}