#include "FeeManager.h"
#include "Common.h"
#include "DBHelper.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

bool FeeManager::validateFee(const Fee& fee, bool isAdd) {
    lastError.clear();

    //校验费用ID，自增
    if (!isAdd) {
        std::string trimmedId = Common::trim(fee.feeId);
        if (trimmedId.empty()) {
            lastError = "修改费用记录时，费用ID不能为空！";
            return false;
        }
    }
    //校验学号
    std::string trimmedStuId = Common::trim(fee.studentId);
    if (trimmedStuId.empty()) {
        lastError = "学号不能为空";
        return false;
    }
    if (!Common::isValidStudentID(trimmedStuId)) {
        lastError = "学号格式不正确";
        return false;
    }
    if (!isStudentExist(trimmedStuId)) {
        lastError = "学号" + trimmedStuId + "对应的学生不存在";
        return false;
    }
    //校验宿舍号
    std::string trimmedDormId = Common::trim(fee.dormId);
    if (trimmedDormId.empty()) {
        lastError = "宿舍号不能为空";
        return false;
    }
    if (!Common::isValidDormID(trimmedDormId)) {
        lastError = "宿舍号格式不正确";
        return false;
    }
    if (!isDormExist(trimmedDormId)) {
        lastError = "宿舍号" + trimmedDormId + "不存在";
        return false;
    }
    //校验费用月份
    std::string trimmedMonth = Common::trim(fee.feeMonth);
    if (trimmedMonth.empty()) {
        lastError = "费用月份不能为空";
        return false;
    }
    if (!Common::isValidFeeMonth(trimmedMonth)) {
        lastError = "费用月份格式不正确，应为YYYY-MM格式";
        return false;
    }
    //校验费用金额
    if (fee.waterFee < 0.0) {
        lastError = "水费金额不能为负数";
        return false;
    }
    if (fee.electricFee < 0.0) {
        lastError = "电费金额不能为负数";
        return false;
    }
    if (fee.waterFee + fee.electricFee < 0.0001) {
        lastError = "总费用不能为0";
        return false;
    }
    //校验缴费状态
    if (fee.payStatus != PayStatus::UNPAID && fee.payStatus != PayStatus::PAID) {
        lastError = "缴费状态不正确";
        return false;
    }
    //检查重复记录（仅添加时）
    if (isAdd && isFeeDuplicate(trimmedStuId, trimmedMonth)) {
        lastError = "学号" + trimmedStuId + "在" + trimmedMonth + "月份的费用记录已存在";
        return false;
    }

    return true;
}

//将数据库查询结果行转换为Fee对象
Fee FeeManager::rowToFee(const std::map<std::string, std::string>& row) {
    Fee fee;
    fee.feeId = row.at("fee_id");
    fee.studentId = row.at("student_id");
    fee.dormId = row.at("dorm_id");
    fee.feeMonth = row.at("fee_month");

    Common::stringToDouble(row.at("water_fee"), fee.waterFee);
    Common::stringToDouble(row.at("electric_fee"), fee.electricFee);
    Common::stringToDouble(row.at("total_fee"), fee.totalFee);
    int statusInt = 0;
    Common::stringToInt(row.at("pay_status"), statusInt);
    fee.payStatus = (statusInt == 1) ? PayStatus::PAID : PayStatus::UNPAID;
    std::string payDateStr = row.at("pay_date");
    if (!payDateStr.empty()) {
        fee.payDate = Common::stringToDate(payDateStr);
    }
    return fee;
}

//生成费用ID，自增
std::string FeeManager::generateFeeId() {
    Date now;
    std::ostringstream dateStream;
    dateStream << std::setfill('0') << std::setw(4) << now.year
        << std::setw(2) << now.month;
    std::string datePart = dateStream.str();

    //查询当前月份的最大序号
    std::ostringstream sqlStream;
    sqlStream << "SELECT MAX(fee_id) AS max_id FROM fee WHERE fee_id LIKE 'F" << datePart << "%'";

    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        return "F" + datePart + "0001"; //没有记录则从0001开始
    }

    //获取最大序号并加1
    int maxSeq = 0;
    if (result->rowCount > 0 && !result->rows[0]["max_id"].empty()) {
        std::string maxId = result->rows[0]["max_id"];
        if (maxId.size() >= 10) {
            std::string seqPart = maxId.substr(8); //提取后4位序号
            Common::stringToInt(seqPart, maxSeq);
        }
    }

    DBHelper::getInstance().freeResultset(result);
    maxSeq++;

    //生成4位序号
    std::ostringstream seqStream;
    seqStream << std::setfill('0') << std::setw(4) << maxSeq;
    return "F" + datePart + seqStream.str();
}

//检查学生是否存在
bool FeeManager::isStudentExist(const std::string& studentId) {
    std::string trimmedId = Common::trim(studentId);
    if (trimmedId.empty()) return false;

    std::string sql = "SELECT student_id FROM student WHERE student_id = '" + trimmedId + "' LIMIT 1";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) return false;

    bool exist = result->rowCount > 0;
    DBHelper::getInstance().freeResultset(result);
    return exist;
}

//检查宿舍是否存在
bool FeeManager::isDormExist(const std::string& dormId) {
    std::string trimmedId = Common::trim(dormId);
    if (trimmedId.empty()) return false;

    std::string sql = "SELECT dorm_id FROM dorm WHERE dorm_id = '" + trimmedId + "' LIMIT 1";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) return false;

    bool exist = result->rowCount > 0;
    DBHelper::getInstance().freeResultset(result);
    return exist;
}

//添加费用记录
bool FeeManager::addFee(const Fee& fee) {
    lastError.clear();

    //验证费用数据
    if (!validateFee(fee, true)) {
        return false;
    }

    //计算总费用
    double totalFee = fee.waterFee + fee.electricFee;

    //构建INSERT SQL
    std::ostringstream sqlStream;
    std::string payDateStr = Common::dateToString(fee.payDate);

    sqlStream << "INSERT INTO fee (student_id, dorm_id, fee_month, water_fee, electric_fee, total_fee, "
        << "pay_status, pay_date) "
        << "VALUES ("
        << "'" << Common::trim(fee.studentId) << "', "
        << "'" << Common::trim(fee.dormId) << "', "
        << "'" << Common::trim(fee.feeMonth) << "', "
        << fee.waterFee << ", "
        << fee.electricFee << ", "
        << totalFee << ", "
        << static_cast<int>(fee.payStatus) << ", ";

    //缴费日期：未缴则存NULL，已缴则存日期
    if (fee.payStatus == PayStatus::UNPAID) {
        sqlStream << "NULL";
    }
    else {
        sqlStream << "'" << payDateStr << "'";
    }

    sqlStream << ")";

    //SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "添加费用失败：" + dbErr.errorMsg;
        return false;
    }

    return affectedRows >= 0;
}

//更新费用记录
bool FeeManager::updateFee(const Fee& fee) {
    lastError.clear();

    //验证费用数据
    if (!validateFee(fee, false)) {
        return false;
    }

    //检查费用记录是否存在
    Fee existFee = getFeeById(fee.feeId);
    if (existFee.feeId.empty()) {
        lastError = "费用记录不存在，ID" + fee.feeId + "未找到";
        return false;
    }

    //检查是否已缴费，已缴费的不能修改
    if (existFee.payStatus == PayStatus::PAID) {
        lastError = "已缴费的费用记录不允许修改";
        return false;
    }

    //计算总费用
    double totalFee = fee.waterFee + fee.electricFee;

    //SQL
    std::ostringstream sqlStream;

    sqlStream << "UPDATE fee SET "
        << "student_id = '" << Common::trim(fee.studentId) << "', "
        << "dorm_id = '" << Common::trim(fee.dormId) << "', "
        << "fee_month = '" << Common::trim(fee.feeMonth) << "', "
        << "water_fee = " << fee.waterFee << ", "
        << "electric_fee = " << fee.electricFee << ", "
        << "total_fee = " << totalFee << ", ";

    sqlStream << "WHERE fee_id = " << Common::trim(fee.feeId);

    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "更新费用失败：" + dbErr.errorMsg;
        return false;
    }

    return affectedRows >= 0;
}

//更新缴费状态
bool FeeManager::updatePayStatus(const std::string& feeId, const Date& payDate) {
    lastError.clear();

    //验证费用ID - 现在支持纯数字ID
    std::string trimmedId = Common::trim(feeId);
    if (trimmedId.empty() || !Common::isValidNumber(trimmedId)) {
        lastError = "费用ID格式不正确，应为纯数字";
        return false;
    }

    //检查费用记录是否存在
    Fee existFee = getFeeById(trimmedId);
    if (existFee.feeId.empty()) {
        lastError = "费用记录不存在，ID" + trimmedId + "未找到";
        return false;
    }

    //检查是否已缴费，已缴费的不能重复缴费
    if (existFee.payStatus == PayStatus::PAID) {
        lastError = "该费用记录已缴费，不能重复缴费";
        return false;
    }

    //构建UPDATE SQL
    std::ostringstream sqlStream;
    std::string payDateStr;
    
    // 如果从未付变为已付，使用当前日期；如果传入的日期为空，也使用当前日期
    if (payDate.year == 0 && payDate.month == 0 && payDate.day == 0) {
        Date currentDate;
        payDateStr = Common::dateToString(currentDate);
    } else {
        payDateStr = Common::dateToString(payDate);
    }
    
    sqlStream << "UPDATE fee SET "
        << "pay_status = " << static_cast<int>(PayStatus::PAID) << ", "
        << "pay_date = '" << payDateStr << "' "
        << "WHERE fee_id = " << trimmedId;

    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "更新缴费状态失败：" + dbErr.errorMsg;
        return false;
    }

    return affectedRows >= 0;
}

//删除费用记录
bool FeeManager::deleteFee(const std::string& feeId) {
    lastError.clear();

    //验证费用ID
    std::string trimmedId = Common::trim(feeId);
    if (trimmedId.empty() || !Common::isValidNumber(trimmedId)) {
        lastError = "费用ID格式不正确，应为纯数字";
        return false;
    }

    //检查费用记录是否存在
    Fee existFee = getFeeById(trimmedId);
    if (existFee.feeId.empty()) {
        lastError = "费用记录不存在，ID" + trimmedId + "未找到";
        return false;
    }

    //检查是否已缴费，已缴费的不能删除
    if (existFee.payStatus == PayStatus::PAID) {
        lastError = "已缴费的费用记录不允许删除";
        return false;
    }

    //构建DELETE
    std::ostringstream sqlStream;
    sqlStream << "DELETE FROM fee WHERE fee_id = " << trimmedId;

    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "删除费用失败：" + dbErr.errorMsg;
        return false;
    }

    return affectedRows >= 0;
}

//根据ID获取费用记录
Fee FeeManager::getFeeById(const std::string& feeId) {
    lastError.clear();
    Fee emptyFee;

    //验证费用ID
    std::string trimmedId = Common::trim(feeId);
    if (trimmedId.empty() || !Common::isValidNumber(trimmedId)) {
        lastError = "费用ID格式不正确，应为纯数字";
        return emptyFee;
    }

    //构建查询SQL - fee_id 是 INT 类型，不需要单引号
    std::ostringstream sqlStream;
    sqlStream << "SELECT fee_id, student_id, dorm_id, fee_month, water_fee, electric_fee, total_fee, "
        << "pay_status, pay_date "
        << "FROM fee WHERE fee_id = " << trimmedId;

    //执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询费用失败：" + dbErr.errorMsg;
        return emptyFee;
    }

    //处理查询结果
    Fee fee;
    if (result->rowCount == 1) {
        fee = rowToFee(result->rows[0]);
    }
    else {
        lastError = "未找到费用记录，ID" + trimmedId + "不存在";
    }

    //释放结果集
    DBHelper::getInstance().freeResultset(result);
    return fee;
}

//获取所有费用记录
std::vector<Fee> FeeManager::getAllFees(const PageParam& pageParam) {
    lastError.clear();
    std::vector<Fee> feeList;

    //验证分页参数
    if (pageParam.pageIndex < 1 || pageParam.pageSize < 1) {
        lastError = "分页参数不正确，页码和页大小必须大于0";
        return feeList;
    }

    //构建查询SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT fee_id, student_id, dorm_id, fee_month, water_fee, electric_fee, total_fee, "
        << "pay_status, pay_date "
        << "FROM fee "
        << "ORDER BY fee_id ASC "
        << "LIMIT " << pageParam.getOffset() << ", " << pageParam.pageSize;

    //执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询费用列表失败：" + dbErr.errorMsg;
        return feeList;
    }

    //处理查询结果
    for (const auto& row : result->rows) {
        feeList.push_back(rowToFee(row));
    }

    //释放结果集
    DBHelper::getInstance().freeResultset(result);
    return feeList;
}

//筛选费用记录
std::vector<Fee> FeeManager::filterFees(const std::string& studentId, const std::string& dormId,
    PayStatus payStatus, const PageParam& pageParam) {
    lastError.clear();
    std::vector<Fee> feeList;

    if (pageParam.pageIndex < 1 || pageParam.pageSize < 1) {
        lastError = "分页参数不正确，页码和页大小必须大于0";
        return feeList;
    }

    std::ostringstream sqlStream;
    sqlStream << "SELECT fee_id, student_id, dorm_id, fee_month, water_fee, electric_fee, total_fee, "
        << "pay_status, pay_date "
        << "FROM fee WHERE 1=1 ";

    std::string trimmedStuId = Common::trim(studentId);
    if (!trimmedStuId.empty()) {
        sqlStream << "AND student_id = '" << trimmedStuId << "' ";
    }

    std::string trimmedDormId = Common::trim(dormId);
    if (!trimmedDormId.empty()) {
        sqlStream << "AND dorm_id = '" << trimmedDormId << "' ";
    }

    if (payStatus == PayStatus::UNPAID || payStatus == PayStatus::PAID) {
        sqlStream << "AND pay_status = " << static_cast<int>(payStatus) << " ";
    }

    sqlStream << "ORDER BY fee_month DESC, fee_id ASC "
        << "LIMIT " << pageParam.getOffset() << ", " << pageParam.pageSize;

    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "筛选费用失败：" + dbErr.errorMsg;
        return feeList;
    }

    for (const auto& row : result->rows) {
        feeList.push_back(rowToFee(row));
    }

    DBHelper::getInstance().freeResultset(result);
    return feeList;
}

std::vector<Fee> FeeManager::getFeesByMonth(const std::string& feeMonth, const PageParam& pageParam) {
    lastError.clear();
    std::vector<Fee> feeList;

    std::string trimmedMonth = Common::trim(feeMonth);
    if (trimmedMonth.empty() || !Common::isValidFeeMonth(trimmedMonth)) {
        lastError = "费用月份格式不正确，应为YYYY-MM格式";
        return feeList;
    }
    if (pageParam.pageIndex < 1 || pageParam.pageSize < 1) {
        lastError = "分页参数不正确";
        return feeList;
    }

    std::ostringstream sqlStream;
    sqlStream << "SELECT fee_id, student_id, dorm_id, fee_month, water_fee, electric_fee, total_fee, "
        << "pay_status, pay_date "
        << "FROM fee "
        << "WHERE fee_month = '" << trimmedMonth << "' "
        << "ORDER BY fee_id ASC "
        << "LIMIT " << pageParam.getOffset() << ", " << pageParam.pageSize;

    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "按月份查询费用失败：" + dbErr.errorMsg;
        return feeList;
    }

    for (const auto& row : result->rows) {
        feeList.push_back(rowToFee(row));
    }

    DBHelper::getInstance().freeResultset(result);
    return feeList;
}

int FeeManager::getFeeTotalCount() {
    lastError.clear();

    std::string sql = "SELECT COUNT(*) AS total FROM fee";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "获取费用总数失败：" + dbErr.errorMsg;
        return 0;
    }

    int totalCount = 0;
    if (result->rowCount > 0) {
        Common::stringToInt(result->rows[0]["total"], totalCount);
    }

    DBHelper::getInstance().freeResultset(result);
    return totalCount;
}

int FeeManager::getFilterTotalCount(const std::string& studentId, const std::string& dormId, PayStatus payStatus) {
    lastError.clear();

    std::ostringstream sqlStream;
    sqlStream << "SELECT COUNT(*) AS total FROM fee WHERE 1=1 ";

    std::string trimmedStuId = Common::trim(studentId);
    if (!trimmedStuId.empty()) {
        sqlStream << "AND student_id = '" << trimmedStuId << "' ";
    }

    std::string trimmedDormId = Common::trim(dormId);
    if (!trimmedDormId.empty()) {
        sqlStream << "AND dorm_id = '" << trimmedDormId << "' ";
    }

    if (payStatus == PayStatus::UNPAID || payStatus == PayStatus::PAID) {
        sqlStream << "AND pay_status = " << static_cast<int>(payStatus) << " ";
    }

    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "获取筛选总数失败：" + dbErr.errorMsg;
        return 0;
    }

    int totalCount = 0;
    if (result->rowCount > 0) {
        Common::stringToInt(result->rows[0]["total"], totalCount);
    }

    DBHelper::getInstance().freeResultset(result);
    return totalCount;
}

bool FeeManager::isFeeDuplicate(const std::string& studentId, const std::string& feeMonth) {
    std::string trimmedStuId = Common::trim(studentId);
    std::string trimmedMonth = Common::trim(feeMonth);
    if (trimmedStuId.empty() || trimmedMonth.empty()) return false;

    std::ostringstream sqlStream;
    sqlStream << "SELECT fee_id FROM fee WHERE student_id = '" << trimmedStuId
        << "' AND fee_month = '" << trimmedMonth << "' LIMIT 1";

    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) return false;

    bool duplicate = result->rowCount > 0;
    DBHelper::getInstance().freeResultset(result);
    return duplicate;
}

std::string FeeManager::getLastError() const {
    return lastError;
}

int FeeManager::getUnpaidFeeCount() {
    lastError.clear();

    std::string sql = "SELECT COUNT(*) AS total FROM fee WHERE pay_status = 0";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "获取未支付费用数量失败：" + dbErr.errorMsg;
        return 0;
    }

    int totalCount = 0;
    if (result->rowCount > 0) {
        Common::stringToInt(result->rows[0]["total"], totalCount);
    }

    DBHelper::getInstance().freeResultset(result);
    return totalCount;
}