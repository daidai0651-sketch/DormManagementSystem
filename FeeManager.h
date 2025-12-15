#ifndef FEEMANAGER_H
#define FEEMANAGER_H

#include "Common.h"
#include "DBHelper.h"
#include <vector>
#include <string>

// --------------- 水电费结构体（与MySQL fee表字段一一对应）---------------
// 修改Fee结构体
struct Fee {
    std::string feeId;        // 费用ID，对应fee.fee_id
    std::string studentId;    // 学号，对应fee.student_id
    std::string dormId;       // 宿舍号，对应fee.dorm_id
    std::string feeMonth;     // 费用月份，对应fee.fee_month
    double waterFee;          // 水费，对应fee.water_fee
    double electricFee;       // 电费，对应fee.electric_fee
    double totalFee;          // 总费用，对应fee.total_fee
    PayStatus payStatus;      // 缴费状态，对应fee.pay_status
    Date payDate;            // 缴费日期，对应fee.pay_date

    // 默认构造函数
    Fee() : feeId(""), studentId(""), dormId(""), feeMonth(""),
        waterFee(0.0), electricFee(0.0), totalFee(0.0),
        payStatus(PayStatus::UNPAID) {
    }

    // 带参数构造函数
    Fee(const std::string& id, const std::string& stuId, const std::string& dorm,
        const std::string& month, double water, double electric,
        PayStatus status = PayStatus::UNPAID, const Date& payDt = Date())
        : feeId(id), studentId(stuId), dormId(dorm), feeMonth(month),
        waterFee(water), electricFee(electric), totalFee(water + electric),
        payStatus(status), payDate(payDt) {
    }
};


// --------------- 水电费业务逻辑封装类 ---------------
class FeeManager {
public:
    // 1. 添加水电费记录（总费用自动计算，返回true成功，false失败）
    bool addFee(const Fee& fee);

    // 2. 修改水电费记录（不含缴费状态，返回true成功，false失败）
    bool updateFee(const Fee& fee);

    // 3. 更新缴费状态（未缴→已缴，自动记录缴费日期，返回true成功）
    bool updatePayStatus(const std::string& feeId, const Date& payDate = Date());

    // 4. 删除水电费记录（返回true成功，false失败）
    bool deleteFee(const std::string& feeId);

    // 5. 通过费用ID查询记录（返回空Fee表示查询失败或不存在）
    Fee getFeeById(const std::string& feeId);

    // 6. 分页查询所有水电费记录
    std::vector<Fee> getAllFees(const PageParam& pageParam);

    // 7. 多条件筛选查询（学号/宿舍号/缴费状态，支持组合筛选）
    std::vector<Fee> filterFees(const std::string& studentId, const std::string& dormId,
        PayStatus payStatus, const PageParam& pageParam);

    // 8. 按费用月份查询记录（分页返回指定月份的所有费用）
    std::vector<Fee> getFeesByMonth(const std::string& feeMonth, const PageParam& pageParam);

    // 9. 获取费用记录总数（用于分页计算）
    int getFeeTotalCount();

    // 9.1 获取未支付费用数量
    int getUnpaidFeeCount();

    // 10. 获取筛选条件下的记录总数（用于分页计算）
    int getFilterTotalCount(const std::string& studentId, const std::string& dormId, PayStatus payStatus);

    // 11. 检查同一学生/宿舍同一月份是否已存在费用记录（返回true表示已存在）
    bool isFeeDuplicate(const std::string& studentId, const std::string& feeMonth);

    // 12. 获取最后一次操作错误信息
    std::string getLastError() const;

private:
    std::string lastError; // 存储最后一次操作错误描述

    // 私有辅助函数：校验水电费数据合法性
    bool validateFee(const Fee& fee, bool isAdd = true);

    // 私有辅助函数：将查询结果行转换为Fee对象
    Fee rowToFee(const std::map<std::string, std::string>& row);

    // 私有辅助函数：生成费用ID（格式F+年月日+序号，如F2024110001）
    std::string generateFeeId();

    // 私有辅助函数：校验学号是否存在
    bool isStudentExist(const std::string& studentId);

    // 私有辅助函数：校验宿舍号是否存在
    bool isDormExist(const std::string& dormId);
};

#endif // FEEMANAGER_H