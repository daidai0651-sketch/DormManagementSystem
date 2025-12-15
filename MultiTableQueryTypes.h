#ifndef MULTITABLEQUERYTYPES_H
#define MULTITABLEQUERYTYPES_H

// 简化后的多表查询类型枚举（集成到学生管理模块）
enum class MultiTableQueryType {
    STUDENT_DORM_INFO = 0,    // 学生-宿舍信息查询
    STUDENT_FEE_STATUS = 1    // 学生缴费状态查询
};

#endif // MULTITABLEQUERYTYPES_H