#ifndef DORMANAGER_H
#define DORMANAGER_H

#include "Common.h"
#include "DBHelper.h"
#include <vector>
#include <string>

//宿舍结构体
struct Dorm {
    std::string dormId;         // 宿舍号
    std::string building;       // 楼栋
    std::string roomType;      // 房间类型
    int maxCapacity;           // 最大容量
    int currentOccupancy;      // 当前入住人数
    std::string dormManager;   // 宿舍管理员
    // 默认构造函数
    Dorm() : dormId(""), building(""), roomType(""), maxCapacity(0),
        currentOccupancy(0), dormManager("") {
    }
    // 带参构造函数
    Dorm(const std::string& id, const std::string& build, const std::string& type,
        int maxCap, int currOcc = 0, const std::string& manager = "")
        : dormId(id), building(build), roomType(type), maxCapacity(maxCap),
        currentOccupancy(currOcc), dormManager(manager) {
    }
};


//宿舍业务逻辑封装
class DormManager {
public:
    //添加宿舍（返回true成功，false失败，错误信息通过getLastError获取）
    bool addDorm(const Dorm& dorm);
    //修改宿舍信息（按宿舍号修改，返回true成功，false失败）
    bool updateDorm(const Dorm& dorm);
    //删除宿舍（先检查是否关联学生，返回true成功，false失败）
    bool deleteDorm(const std::string& dormId);
    //通过宿舍号查询宿舍（返回空Dorm表示查询失败或不存在）
    Dorm getDormById(const std::string& dormId);
    //分页查询所有宿舍（返回宿舍列表，无数据或查询失败返回空）
    std::vector<Dorm> getAllDorms(const PageParam& pageParam);
    //按楼栋筛选宿舍（分页返回指定楼栋的宿舍）
    std::vector<Dorm> filterDormsByBuilding(const std::string& building, const PageParam& pageParam);
    //获取宿舍总数量（用于分页计算）
    int getDormTotalCount();
    //获取指定楼栋的宿舍数量（用于分页计算）
    int getBuildingDormCount(const std::string& building);
    //更新当前住宿人数（学生入住/退宿时调用，返回true成功）
    bool updateCurrentCount(const std::string& dormId, int changeNum);
    //获取最近一次错误信息
    std::string getLastError() const;

private:
    std::string lastError; // 存储最近一次错误信息
    //数据验证（添加/修改时调用）
    bool validateDorm(const Dorm& dorm);
    //查询结果转换为Dorm对象
    Dorm rowToDorm(const std::map<std::string, std::string>& row);
    //检查宿舍是否关联学生，true/false
    bool isDormRelatedToStudent(const std::string& dormId);
    //根据房间类型获取默认最大容量
    int getDefaultMaxCapacity(const std::string& roomType);
};

#endif // DORMANAGER_H