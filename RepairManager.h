#ifndef REPAIRMANAGER_H
#define REPAIRMANAGER_H

#include "Common.h"
#include "DBHelper.h"
#include <vector>
#include <string>
#include <map>

//Repair结构体
struct Repair {
    std::string repairId;     //报修ID
    std::string studentId;    //学号
    std::string dormId;       //宿舍号
    std::string repairContent;//报修内容
    Date repairDate;         //报修日期
    RepairStatus handleStatus;//处理状态
    Date handleDate;         //处理日期

    //默认构造函数
    Repair() : repairId(""), studentId(""), dormId(""), repairContent(""),
        handleStatus(RepairStatus::UNHANDLED), handleDate(0, 0, 0) {
    }

    //带参数构造函数
    Repair(const std::string& id, const std::string& stuId, const std::string& dorm,
        const std::string& content, const Date& repairDt = Date(),
        RepairStatus status = RepairStatus::UNHANDLED,
        const Date& handleDt = Date())
        : repairId(id), studentId(stuId), dormId(dorm), repairContent(content),
        repairDate(repairDt), handleStatus(status), handleDate(handleDt) {
    }
};

class RepairManager {
public:
    bool addRepair(const Repair& repair);
    bool updateRepair(const Repair& repair);
    bool updateHandleStatus(const std::string& repairId, RepairStatus newStatus);
    bool deleteRepair(const std::string& repairId);
    Repair getRepairById(const std::string& repairId);
    std::vector<Repair> getAllRepairs(const PageParam& pageParam);
    std::vector<Repair> filterRepairs(const std::string& studentId, const std::string& dormId,
        RepairStatus handleStatus, const PageParam& pageParam);
    int getRepairTotalCount();
    int getUnfinishedRepairCount();
    int getFilterTotalCount(const std::string& studentId, const std::string& dormId,
        RepairStatus handleStatus);
    std::string getLastError() const;

private:
    std::string lastError; 
    bool validateRepair(const Repair& repair, bool isAdd = true);
    Repair rowToRepair(const std::map<std::string, std::string>& row);
    std::string generateRepairId();
    bool isStudentExist(const std::string& studentId);
    bool isDormExist(const std::string& dormId);
    bool isValidStatusTransition(RepairStatus prevStatus, RepairStatus newStatus);
};

#endif 