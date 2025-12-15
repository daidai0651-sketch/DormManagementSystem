#ifndef STUDENTMANAGER_H
#define STUDENTMANAGER_H

#include "Common.h"
#include "DBHelper.h"
#include <vector>
#include <string>
#include <map>

// 前向声明
class DormManager;

//学生结构体
struct Student {
    std::string studentId;      // 学号
    std::string studentName;    // 学生姓名
    std::string gender;         // 性别
    int age;                    // 年龄
    std::string major;          // 专业
    std::string dormId;         // 宿舍号
    Date checkInDate;           // 入住日期
    std::string studentPhone;   // 手机号

    // 默认构造函数 - 需要初始化checkInDate
    Student() : studentId(""), studentName(""), gender(""), age(0), 
                major(""), dormId(""), checkInDate(), studentPhone("") {}
    
    // 带参数构造函数 - 需要初始化checkInDate
    Student(const std::string& id, const std::string& name, const std::string& gend,
            int a, const std::string& maj, const std::string& dorm, 
            const std::string& phone = "", const Date& checkIn = Date())
        : studentId(id), studentName(name), gender(gend), age(a), major(maj), 
          dormId(dorm), checkInDate(checkIn), studentPhone(phone) {}
};


class StudentManager {
public:
    // 1. 添加学生
    bool addStudent(const Student& student);

    // 2. 修改学生信息
    bool updateStudent(const Student& student);

    // 3. 删除学生=
    bool deleteStudent(const std::string& studentId);

    // 4. 通过学号查询学生
    Student getStudentById(const std::string& studentId);

    // 5. 分页查询所有学生
    std::vector<Student> getAllStudents(const PageParam& pageParam);

    // 6. 模糊查询学生
    std::vector<Student> searchStudents(const std::string& keyword, const PageParam& pageParam);

    // 7. 获取学生总数（用于分页计算）
    int getStudentTotalCount();

    // 8. 获取模糊查询总数（用于分页计算）
    int getSearchTotalCount(const std::string& keyword);

    // 9. 获取最后一次操作错误信息
    std::string getLastError() const;

    // 10. 设置DormManager实例指针（用于更新宿舍人数）
    static void setDormManager(DormManager* dormMgr);

private:
    std::string lastError; // 存储最后一次操作错误描述
    static DormManager* dormManager; // 静态DormManager指针

    // 私有辅助函数：校验学生数据合法性
    bool validateStudent(const Student& student);

    // 私有辅助函数：将查询结果行转换为Student对象
    Student rowToStudent(const std::map<std::string, std::string>& row);

    // 私有辅助函数：校验宿舍号是否存在
    bool isDormExist(const std::string& dormId);
};

#endif // STUDENTMANAGER_H