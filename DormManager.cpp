#include "DormManager.h"
#include "Common.h"
#include "DBHelper.h"
#include <sstream>
#include <algorithm>

//校验宿舍数据合法性
bool DormManager::validateDorm(const Dorm& dorm) {
    lastError.clear();

    //校验宿舍号
    std::string trimmedId = Common::trim(dorm.dormId);
    if (trimmedId.empty()) {
        lastError = "宿舍号不能为空！";
        return false;
    }
    if (!Common::isValidDormID(trimmedId)) {
        lastError = "宿舍号格式错误（3-10位，格式如1-101）！";
        return false;
    }
    //校验楼栋
    std::string trimmedBuilding = Common::trim(dorm.building);
    if (trimmedBuilding.empty()) {
        lastError = "楼栋不能为空！";
        return false;
    }
    if (!Common::isValidBuilding(trimmedBuilding)) {
        lastError = "楼栋格式错误（1-10位中文/数字）！";
        return false;
    }

    //校验房间类型
    std::string trimmedType = Common::trim(dorm.roomType);
    if (trimmedType.empty()) {
        lastError = "房间类型不能为空！";
        return false;
    }
    if (trimmedType != "4" && trimmedType != "6" && trimmedType != "8") {
        lastError = "房间类型错误（仅支持数字4、6、8）！";
        return false;
    }

    //校验最大容纳人数
    if (dorm.maxCapacity <= 0) {
        lastError = "最大容纳人数必须大于0！";
        return false;
    }
    //验证人数与房间类型一致性
    int defaultMax = getDefaultMaxCapacity(trimmedType);
    if (dorm.maxCapacity != defaultMax) {
        lastError = trimmedType + "最大容纳人数应为" + std::to_string(defaultMax) + "人！";
        return false;
    }

    //校验当前入住人数
    if (dorm.currentOccupancy < 0) {
        lastError = "当前入住人数不能为负数！";
        return false;
    }
    if (dorm.currentOccupancy > dorm.maxCapacity) {
        lastError = "当前入住人数不能超过最大容纳人数（" + std::to_string(dorm.maxCapacity) + "人）！";
        return false;
    }

    //校验宿管联系方式
    std::string trimmedManager = Common::trim(dorm.dormManager);
    if (!trimmedManager.empty() && !Common::isValidPhone(trimmedManager)) {
        lastError = "宿管联系方式格式错误（11位数字，以13/14/15/17/18/19开头）！";
        return false;
    }

    return true;
}

//将查询结果行转换为Dorm对象
Dorm DormManager::rowToDorm(const std::map<std::string, std::string>& row) {
    Dorm dorm;
    //宿舍号
    dorm.dormId = row.at("dorm_id");
    //楼栋
    dorm.building = row.at("building");
    //房间类型
    dorm.roomType = row.at("room_type");
    //最大容纳人数
    Common::stringToInt(row.at("max_capacity"), dorm.maxCapacity);
    //当前入住人数
    Common::stringToInt(row.at("current_occupancy"), dorm.currentOccupancy);
    //宿管联系方式
    dorm.dormManager = row.at("dorm_manager");
    return dorm;
}

//检查宿舍是否关联学生
bool DormManager::isDormRelatedToStudent(const std::string& dormId) {
    std::string trimmedId = Common::trim(dormId);
    if (trimmedId.empty()) return false;

    //查询SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT COUNT(*) AS count FROM student WHERE dorm_id = '" << trimmedId << "' LIMIT 1";

    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) return false;

    int relatedCount = 0;
    if (result->rowCount > 0) {
        Common::stringToInt(result->rows[0]["count"], relatedCount);
    }

    DBHelper::getInstance().freeResultset(result);
    return relatedCount > 0;
}

//根据房间类型获取默认最大容纳人数
int DormManager::getDefaultMaxCapacity(const std::string& roomType) {
    if (roomType == "4") return 4;
    if (roomType == "6") return 6;
    if (roomType == "8") return 8;
    return 0;
}

//添加宿舍实现
bool DormManager::addDorm(const Dorm& dorm) {
    lastError.clear();
    //校验宿舍数据合法性
    if (!validateDorm(dorm)) {
        return false;
    }
    //检查宿舍号是否已存在
    Dorm existDorm = getDormById(dorm.dormId);
    if (!existDorm.dormId.empty()) {
        lastError = "添加失败：宿舍号" + dorm.dormId + "已存在！";
        return false;
    }
    //构建INSERT SQL（处理空值）
    std::ostringstream sqlStream;
    std::string trimmedManager = Common::trim(dorm.dormManager);

    sqlStream << "INSERT INTO dorm (dorm_id, building, room_type, max_capacity, current_occupancy, dorm_manager) "
        << "VALUES ("
        << "'" << Common::trim(dorm.dormId) << "', "
        << "'" << Common::trim(dorm.building) << "', "
        << "'" << Common::trim(dorm.roomType) << "', "
        << dorm.maxCapacity << ", "
        << dorm.currentOccupancy << ", ";

    //宿管联系方式空值处理
    if (trimmedManager.empty()) {
        sqlStream << "NULL";
    }
    else {
        sqlStream << "'" << trimmedManager << "'";
    }

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

//修改宿舍信息实现
bool DormManager::updateDorm(const Dorm& dorm) {
    lastError.clear();

    //校验宿舍数据合法性
    if (!validateDorm(dorm)) {
        return false;
    }

    //检查宿舍是否存在
    Dorm existDorm = getDormById(dorm.dormId);
    if (existDorm.dormId.empty()) {
        lastError = "修改失败：未查询到宿舍号" + dorm.dormId + "对应的宿舍！";
        return false;
    }
    //构建UPDATE SQL
    std::ostringstream sqlStream;
    std::string trimmedManager = Common::trim(dorm.dormManager);

    sqlStream << "UPDATE dorm SET "
        << "building = '" << Common::trim(dorm.building) << "', "
        << "room_type = '" << Common::trim(dorm.roomType) << "', "
        << "max_capacity = " << dorm.maxCapacity << ", "
        << "current_occupancy = " << dorm.currentOccupancy << ", ";
    //宿管联系方式空值处理
    if (trimmedManager.empty()) {
        sqlStream << "dorm_manager = NULL ";
    }
    else {
        sqlStream << "dorm_manager = '" << trimmedManager << "' ";
    }
    sqlStream << "WHERE dorm_id = '" << Common::trim(dorm.dormId) << "'";
    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "修改失败：" + dbErr.errorMsg;
        return false;
    }
    return affectedRows >= 0;
}
//删除宿舍实现
bool DormManager::deleteDorm(const std::string& dormId) {
    lastError.clear();

    //校验宿舍号参数
    std::string trimmedId = Common::trim(dormId);
    if (trimmedId.empty()) {
        lastError = "宿舍号不能为空！";
        return false;
    }
    if (!Common::isValidDormID(trimmedId)) {
        lastError = "宿舍号格式错误！";
        return false;
    }

    //检查宿舍是否存在
    Dorm existDorm = getDormById(trimmedId);
    if (existDorm.dormId.empty()) {
        lastError = "删除失败：未查询到宿舍号" + trimmedId + "对应的宿舍！";
        return false;
    }

    //检查宿舍是否关联学生（关联则不允许删除）
    if (isDormRelatedToStudent(trimmedId)) {
        lastError = "删除失败：该宿舍仍有关联学生，请先处理学生入住信息！";
        return false;
    }

    //构建DELETE SQL
    std::ostringstream sqlStream;
    sqlStream << "DELETE FROM dorm WHERE dorm_id = '" << trimmedId << "'";

    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "删除失败：" + dbErr.errorMsg;
        return false;
    }

    return affectedRows >= 0;
}

//通过宿舍号查询宿舍实现
Dorm DormManager::getDormById(const std::string& dormId) {
    lastError.clear();
    Dorm emptyDorm;

    //校验宿舍号参数
    std::string trimmedId = Common::trim(dormId);
    if (trimmedId.empty() || !Common::isValidDormID(trimmedId)) {
        lastError = "宿舍号格式错误！";
        return emptyDorm;
    }
    //构建查询SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT dorm_id, building, room_type, max_capacity, current_occupancy, dorm_manager "
        << "FROM dorm WHERE dorm_id = '" << trimmedId << "'";
    //执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return emptyDorm;
    }
    //解析结果集
    Dorm dorm;
    if (result->rowCount == 1) {
        dorm = rowToDorm(result->rows[0]);
    }
    else {
        lastError = "未查询到宿舍号" + trimmedId + "对应的宿舍！";
    }
    //释放结果集
    DBHelper::getInstance().freeResultset(result);
    return dorm;
}
//分页查询所有宿舍实现
std::vector<Dorm> DormManager::getAllDorms(const PageParam& pageParam) {
    lastError.clear();
    std::vector<Dorm> dormList;
    //校验分页参数
    if (pageParam.pageIndex < 1 || pageParam.pageSize < 1) {
        lastError = "分页参数错误：页码和每页条数必须大于0！";
        return dormList;
    }
    //构建分页查询SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT dorm_id, building, room_type, max_capacity, current_occupancy, dorm_manager "
        << "FROM dorm "
        << "ORDER BY dorm_id ASC "
        << "LIMIT " << pageParam.getOffset() << ", " << pageParam.pageSize;
    //执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return dormList;
    }
    //解析结果集
    for (const auto& row : result->rows) {
        dormList.push_back(rowToDorm(row));
    }
    //释放结果集
    DBHelper::getInstance().freeResultset(result);
    return dormList;
}
//按楼栋筛选宿舍实现
std::vector<Dorm> DormManager::filterDormsByBuilding(const std::string& building, const PageParam& pageParam) {
    lastError.clear();
    std::vector<Dorm> dormList;

    //校验参数
    std::string trimmedBuilding = Common::trim(building);
    if (trimmedBuilding.empty()) {
        lastError = "楼栋名称不能为空！";
        return dormList;
    }
    if (pageParam.pageIndex < 1 || pageParam.pageSize < 1) {
        lastError = "分页参数错误：页码和每页条数必须大于0！";
        return dormList;
    }
    //构建筛选查询SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT dorm_id, building, room_type, max_capacity, current_occupancy, dorm_manager "
        << "FROM dorm "
        << "WHERE building LIKE '%" << trimmedBuilding << "%' "
        << "ORDER BY dorm_id ASC "
        << "LIMIT " << pageParam.getOffset() << ", " << pageParam.pageSize;
    //执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return dormList;
    }

    //4. 解析结果集
    for (const auto& row : result->rows) {
        dormList.push_back(rowToDorm(row));
    }

    //5. 释放结果集
    DBHelper::getInstance().freeResultset(result);
    return dormList;
}

// 获取宿舍总数实现 
int DormManager::getDormTotalCount() {
    lastError.clear();

    //构建查询总数SQL
    std::string sql = "SELECT COUNT(*) AS total FROM dorm";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "获取总数失败：" + dbErr.errorMsg;
        return 0;
    }

    //解析结果
    int totalCount = 0;
    if (result->rowCount > 0) {
        Common::stringToInt(result->rows[0]["total"], totalCount);
    }

    //释放结果集
    DBHelper::getInstance().freeResultset(result);
    return totalCount;
}

// 获取指定楼栋的宿舍总数实现 
int DormManager::getBuildingDormCount(const std::string& building) {
    lastError.clear();

    //校验楼栋参数
    std::string trimmedBuilding = Common::trim(building);
    if (trimmedBuilding.empty()) {
        lastError = "楼栋名称不能为空！";
        return 0;
    }

    //构建查询总数SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT COUNT(*) AS total FROM dorm "
        << "WHERE building LIKE '%" << trimmedBuilding << "%'";

    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "获取总数失败：" + dbErr.errorMsg;
        return 0;
    }

    //解析结果
    int totalCount = 0;
    if (result->rowCount > 0) {
        Common::stringToInt(result->rows[0]["total"], totalCount);
    }

    //释放结果集
    DBHelper::getInstance().freeResultset(result);
    return totalCount;
}

// 更新宿舍当前入住人数实现 
bool DormManager::updateCurrentCount(const std::string& dormId, int changeNum) {
    lastError.clear();

    //1. 校验参数
    std::string trimmedId = Common::trim(dormId);
    if (trimmedId.empty() || !Common::isValidDormID(trimmedId)) {
        lastError = "宿舍号格式错误！";
        return false;
    }
    if (changeNum == 0) {
        lastError = "变更人数不能为0！";
        return false;
    }

    //2. 检查宿舍是否存在
    Dorm existDorm = getDormById(trimmedId);
    if (existDorm.dormId.empty()) {
        lastError = "更新失败：未查询到宿舍号" + trimmedId + "对应的宿舍！";
        return false;
    }

    //3. 计算新的入住人数（确保非负且不超过最大容量）
    int newCount = existDorm.currentOccupancy + changeNum;
    if (newCount < 0) {
        lastError = "更新失败：当前入住人数不能为负数！";
        return false;
    }
    if (newCount > existDorm.maxCapacity) {
        lastError = "更新失败：当前入住人数不能超过最大容纳人数（" + std::to_string(existDorm.maxCapacity) + "人）！";
        return false;
    }

    //构建UPDATE SQL
    std::ostringstream sqlStream;
    sqlStream << "UPDATE dorm SET current_occupancy = " << newCount << " WHERE dorm_id = '" << trimmedId << "'";

    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "更新失败：" + dbErr.errorMsg;
        return false;
    }
    //显式提交事务，确保数据立即更新
    if (affectedRows > 0) {
        DBHelper::getInstance().commitTransaction();
    }

    return affectedRows >= 0;
}

//获取最后一次操作错误信息实现
std::string DormManager::getLastError() const {
    return lastError;
}
