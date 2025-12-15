#include "RepairManager.h"
#include "Common.h"
#include "DBHelper.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

//私有辅助函数：校验报修数据合法性 
bool RepairManager::validateRepair(const Repair& repair, bool isAdd) {
    lastError.clear();

    //校验报修ID
    if (!isAdd) {
        std::string trimmedId = Common::trim(repair.repairId);
        if (trimmedId.empty()) {
            lastError = "报修ID不能为空！";
            return false;
        }
        
        //校验是否为纯数字
        for (char c : trimmedId) {
            if (!std::isdigit(c)) {
                lastError = "报修ID格式错误：应为纯数字！";
                return false;
            }
        }
    }

    //校验学号
    std::string trimmedStuId = Common::trim(repair.studentId);
    if (trimmedStuId.empty()) {
        lastError = "学号不能为空！";
        return false;
    }
    if (!Common::isValidStudentID(trimmedStuId)) {
        lastError = "学号格式错误！";
        return false;
    }
    if (!isStudentExist(trimmedStuId)) {
        lastError = "学号" + trimmedStuId + "对应的学生不存在！";
        return false;
    }

    //校验宿舍号
    std::string trimmedDormId = Common::trim(repair.dormId);
    if (trimmedDormId.empty()) {
        lastError = "宿舍号不能为空！";
        return false;
    }
    if (!Common::isValidDormID(trimmedDormId)) {
        lastError = "宿舍号格式错误！";
        return false;
    }
    if (!isDormExist(trimmedDormId)) {
        lastError = "宿舍号" + trimmedDormId + "对应的宿舍不存在！";
        return false;
    }

    //校验报修内容
    std::string trimmedContent = Common::trim(repair.repairContent);
    if (trimmedContent.empty()) {
        lastError = "报修内容不能为空！";
        return false;
    }
    if (trimmedContent.size() > MAX_REPAIR_CONTENT_LEN) {
        lastError = "报修内容不能超过" + std::to_string(MAX_REPAIR_CONTENT_LEN) + "个字符！";
        return false;
    }

    //校验处理状态
    if (repair.handleStatus != RepairStatus::UNHANDLED &&
        repair.handleStatus != RepairStatus::HANDLING &&
        repair.handleStatus != RepairStatus::COMPLETED) {
        lastError = "处理状态错误！";
        return false;
    }

    return true;
}

//辅助函数：将查询结果行转换为Repair对象 
Repair RepairManager::rowToRepair(const std::map<std::string, std::string>& row) {
    Repair repair;

    //报修ID（必存在）
    repair.repairId = row.at("repair_id");

    //学号（必存在）
    repair.studentId = row.at("student_id");

    //宿舍号（必存在）
    repair.dormId = row.at("dorm_id");

    //报修内容（必存在）
    repair.repairContent = row.at("repair_content");

    //报修日期（必存在）
    std::string repairDateStr = row.at("repair_date");
    repair.repairDate = Common::stringToDate(repairDateStr);

    //处理状态（必存在，int转枚举）
    int statusInt = 0;
    Common::stringToInt(row.at("handle_status"), statusInt);
    if (statusInt == 1) repair.handleStatus = RepairStatus::HANDLING;
    else if (statusInt == 2) repair.handleStatus = RepairStatus::COMPLETED;
    else repair.handleStatus = RepairStatus::UNHANDLED;

    //处理日期
    std::string handleDateStr = row.at("handle_date");
    //调试：可以在这里添加日志输出，查看从数据库读取的实际值
    
    if (!handleDateStr.empty() && handleDateStr != "NULL") {
        repair.handleDate = Common::stringToDate(handleDateStr);
    } else {
        repair.handleDate = Date(0, 0, 0);
    }

    return repair;
}

//辅助函数：生成报修ID
std::string RepairManager::generateRepairId() {
    //查询当前最大报修ID
    std::string sql = "SELECT MAX(repair_id) AS max_id FROM repair";

    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) {
        return "1"; //查询失败时默认返回1
    }

    //2. 解析最大序号并+1
    int maxSeq = 0;
    if (result->rowCount > 0 && !result->rows[0]["max_id"].empty()) {
        std::string maxId = result->rows[0]["max_id"];
        Common::stringToInt(maxId, maxSeq);
    }

    DBHelper::getInstance().freeResultset(result);
    maxSeq++;

    //3. 返回纯数字ID
    return std::to_string(maxSeq);
}

//辅助函数：校验学号是否存在 
bool RepairManager::isStudentExist(const std::string& studentId) {
    std::string trimmedId = Common::trim(studentId);
    if (trimmedId.empty()) return false;

    std::string sql = "SELECT student_id FROM student WHERE student_id = '" + trimmedId + "' LIMIT 1";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) return false;

    bool exist = result->rowCount > 0;
    DBHelper::getInstance().freeResultset(result);
    return exist;
}

//辅助函数：校验宿舍号是否存在 
bool RepairManager::isDormExist(const std::string& dormId) {
    std::string trimmedId = Common::trim(dormId);
    if (trimmedId.empty()) return false;

    std::string sql = "SELECT dorm_id FROM dorm WHERE dorm_id = '" + trimmedId + "' LIMIT 1";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) return false;

    bool exist = result->rowCount > 0;
    DBHelper::getInstance().freeResultset(result);
    return exist;
}

//辅助函数：校验报修状态流转合法性 
bool RepairManager::isValidStatusTransition(RepairStatus prevStatus, RepairStatus newStatus) {
    //允许流转：未处理→处理中、处理中→已完成、未处理→已完成；不允许逆向流转
    if (prevStatus == RepairStatus::UNHANDLED && newStatus == RepairStatus::HANDLING) return true;
    if (prevStatus == RepairStatus::HANDLING && newStatus == RepairStatus::COMPLETED) return true;
    if (prevStatus == RepairStatus::UNHANDLED && newStatus == RepairStatus::COMPLETED) return true; //允许直接完成
    return false;
}

//提交报修申请实现 
bool RepairManager::addRepair(const Repair& repair) {
    lastError.clear();

    //校验数据合法性（添加模式）
    if (!validateRepair(repair, true)) {
        return false;
    }

    //生成报修ID
    std::string repairId = generateRepairId();
    if (repairId.empty()) {
        lastError = "提交失败：报修ID生成失败！";
        return false;
    }

    //构建INSERT SQL（处理空值）
    std::ostringstream sqlStream;
    std::string trimmedContent = Common::trim(repair.repairContent);

    std::string repairDateStr = Common::dateToString(repair.repairDate);

    sqlStream << "INSERT INTO repair (repair_id, student_id, dorm_id, repair_content, repair_date, "
        << "handle_status, handle_date) "
        << "VALUES ("
        << repairId << ", "
        << "'" << Common::trim(repair.studentId) << "', "
        << "'" << Common::trim(repair.dormId) << "', "
        << "'" << trimmedContent << "', "
        << "'" << repairDateStr << "', "
        << static_cast<int>(RepairStatus::UNHANDLED) << ", "; //新增报修默认未处理

    //处理日期：未处理状态存NULL
    sqlStream << "NULL)";

    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "提交失败：" + dbErr.errorMsg;
        return false;
    }

    return affectedRows >= 0;
}

//修改报修信息实现 
bool RepairManager::updateRepair(const Repair& repair) {
    lastError.clear();

    //校验数据合法性（修改模式）
    if (!validateRepair(repair, false)) {
        return false;
    }

    //检查报修记录是否存在
    Repair existRepair = getRepairById(repair.repairId);
    if (existRepair.repairId.empty()) {
        lastError = "修改失败：未查询到报修ID" + repair.repairId + "对应的记录！";
        return false;
    }

    //仅允许修改未处理的报修
    if (existRepair.handleStatus != RepairStatus::UNHANDLED) {
        lastError = "修改失败：已处理/处理中的报修不允许修改基础信息！";
        return false;
    }

    //构建UPDATE SQL
    std::ostringstream sqlStream;
    std::string trimmedContent = Common::trim(repair.repairContent);
   

    sqlStream << "UPDATE repair SET "
        << "student_id = '" << Common::trim(repair.studentId) << "', "
        << "dorm_id = '" << Common::trim(repair.dormId) << "', "
        << "repair_content = '" << trimmedContent << "', ";

   

    sqlStream << "WHERE repair_id = " << Common::trim(repair.repairId);

    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "修改失败：" + dbErr.errorMsg;
        return false;
    }

    return affectedRows >= 0;
}

//更新报修处理状态实现 
bool RepairManager::updateHandleStatus(const std::string& repairId, RepairStatus newStatus) {
    lastError.clear();

    //校验参数
    std::string trimmedId = Common::trim(repairId);
    if (trimmedId.empty()) {
        lastError = "报修ID不能为空！";
        return false;
    }
    
    if (trimmedId[0] == 'R' && trimmedId.size() >= 10) {
    } else if (std::all_of(trimmedId.begin(), trimmedId.end(), ::isdigit)) {
        //纯数字格式，不做额外处理
    } else {
        lastError = "报修ID格式错误！";
        return false;
    }
    if (newStatus != RepairStatus::HANDLING && newStatus != RepairStatus::COMPLETED) {
        lastError = "仅支持更新为处理中或已完成状态！";
        return false;
    }

    //检查报修记录是否存在
    Repair existRepair = getRepairById(trimmedId);
    if (existRepair.repairId.empty()) {
        lastError = "更新失败：未查询到报修ID" + trimmedId + "对应的记录！";
        return false;
    }

    //校验状态流转合法性
    if (!isValidStatusTransition(existRepair.handleStatus, newStatus)) {
        std::string prevStatusStr = (existRepair.handleStatus == RepairStatus::UNHANDLED) ? "未处理" :
            (existRepair.handleStatus == RepairStatus::HANDLING) ? "处理中" : "已完成";
        std::string newStatusStr = (newStatus == RepairStatus::HANDLING) ? "处理中" : "已完成";
        lastError = "状态流转错误：" + prevStatusStr + "不能直接转为" + newStatusStr + "！";
        return false;
    }

    //构建UPDATE SQL
    std::ostringstream sqlStream;
    Date now;
    std::string handleDateStr = Common::dateToString(now);

    sqlStream << "UPDATE repair SET "
        << "handle_status = " << static_cast<int>(newStatus) << ", "
        << "handle_date = '" << handleDateStr << "' "
        << "WHERE repair_id = " << trimmedId;

    std::string sqlStr = sqlStream.str();
    //执行SQL
    std::string sql = sqlStream.str();
    int affectedRows = DBHelper::getInstance().executeUpdate(sql);
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "更新失败：" + dbErr.errorMsg;
        return false;
    }
    
    //检查是否有行被更新
    if (affectedRows == 0) {
        lastError = "更新失败：未找到匹配的记录";
        return false;
    }

    //强制刷新数据库缓存，确保Navicat等工具能立即看到更新
    DBHelper::getInstance().executeUpdate("FLUSH TABLES");
    
    //延迟一段时间，确保数据库更新完成
    Common::delay(200); //延迟200毫秒

    return true; //只要SQL执行成功且返回受影响行数>0，就认为更新成功
}

//删除报修记录实现 
bool RepairManager::deleteRepair(const std::string& repairId) {
    lastError.clear();

    //校验参数
    std::string trimmedId = Common::trim(repairId);
    if (trimmedId.empty()) {
        lastError = "报修ID不能为空！";
        return false;
    }
    
    if (trimmedId[0] == 'R' && trimmedId.size() >= 10) {
    } else if (std::all_of(trimmedId.begin(), trimmedId.end(), ::isdigit)) {
        //纯数字格式，不做额外处理
    } else {
        lastError = "报修ID格式错误！";
        return false;
    }

    //检查报修记录是否存在
    Repair existRepair = getRepairById(trimmedId);
    if (existRepair.repairId.empty()) {
        lastError = "删除失败：未查询到报修ID" + trimmedId + "对应的记录！";
        return false;
    }

    //仅允许删除未处理的报修
    if (existRepair.handleStatus != RepairStatus::UNHANDLED) {
        lastError = "删除失败：已处理/处理中的报修不允许删除！";
        return false;
    }

    //构建DELETE SQL
    std::ostringstream sqlStream;
    sqlStream << "DELETE FROM repair WHERE repair_id = " << trimmedId;

    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "删除失败：" + dbErr.errorMsg;
        return false;
    }

    return affectedRows >= 0;
}

//通过报修ID查询记录实现 
Repair RepairManager::getRepairById(const std::string& repairId) {
    lastError.clear();
    Repair emptyRepair;

    //校验参数
    std::string trimmedId = Common::trim(repairId);
    if (trimmedId.empty()) {
        lastError = "报修ID不能为空！";
        return emptyRepair;
    }
    
    if (trimmedId[0] == 'R' && trimmedId.size() >= 10) {
    } else if (std::all_of(trimmedId.begin(), trimmedId.end(), ::isdigit)) {
    } else {
        lastError = "报修ID格式错误！";
        return emptyRepair;
    }

    //构建查询SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT repair_id, student_id, dorm_id, repair_content, repair_date, "
        << "handle_status, handle_date "
        << "FROM repair WHERE repair_id = '" << trimmedId << "'";

    //执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return emptyRepair;
    }

    //解析结果集
    Repair repair;
    if (result->rowCount == 1) {
        repair = rowToRepair(result->rows[0]);
    }
    else {
        lastError = "未查询到报修ID" + trimmedId + "对应的记录！";
    }

    //释放结果集
    DBHelper::getInstance().freeResultset(result);
    return repair;
}

//分页查询所有报修记录实现 
std::vector<Repair> RepairManager::getAllRepairs(const PageParam& pageParam) {
    lastError.clear();
    std::vector<Repair> repairList;

    //校验分页参数
    if (pageParam.pageIndex < 1 || pageParam.pageSize < 1) {
        lastError = "分页参数错误：页码和每页条数必须大于0！";
        return repairList;
    }

    //构建分页查询SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT repair_id, student_id, dorm_id, repair_content, repair_date, "
        << "handle_status, handle_date "
        << "FROM repair "
        << "ORDER BY repair_id ASC "
        << "LIMIT " << pageParam.getOffset() << ", " << pageParam.pageSize;

    //执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return repairList;
    }

    //解析结果集
    for (const auto& row : result->rows) {
        repairList.push_back(rowToRepair(row));
    }

    //释放结果集
    DBHelper::getInstance().freeResultset(result);
    return repairList;
}

//多条件筛选查询实现 
std::vector<Repair> RepairManager::filterRepairs(const std::string& studentId, const std::string& dormId,
    RepairStatus handleStatus, const PageParam& pageParam) {
    lastError.clear();
    std::vector<Repair> repairList;

    //校验分页参数
    if (pageParam.pageIndex < 1 || pageParam.pageSize < 1) {
        lastError = "分页参数错误：页码和每页条数必须大于0！";
        return repairList;
    }

    //构建筛选条件SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT repair_id, student_id, dorm_id, repair_content, repair_date, "
        << "handle_status, handle_date "
        << "FROM repair WHERE 1=1 ";

    //学号筛选
    std::string trimmedStuId = Common::trim(studentId);
    if (!trimmedStuId.empty()) {
        sqlStream << "AND student_id = '" << trimmedStuId << "' ";
    }

    //宿舍号筛选
    std::string trimmedDormId = Common::trim(dormId);
    if (!trimmedDormId.empty()) {
        sqlStream << "AND dorm_id = '" << trimmedDormId << "' ";
    }

    //处理状态筛选（-1表示不筛选，0=未处理/1=处理中/2=已完成）
    if (handleStatus == RepairStatus::UNHANDLED ||
        handleStatus == RepairStatus::HANDLING ||
        handleStatus == RepairStatus::COMPLETED) {
        sqlStream << "AND handle_status = " << static_cast<int>(handleStatus) << " ";
    }

    sqlStream << "ORDER BY repair_id ASC "
        << "LIMIT " << pageParam.getOffset() << ", " << pageParam.pageSize;

    //执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return repairList;
    }

    //解析结果集
    for (const auto& row : result->rows) {
        repairList.push_back(rowToRepair(row));
    }

    //释放结果集
    DBHelper::getInstance().freeResultset(result);
    return repairList;
}

//获取报修记录总数实现 
int RepairManager::getRepairTotalCount() {
    lastError.clear();

    std::string sql = "SELECT COUNT(*) AS total FROM repair";
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

//获取未处理报修记录总数实现 
int RepairManager::getUnfinishedRepairCount() {
    lastError.clear();

    std::string sql = "SELECT COUNT(*) AS total FROM repair WHERE handle_status != 2";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "获取未处理报修总数失败：" + dbErr.errorMsg;
        return 0;
    }

    int totalCount = 0;
    if (result->rowCount > 0) {
        Common::stringToInt(result->rows[0]["total"], totalCount);
    }

    DBHelper::getInstance().freeResultset(result);
    return totalCount;
}

//获取筛选条件下的记录总数实现 
int RepairManager::getFilterTotalCount(const std::string& studentId, const std::string& dormId,
    RepairStatus handleStatus) {
    lastError.clear();

    std::ostringstream sqlStream;
    sqlStream << "SELECT COUNT(*) AS total FROM repair WHERE 1=1 ";

    //学号筛选
    std::string trimmedStuId = Common::trim(studentId);
    if (!trimmedStuId.empty()) {
        sqlStream << "AND student_id = '" << trimmedStuId << "' ";
    }

    //宿舍号筛选
    std::string trimmedDormId = Common::trim(dormId);
    if (!trimmedDormId.empty()) {
        sqlStream << "AND dorm_id = '" << trimmedDormId << "' ";
    }

    //处理状态筛选
    if (handleStatus == RepairStatus::UNHANDLED ||
        handleStatus == RepairStatus::HANDLING ||
        handleStatus == RepairStatus::COMPLETED) {
        sqlStream << "AND handle_status = " << static_cast<int>(handleStatus) << " ";
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

//获取最后一次操作错误信息实现 
std::string RepairManager::getLastError() const {
    return lastError;
}