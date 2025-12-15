#include "MultiTableQueryManager.h"
#include <sstream>
#include <iomanip>

// 查询学生-宿舍-费用综合信息
std::vector<StudentDormFeeInfo> MultiTableQueryManager::queryStudentDormFee(
    const std::string& studentId, const std::string& dormId, const std::string& feeMonth, 
    PayStatus payStatus, const PageParam& pageParam) {
    
    std::vector<StudentDormFeeInfo> result;
    
    try {
        DBHelper& dbHelper = DBHelper::getInstance();
        
        // 构建SQL查询语句，连接学生、宿舍、费用表
        std::string sql = "SELECT s.student_id, s.student_name, s.gender, s.major, "
                         "s.dorm_id, d.building, d.room_type, f.fee_id, f.fee_month, "
                         "f.water_fee, f.electric_fee, f.total_fee, f.pay_status, f.pay_date "
                         "FROM student s "
                         "LEFT JOIN dorm d ON s.dorm_id = d.dorm_id "
                         "LEFT JOIN fee f ON s.student_id = f.student_id "
                         "WHERE 1=1";
        
        // 添加条件
        if (!studentId.empty()) {
            sql += " AND s.student_id LIKE '%" + studentId + "%'";
        }
        if (!dormId.empty()) {
            sql += " AND s.dorm_id LIKE '%" + dormId + "%'";
        }
        if (!feeMonth.empty()) {
            sql += " AND f.fee_month = '" + feeMonth + "'";
        }
        // 注意：payStatus是枚举类型，需要转换为整数
        sql += " AND (f.pay_status = " + std::to_string(static_cast<int>(payStatus)) + " OR f.pay_status IS NULL)";
        
        // 添加分页
        sql += " LIMIT " + std::to_string(pageParam.pageSize) + 
               " OFFSET " + std::to_string(pageParam.getOffset());
        
        // 执行查询
        DBResultset* resultSet = dbHelper.executeQuery(sql);
        
        if (resultSet) {
            // 转换结果
            for (const auto& row : resultSet->rows) {
                StudentDormFeeInfo info = rowToStudentDormFeeInfo(row);
                result.push_back(info);
            }
            
            dbHelper.freeResultset(resultSet);
        } else {
            lastError = "查询失败: " + dbHelper.getLastError().errorMsg;
        }
    } catch (const std::exception& e) {
        lastError = "查询异常: " + std::string(e.what());
    }
    
    return result;
}

// 获取最后一次操作错误信息
std::string MultiTableQueryManager::getLastError() const {
    return lastError;
}

// 辅助函数：将查询结果行转换为StudentDormFeeInfo对象
StudentDormFeeInfo MultiTableQueryManager::rowToStudentDormFeeInfo(const std::map<std::string, std::string>& row) {
    StudentDormFeeInfo info;
    
    //设置学生和宿舍信息
    info.studentId = row.count("student_id") ? row.at("student_id") : "";
    info.studentName = row.count("student_name") ? row.at("student_name") : "";
    info.gender = row.count("gender") ? row.at("gender") : "";
    info.major = row.count("major") ? row.at("major") : "";
    info.dormId = row.count("dorm_id") ? row.at("dorm_id") : "";
    info.building = row.count("building") ? row.at("building") : "";
    info.roomType = row.count("room_type") ? row.at("room_type") : "";
    
    //设置费用信息
    info.feeMonth = row.count("fee_month") ? row.at("fee_month") : "";
    info.waterFee = row.count("water_fee") ? std::stod(row.at("water_fee")) : 0.0;
    info.electricFee = row.count("electric_fee") ? std::stod(row.at("electric_fee")) : 0.0;
    info.totalFee = row.count("total_fee") ? std::stod(row.at("total_fee")) : 0.0;
    
    // 设置支付状态
    if (row.count("pay_status")) {
        info.payStatus = static_cast<PayStatus>(std::stoi(row.at("pay_status")));
    } else {
        info.payStatus = PayStatus::UNPAID;
    }
    
    // 设置支付日期
    if (row.count("pay_date") && !row.at("pay_date").empty()) {
        info.payDate = Common::stringToDate(row.at("pay_date"));
    }
    
    return info;
}