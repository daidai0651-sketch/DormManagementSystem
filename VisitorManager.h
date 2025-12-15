#ifndef VISITORMANAGER_H
#define VISITORMANAGER_H

#include "Common.h"
#include "DBHelper.h"
#include <vector>
#include <string>

// 访客状态枚举（对应handle_status字段）
enum class VisitorStatus {
    VISITING = 0,  // 拜访中（未离开）
    LEFT = 1       // 已离开
};

// --------------- 访客结构体（与MySQL visitor表字段一一对应）---------------
// 修改Visitor结构体
struct Visitor {
    std::string visitorId;    // 访客ID，对应visitor.visitor_id
    std::string visitorName;  // 访客姓名，对应visitor.visitor_name
    std::string gender;       // 访客性别，对应visitor.gender（新增字段）
    std::string idCard;       // 身份证号，对应visitor.id_card
    std::string dormId;       // 宿舍号，对应visitor.dorm_id
    std::string visitReason;  // 访问事由，对应visitor.visit_reason（新增字段）
    std::string visitTime;    // 访问时间，对应visitor.visit_time（修改类型）
    std::string leaveTime;    // 离开时间，对应visitor.leave_time（修改类型）
    std::string registerAdmin;// 登记管理员，对应visitor.register_admin（修改字段名）

    // 默认构造函数
    Visitor() : visitorId(""), visitorName(""), gender(""), idCard(""),
        dormId(""), visitReason(""), visitTime(""), leaveTime(""),
        registerAdmin("") {
    }

    // 带参数构造函数
    Visitor(const std::string& id, const std::string& name, const std::string& gend,
        const std::string& idCardNo, const std::string& dorm, const std::string& reason,
        const std::string& visitTm, const std::string& leaveTm = "",
        const std::string& regAdmin = "")
        : visitorId(id), visitorName(name), gender(gend), idCard(idCardNo),
        dormId(dorm), visitReason(reason), visitTime(visitTm),
        leaveTime(leaveTm), registerAdmin(regAdmin) {
    }
};


// --------------- 访客业务逻辑封装类 ---------------
class VisitorManager {
public:
    // 1. 新增访客登记（访客ID自动生成，返回true成功，false失败）
    bool addVisitor(const Visitor& visitor);

    // 2. 修改访客信息（仅允许修改未离开的访客，返回true成功，false失败）
    bool updateVisitor(const Visitor& visitor);

    // 3. 登记访客离开（更新离开时间和状态，返回true成功，false失败）
    bool recordLeave(const std::string& visitorId, const Date& leaveDate, const std::string& leaveTime);

    // 4. 删除访客记录（仅允许删除未离开的访客，返回true成功，false失败）
    bool deleteVisitor(const std::string& visitorId);

    // 5. 通过访客ID查询记录（返回空Visitor表示查询失败或不存在）
    Visitor getVisitorById(const std::string& visitorId);

    // 6. 分页查询所有访客记录
    std::vector<Visitor> getAllVisitors(const PageParam& pageParam);

    // 7. 多条件筛选查询（按学号/宿舍号/访客状态，支持组合筛选）
    std::vector<Visitor> filterVisitors(const std::string& studentId, const std::string& dormId,
        VisitorStatus status, const PageParam& pageParam);

    // 8. 获取访客记录总数（用于分页计算）
    int getVisitorTotalCount();

    // 8.1 获取当前访客数量（未离开的访客）
    int getActiveVisitorCount();

    // 9. 获取筛选条件下的记录总数（用于分页计算）
    int getFilterTotalCount(const std::string& studentId, const std::string& dormId,
        VisitorStatus status);

    // 10. 获取最后一次操作错误信息
    std::string getLastError() const;

private:
    std::string lastError; // 存储最后一次操作错误描述

    // 私有辅助函数：校验访客数据合法性
    bool validateVisitor(const Visitor& visitor, bool isAdd = true);

    // 私有辅助函数：将查询结果行转换为Visitor对象
    Visitor rowToVisitor(const std::map<std::string, std::string>& row);

    // 私有辅助函数：生成访客ID（格式V+年月日+4位序号，如V2024110001）
    std::string generateVisitorId();

    // 私有辅助函数：校验学号是否存在
    bool isStudentExist(const std::string& studentId);

    // 私有辅助函数：校验宿舍号是否存在
    bool isDormExist(const std::string& dormId);

    // 私有辅助函数：校验身份证号格式（18位）
    bool isValidIdCard(const std::string& idCard);

    // 私有辅助函数：校验时间格式（HH:MM）
    bool isValidTimeFormat(const std::string& time);

    // 私有辅助函数：校验出入时间逻辑（离开时间≥拜访时间）
    bool isValidTimeLogic(const Date& visitDate, const std::string& visitTime,
        const Date& leaveDate, const std::string& leaveTime);
};

#endif // VISITORMANAGER_H