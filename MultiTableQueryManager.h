#ifndef MULTITABLEQUERYMANAGER_H
#define MULTITABLEQUERYMANAGER_H

#include "Common.h"
#include "DBHelper.h"
#include <vector>
#include <string>
#include <map>

// 多表查询结果结构体，用于存储学生-宿舍-费用组合信息
struct StudentDormFeeInfo {
    std::string studentId;     // 学号
    std::string studentName;   // 学生姓名
    std::string gender;        // 性别
    std::string major;         // 专业
    std::string dormId;        // 宿舍号
    std::string building;      // 楼号
    std::string roomType;      // 宿舍类型
    std::string feeMonth;      // 费用月份
    double waterFee;           // 水费
    double electricFee;        // 电费
    double totalFee;           // 总费用
    PayStatus payStatus;       // 缴费状态
    Date payDate;             // 缴费日期

    // 默认构造函数
    StudentDormFeeInfo() : studentId(""), studentName(""), gender(""), major(""), 
        dormId(""), building(""), roomType(""), feeMonth(""),
        waterFee(0.0), electricFee(0.0), totalFee(0.0), 
        payStatus(PayStatus::UNPAID) {
    }

    // 带参数构造函数
    StudentDormFeeInfo(const std::string& stuId, const std::string& stuName, const std::string& gend,
        const std::string& maj, const std::string& dorm, const std::string& build, const std::string& type,
        const std::string& month, double water, double electric, double total,
        PayStatus status = PayStatus::UNPAID, const Date& payDt = Date())
        : studentId(stuId), studentName(stuName), gender(gend), major(maj),
        dormId(dorm), building(build), roomType(type), feeMonth(month),
        waterFee(water), electricFee(electric), totalFee(total),
        payStatus(status), payDate(payDt) {
    }
};



// 多表查询管理类
class MultiTableQueryManager {
public:
    // 查询学生-宿舍-费用综合信息
    std::vector<StudentDormFeeInfo> queryStudentDormFee(const std::string& studentId = "", 
        const std::string& dormId = "", const std::string& feeMonth = "", 
        PayStatus payStatus = PayStatus::UNPAID, const PageParam& pageParam = PageParam());

    // 获取最后一次操作错误信息
    std::string getLastError() const;

private:
    std::string lastError; // 存储最后一次操作错误描述

    // 辅助函数：将查询结果行转换为StudentDormFeeInfo对象
    StudentDormFeeInfo rowToStudentDormFeeInfo(const std::map<std::string, std::string>& row);
};

#endif // MULTITABLEQUERYMANAGER_H