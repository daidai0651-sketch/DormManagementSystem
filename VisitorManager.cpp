#include "VisitorManager.h"
#include "Common.h"
#include "DBHelper.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cctype>

bool VisitorManager::validateVisitor(const Visitor& visitor, bool isAdd) {
    lastError.clear();
    if (!isAdd) {
        std::string trimmedId = Common::trim(visitor.visitorId);
        if (trimmedId.empty()) {
            lastError = "访客ID不能为空！";
            return false;
        }
        for (char c : trimmedId) {
            if (!isdigit(c)) {
                lastError = "访客ID格式错误（必须为纯数字，如1, 2, 3）！";
                return false;
            }
        }
    }
    std::string trimmedName = Common::trim(visitor.visitorName);
    if (trimmedName.empty()) {
        lastError = "访客姓名不能为空！";
        return false;
    }
    if (trimmedName.size() > 20) {
        lastError = "访客姓名不能超过20个字符！";
        return false;
    }
    std::string trimmedGender = Common::trim(visitor.gender);
    if (trimmedGender.empty()) {
        lastError = "性别不能为空！";
        return false;
    }
    if (trimmedGender != "男" && trimmedGender != "女") {
        lastError = "性别格式错误（应为'男'或'女'）！";
        return false;
    }
    std::string trimmedIdCard = Common::trim(visitor.idCard);
    if (trimmedIdCard.empty()) {
        lastError = "身份证号不能为空！";
        return false;
    }
    if (!isValidIdCard(trimmedIdCard)) {
        lastError = "身份证号格式错误（需18位，支持最后一位X）！";
        return false;
    }
    std::string trimmedDormId = Common::trim(visitor.dormId);
    if (trimmedDormId.empty()) {
        lastError = "被访宿舍号不能为空！";
        return false;
    }
    if (!Common::isValidDormID(trimmedDormId)) {
        lastError = "被访宿舍号格式错误！";
        return false;
    }
    if (!isDormExist(trimmedDormId)) {
        lastError = "宿舍号" + trimmedDormId + "对应的宿舍不存在！";
        return false;
    }
    std::string trimmedReason = Common::trim(visitor.visitReason);
    if (trimmedReason.empty()) {
        lastError = "访问事由不能为空！";
        return false;
    }
    if (trimmedReason.size() > 100) {
        lastError = "访问事由不能超过100个字符！";
        return false;
    }
    if (!isValidTimeFormat(visitor.visitTime)) {
        lastError = "拜访时间格式错误（应为HH:MM，如09:30）！";
        return false;
    }
    std::string trimmedLeaveTime = Common::trim(visitor.leaveTime);
    if (!trimmedLeaveTime.empty()) {
        if (!isValidTimeFormat(trimmedLeaveTime)) {
            lastError = "离开时间格式错误（应为HH:MM，如11:45）！";
            return false;
        }
    }
    std::string trimmedRegAdmin = Common::trim(visitor.registerAdmin);
    if (trimmedRegAdmin.empty()) {
        lastError = "登记管理员不能为空！";
        return false;
    }
    if (trimmedRegAdmin.size() > 20) {
        lastError = "登记管理员姓名不能超过20个字符！";
        return false;
    }

    return true;
}
Visitor VisitorManager::rowToVisitor(const std::map<std::string, std::string>& row) {
    Visitor visitor;
    visitor.visitorId = row.at("visitor_id");
    visitor.visitorName = row.at("visitor_name");
    visitor.gender = row.at("gender");
    visitor.idCard = row.at("id_card");
    visitor.dormId = row.at("dorm_id");
    visitor.visitReason = row.at("visit_reason");
    visitor.visitTime = row.at("visit_time");
    visitor.leaveTime = row.at("leave_time");
    visitor.registerAdmin = row.at("register_admin");

    return visitor;
}

std::string VisitorManager::generateVisitorId() {
    //查询当前最大访客ID
    std::string sql = "SELECT MAX(visitor_id) AS max_id FROM visitor";

    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) {
        return "1"; // 查询失败时默认返回1
    }

    // 2. 解析最大序号并+1
    int maxSeq = 0;
    if (result->rowCount > 0 && !result->rows[0]["max_id"].empty()) {
        std::string maxId = result->rows[0]["max_id"];
        Common::stringToInt(maxId, maxSeq);
    }

    DBHelper::getInstance().freeResultset(result);
    maxSeq++;

    // 3. 返回纯数字ID
    return std::to_string(maxSeq);
}

//校验学号是否存在
bool VisitorManager::isStudentExist(const std::string& studentId) {
    std::string trimmedId = Common::trim(studentId);
    if (trimmedId.empty()) return false;

    std::string sql = "SELECT student_id FROM student WHERE student_id = '" + trimmedId + "' LIMIT 1";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) return false;

    bool exist = result->rowCount > 0;
    DBHelper::getInstance().freeResultset(result);
    return exist;
}

//校验宿舍号是否存在
bool VisitorManager::isDormExist(const std::string& dormId) {
    std::string trimmedId = Common::trim(dormId);
    if (trimmedId.empty()) return false;

    std::string sql = "SELECT dorm_id FROM dorm WHERE dorm_id = '" + trimmedId + "' LIMIT 1";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) return false;

    bool exist = result->rowCount > 0;
    DBHelper::getInstance().freeResultset(result);
    return exist;
}

//校验身份证号格式（18位，支持最后一位X）
bool VisitorManager::isValidIdCard(const std::string& idCard) {
    if (idCard.size() != 18) return false;
    for (int i = 0; i < 17; i++) {
        if (!isdigit(idCard[i])) return false;
    }
    char lastChar = idCard[17];
    return isdigit(lastChar) || lastChar == 'X' || lastChar == 'x';
}

//校验时间格式
bool VisitorManager::isValidTimeFormat(const std::string& time) {
    if (time.size() != 5) return false;
    if (time[2] != ':') return false;
    std::string hourStr = time.substr(0, 2);
    std::string minuteStr = time.substr(3, 2);
    int hour = 0, minute = 0;
    if (!Common::stringToInt(hourStr, hour) || !Common::stringToInt(minuteStr, minute)) {
        return false;
    }
    return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59;
}

//校验出入时间逻辑（离开时间≥拜访时间）
bool VisitorManager::isValidTimeLogic(const Date& visitDate, const std::string& visitTime,
    const Date& leaveDate, const std::string& leaveTime) {
    //拜访日期 > 离开日期：无效
    if (visitDate.year > leaveDate.year ||
        (visitDate.year == leaveDate.year && visitDate.month > leaveDate.month) ||
        (visitDate.year == leaveDate.year && visitDate.month == leaveDate.month && visitDate.day > leaveDate.day)) {
        return false;
    }

    //拜访日期 == 离开日期：校验时间
    if (visitDate.year == leaveDate.year &&
        visitDate.month == leaveDate.month &&
        visitDate.day == leaveDate.day) {
        int visitHour = std::stoi(visitTime.substr(0, 2));
        int visitMin = std::stoi(visitTime.substr(3, 2));
        int leaveHour = std::stoi(leaveTime.substr(0, 2));
        int leaveMin = std::stoi(leaveTime.substr(3, 2));
        if (leaveHour < visitHour || (leaveHour == visitHour && leaveMin < visitMin)) {
            return false;
        }
    }
    return true;
}

//  新增访客登记实现 
bool VisitorManager::addVisitor(const Visitor& visitor) {
    lastError.clear();

    //校验数据合法性（添加模式）
    if (!validateVisitor(visitor, true)) {
        return false;
    }

    //生成访客ID
    std::string visitorId = generateVisitorId();
    if (visitorId.empty()) {
        lastError = "添加失败：访客ID生成失败！";
        return false;
    }

    //构建INSERT SQL（处理空值）
    std::ostringstream sqlStream;
    std::string trimmedLeaveTime = Common::trim(visitor.leaveTime);

    //组合当前日期和拜访时间
    std::string currentDate = Common::getCurrentDateStr("-");
    std::string visitDateTime = currentDate + " " + visitor.visitTime;

    sqlStream << "INSERT INTO visitor (visitor_id, visitor_name, gender, id_card, dorm_id, "
        << "visit_reason, visit_time, leave_time, register_admin) "
        << "VALUES ("
        << "'" << visitorId << "', "
        << "'" << Common::trim(visitor.visitorName) << "', "
        << "'" << Common::trim(visitor.gender) << "', "
        << "'" << Common::trim(visitor.idCard) << "', "
        << "'" << Common::trim(visitor.dormId) << "', "
        << "'" << Common::trim(visitor.visitReason) << "', "
        << "'" << visitDateTime << "', ";

    // 离开时间空值处理
    if (trimmedLeaveTime.empty()) {
        sqlStream << "NULL, ";
    }
    else {
        // 组合当前日期和离开时间
        std::string leaveDateTime = currentDate + " " + trimmedLeaveTime;
        sqlStream << "'" << leaveDateTime << "', ";
    }

    sqlStream << "'" << Common::trim(visitor.registerAdmin) << "'";
    sqlStream << ")";

    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "添加失败：" + dbErr.errorMsg;
        return false;
    }

    return affectedRows >= 0;
}

//  修改访客信息实现 
bool VisitorManager::updateVisitor(const Visitor& visitor) {
    lastError.clear();

    // 校验数据合法性（修改模式）
    if (!validateVisitor(visitor, false)) {
        return false;
    }

    //检查访客记录是否存在
    Visitor existVisitor = getVisitorById(visitor.visitorId);
    if (existVisitor.visitorId.empty()) {
        lastError = "修改失败：未查询到访客ID" + visitor.visitorId + "对应的记录！";
        return false;
    }

    // 3. 构建UPDATE SQL
    std::ostringstream sqlStream;

    // 组合当前日期和拜访时间
    std::string currentDate = Common::getCurrentDateStr("-");
    std::string visitDateTime = currentDate + " " + visitor.visitTime;

    sqlStream << "UPDATE visitor SET "
        << "visitor_name = '" << Common::trim(visitor.visitorName) << "', "
        << "gender = '" << Common::trim(visitor.gender) << "', "
        << "id_card = '" << Common::trim(visitor.idCard) << "', "
        << "dorm_id = '" << Common::trim(visitor.dormId) << "', "
        << "visit_reason = '" << Common::trim(visitor.visitReason) << "', "
        << "visit_time = '" << visitDateTime << "', "
        << "register_admin = '" << Common::trim(visitor.registerAdmin) << "' ";

    sqlStream << "WHERE visitor_id = '" << Common::trim(visitor.visitorId) << "'";

    // 4. 执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "修改失败：" + dbErr.errorMsg;
        return false;
    }

    return affectedRows >= 0;
}

//  登记访客离开实现 
bool VisitorManager::recordLeave(const std::string& visitorId, const Date& leaveDate, const std::string& leaveTime) {
    lastError.clear();

    // 1. 校验参数
    std::string trimmedId = Common::trim(visitorId);
    if (trimmedId.empty()) {
        lastError = "访客ID不能为空！";
        return false;
    }
    // 只支持纯数字ID
    for (char c : trimmedId) {
        if (!isdigit(c)) {
            lastError = "访客ID格式错误（必须为纯数字）！";
            return false;
        }
    }
    if (!isValidTimeFormat(leaveTime)) {
        lastError = "离开时间格式错误（应为HH:MM，如11:45）！";
        return false;
    }

    // 2. 检查访客记录是否存在
    Visitor existVisitor = getVisitorById(trimmedId);
    if (existVisitor.visitorId.empty()) {
        lastError = "登记失败：未查询到访客ID" + trimmedId + "对应的记录！";
        return false;
    }

    // 3. 构建UPDATE SQL
    std::ostringstream sqlStream;

    // 组合当前日期和离开时间
    std::string currentDate = Common::getCurrentDateStr("-");
    std::string leaveDateTime = currentDate + " " + leaveTime;

    sqlStream << "UPDATE visitor SET "
        << "leave_time = '" << leaveDateTime << "' "
        << "WHERE visitor_id = '" << trimmedId << "'";

    // 4. 执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "添加失败：" + dbErr.errorMsg;
        return false;
    }

    return affectedRows >= 0;
}

//  删除访客记录实现 
bool VisitorManager::deleteVisitor(const std::string& visitorId) {
    lastError.clear();

    // 1. 校验参数
    std::string trimmedId = Common::trim(visitorId);
    if (trimmedId.empty()) {
        lastError = "访客ID不能为空！";
        return false;
    }
    // 只支持纯数字ID
    for (char c : trimmedId) {
        if (!isdigit(c)) {
            lastError = "访客ID格式错误（必须为纯数字）！";
            return false;
        }
    }

    // 2. 检查访客记录是否存在
    Visitor existVisitor = getVisitorById(trimmedId);
    if (existVisitor.visitorId.empty()) {
        lastError = "删除失败：未查询到访客ID" + trimmedId + "对应的记录！";
        return false;
    }

    // 3. 构建DELETE SQL
    std::ostringstream sqlStream;
    sqlStream << "DELETE FROM visitor WHERE visitor_id = '" << trimmedId << "'";

    // 4. 执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "删除失败：" + dbErr.errorMsg;
        return false;
    }

    return affectedRows >= 0;
}

//  通过访客ID查询记录实现 
Visitor VisitorManager::getVisitorById(const std::string& visitorId) {
    lastError.clear();
    Visitor emptyVisitor;

    // 1. 校验参数
    std::string trimmedId = Common::trim(visitorId);
    if (trimmedId.empty()) {
        lastError = "访客ID不能为空！";
        return emptyVisitor;
    }
    // 只支持纯数字ID
    for (char c : trimmedId) {
        if (!isdigit(c)) {
            lastError = "访客ID格式错误（必须为纯数字）！";
            return emptyVisitor;
        }
    }

    // 2. 构建查询SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT visitor_id, visitor_name, gender, id_card, dorm_id, "
        << "visit_reason, visit_time, leave_time, register_admin "
        << "FROM visitor WHERE visitor_id = '" << trimmedId << "'";

    // 3. 执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return emptyVisitor;
    }

    // 4. 解析结果集
    Visitor visitor;
    if (result->rowCount == 1) {
        visitor = rowToVisitor(result->rows[0]);
    }
    else {
        lastError = "未查询到访客ID" + trimmedId + "对应的记录！";
    }

    // 5. 释放结果集
    DBHelper::getInstance().freeResultset(result);
    return visitor;
}

//  分页查询所有访客记录实现 
std::vector<Visitor> VisitorManager::getAllVisitors(const PageParam& pageParam) {
    lastError.clear();
    std::vector<Visitor> visitorList;

    // 1. 校验分页参数
    if (pageParam.pageIndex < 1 || pageParam.pageSize < 1) {
        lastError = "分页参数错误：页码和每页条数必须大于0！";
        return visitorList;
    }

    // 2. 构建分页查询SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT visitor_id, visitor_name, gender, id_card, dorm_id, "
        << "visit_reason, visit_time, leave_time, register_admin "
        << "FROM visitor "
        << "ORDER BY visitor_id ASC "
        << "LIMIT " << pageParam.getOffset() << ", " << pageParam.pageSize;

    // 3. 执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return visitorList;
    }

    // 4. 解析结果集
    for (const auto& row : result->rows) {
        visitorList.push_back(rowToVisitor(row));
    }

    // 5. 释放结果集
    DBHelper::getInstance().freeResultset(result);
    return visitorList;
}

std::vector<Visitor> VisitorManager::filterVisitors(const std::string& studentId, const std::string& dormId,
    VisitorStatus status, const PageParam& pageParam) {
    lastError.clear();
    std::vector<Visitor> visitorList;

    if (pageParam.pageIndex < 1 || pageParam.pageSize < 1) {
        lastError = "分页参数错误：页码和每页条数必须大于0！";
        return visitorList;
    }

    std::ostringstream sqlStream;
    sqlStream << "SELECT visitor_id, visitor_name, gender, id_card, dorm_id, "
        << "visit_reason, visit_time, leave_time, register_admin "
        << "FROM visitor WHERE 1=1 ";

    std::string trimmedStuId = Common::trim(studentId);
    if (!trimmedStuId.empty()) {
        sqlStream << "AND dorm_id IN (SELECT dorm_id FROM student WHERE student_id = '" << trimmedStuId << "') ";
    }

    std::string trimmedDormId = Common::trim(dormId);
    if (!trimmedDormId.empty()) {
        sqlStream << "AND dorm_id = '" << trimmedDormId << "' ";
    }

    if (status == VisitorStatus::VISITING || status == VisitorStatus::LEFT) {
        if (status == VisitorStatus::VISITING) {
            sqlStream << "AND leave_time IS NULL ";
        }
        else {
            sqlStream << "AND leave_time IS NOT NULL ";
        }
    }

    sqlStream << "ORDER BY visitor_id ASC "
        << "LIMIT " << pageParam.getOffset() << ", " << pageParam.pageSize;

    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return visitorList;
    }

    for (const auto& row : result->rows) {
        visitorList.push_back(rowToVisitor(row));
    }


    DBHelper::getInstance().freeResultset(result);
    return visitorList;
}

int VisitorManager::getVisitorTotalCount() {
    lastError.clear();

    std::string sql = "SELECT COUNT(*) AS total FROM visitor";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "获取总数失败：" + dbErr.errorMsg;
        return 0;
    }

    int totalCount = 0;
    if (result->rowCount > 0) {
        Common::stringToInt(result->rows[0]["total"], totalCount);
    }

    DBHelper::getInstance().freeResultset(result);
    return totalCount;
}

int VisitorManager::getFilterTotalCount(const std::string& studentId, const std::string& dormId,
    VisitorStatus status) {
    lastError.clear();

    std::ostringstream sqlStream;
    sqlStream << "SELECT COUNT(*) AS total FROM visitor WHERE 1=1 ";

    std::string trimmedStuId = Common::trim(studentId);
    if (!trimmedStuId.empty()) {
        sqlStream << "AND dorm_id IN (SELECT dorm_id FROM student WHERE student_id = '" << trimmedStuId << "') ";
    }

    std::string trimmedDormId = Common::trim(dormId);
    if (!trimmedDormId.empty()) {
        sqlStream << "AND dorm_id = '" << trimmedDormId << "' ";
    }

    if (status == VisitorStatus::VISITING || status == VisitorStatus::LEFT) {
        if (status == VisitorStatus::VISITING) {
            sqlStream << "AND leave_time IS NULL ";
        }
        else {
            sqlStream << "AND leave_time IS NOT NULL ";
        }
    }

    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "获取总数失败：" + dbErr.errorMsg;
        return 0;
    }

    int totalCount = 0;
    if (result->rowCount > 0) {
        Common::stringToInt(result->rows[0]["total"], totalCount);
    }

    DBHelper::getInstance().freeResultset(result);
    return totalCount;
}

std::string VisitorManager::getLastError() const {
    return lastError;
}

int VisitorManager::getActiveVisitorCount() {
    lastError.clear();

    std::string sql = "SELECT COUNT(*) AS total FROM visitor WHERE leave_time IS NULL";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "获取当前访客数量失败：" + dbErr.errorMsg;
        return 0;
    }

    int totalCount = 0;
    if (result->rowCount > 0) {
        Common::stringToInt(result->rows[0]["total"], totalCount);
    }

    DBHelper::getInstance().freeResultset(result);
    return totalCount;
}
