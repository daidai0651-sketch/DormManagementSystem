#include "StudentManager.h"
#include "DormManager.h"
#include "Common.h"
#include "DBHelper.h"
#include <sstream>
#include <algorithm>

//化静态成员变量
DormManager* StudentManager::dormManager = nullptr;

//辅助函数：校验学生数据合法性 
bool StudentManager::validateStudent(const Student& student) {
    lastError.clear();

    //校验学号
    std::string trimmedId = Common::trim(student.studentId);
    if (trimmedId.empty()) {
        lastError = "学号不能为空！";
        return false;
    }
    if (!Common::isValidStudentID(trimmedId)) {
        lastError = "学号格式错误！";
        return false;
    }

    //校验姓名
    std::string trimmedName = Common::trim(student.studentName);
    if (trimmedName.empty()) {
        lastError = "姓名不能为空！";
        return false;
    }
    if (!Common::isValidName(trimmedName)) {
        lastError = "姓名格式错误（2-20位中文或字母）！";
        return false;
    }

    //校验性别
    std::string trimmedGender = Common::trim(student.gender);
    if (trimmedGender.empty()) {
        lastError = "性别不能为空！";
        return false;
    }
    if (trimmedGender != "男" && trimmedGender != "女") {
        lastError = "性别格式错误（仅支持\"男\"或\"女\"）！";
        return false;
    }

    //校验专业
    std::string trimmedMajor = Common::trim(student.major);
    if (trimmedMajor.empty()) {
        lastError = "专业不能为空！";
        return false;
    }
    if (!Common::isValidMajor(trimmedMajor)) {
        lastError = "专业格式错误";
        return false;
    }

    //校验宿舍号
    std::string trimmedDorm = Common::trim(student.dormId);
    if (trimmedDorm.empty()) {
        lastError = "宿舍号不能为空！";
        return false;
    }
    if (!Common::isValidDormID(trimmedDorm)) {
        lastError = "宿舍号格式错误";
        return false;
    }

    //校验年龄
    if (student.age < 18 || student.age > 30) {
        lastError = "年龄必须在18-30岁之间！";
        return false;
    }

    //校验手机号（可选，非空则格式正确）
    std::string trimmedPhone = Common::trim(student.studentPhone);
    if (!trimmedPhone.empty() && !Common::isValidPhone(trimmedPhone)) {
        lastError = "手机号格式错误（11位数字，以13/14/15/17/18/19开头）！";
        return false;
    }

    //校验入学日期
    return true;
}

//辅助函数：将查询结果行转换为Student对象 
Student StudentManager::rowToStudent(const std::map<std::string, std::string>& row) {
    Student student;

    //学号
    student.studentId = row.at("student_id");
    //姓名
    student.studentName = row.at("student_name");
    //性别
    student.gender = row.at("gender");
    //年龄
    Common::stringToInt(row.at("age"), student.age);
    //专业
    student.major = row.at("major");
    //宿舍号
    student.dormId = row.at("dorm_id");
    //手机号
    student.studentPhone = row.at("student_phone");
    //入住日期
    std::string checkInDateStr = row.at("check_in_date");
    student.checkInDate = Common::stringToDate(checkInDateStr);

    return student;
}

//添加学生实现 
bool StudentManager::addStudent(const Student& student) {
    lastError.clear();

    //校验学生数据合法性
    if (!validateStudent(student)) {
        return false;
    }

    //检查学号是否已存在
    Student existStudent = getStudentById(student.studentId);
    if (!existStudent.studentId.empty()) {
        lastError = "添加失败：学号" + student.studentId + "已存在！";
        return false;
    }

    //检查宿舍是否存在
    if (!isDormExist(Common::trim(student.dormId))) {
        lastError = "添加失败：该生要入住的宿舍不存在！";
        return false;
    }

    //构建INSERT SQL（处理日期和空值）
    std::ostringstream sqlStream;
    std::string trimmedPhone = Common::trim(student.studentPhone);
    sqlStream << "INSERT INTO student (student_id, student_name, gender, age, major, dorm_id, student_phone, check_in_date) "
        << "VALUES ("
        << "'" << Common::trim(student.studentId) << "', "
        << "'" << Common::trim(student.studentName) << "', "
        << "'" << Common::trim(student.gender) << "', "
        << student.age << ", "  //年龄是数字，不需要引号
        << "'" << Common::trim(student.major) << "', "
        << "'" << Common::trim(student.dormId) << "', ";

    //手机号空值处理
    if (trimmedPhone.empty()) {
        sqlStream << "NULL, ";
    }
    else {
        sqlStream << "'" << trimmedPhone << "', ";
    }
    //入住日期转换为字符串
    sqlStream << "'" << Common::dateToString(student.checkInDate) << "'"
        << ")";

    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        //检查是否为外键约束失败错误
        if (dbErr.errorCode == 1452 && dbErr.errorMsg.find("dorm_id") != std::string::npos) {
            lastError = "添加失败：该生要入住的宿舍不存在！";
        } else {
            lastError = "添加失败：" + dbErr.errorMsg;
        }
        return false;
    }

    //更新宿舍人数
    if (affectedRows > 0 && dormManager != nullptr) {
        dormManager->updateCurrentCount(student.dormId, 1);
        //显式提交事务，确保数据立即更新
        DBHelper::getInstance().commitTransaction();
    }

    return affectedRows >= 0;
}

//修改学生信息实现 
bool StudentManager::updateStudent(const Student& student) {
    lastError.clear();

    //校验学生数据合法性
    if (!validateStudent(student)) {
        return false;
    }

    //检查学生是否存在
    Student existStudent = getStudentById(student.studentId);
    if (existStudent.studentId.empty()) {
        lastError = "修改失败：未查询到学号" + student.studentId + "对应的学生！";
        return false;
    }

    //检查宿舍是否存在
    if (!isDormExist(Common::trim(student.dormId))) {
        lastError = "修改失败：该生要入住的宿舍不存在！";
        return false;
    }

    //构建UPDATE SQL（学号不可修改，仅更新其他字段）
    std::ostringstream sqlStream;
    std::string trimmedPhone = Common::trim(student.studentPhone);
    sqlStream << "UPDATE student SET "
        << "student_name = '" << Common::trim(student.studentName) << "', "
        << "gender = '" << Common::trim(student.gender) << "', "
        << "age = " << student.age << ", "  //年龄是数字，不需要引号
        << "major = '" << Common::trim(student.major) << "', "
        << "dorm_id = '" << Common::trim(student.dormId) << "', ";

    //手机号空值处理
    if (trimmedPhone.empty()) {
        sqlStream << "student_phone = NULL, ";
    }
    else {
        sqlStream << "student_phone = '" << trimmedPhone << "', ";
    }

    //入住日期更新
    sqlStream << "check_in_date = '" << Common::dateToString(student.checkInDate) << "' "
        << "WHERE student_id = '" << Common::trim(student.studentId) << "'";

    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        //检查是否为外键约束失败错误
        if (dbErr.errorCode == 1452 && dbErr.errorMsg.find("dorm_id") != std::string::npos) {
            lastError = "修改失败：该生要入住的宿舍不存在！";
        } else {
            lastError = "修改失败：" + dbErr.errorMsg;
        }
        return false;
    }

    //更新宿舍人数（如果宿舍号发生变化）
    if (affectedRows > 0 && dormManager != nullptr && existStudent.dormId != student.dormId) {
        dormManager->updateCurrentCount(existStudent.dormId, -1);  //原宿舍人数减1
        dormManager->updateCurrentCount(student.dormId, 1);      //新宿舍人数加1
        //显式提交事务，确保数据立即更新
        DBHelper::getInstance().commitTransaction();
    }

    return affectedRows >= 0;
}

//删除学生实现 
bool StudentManager::deleteStudent(const std::string& studentId) {
    lastError.clear();

    //校验学号参数
    std::string trimmedId = Common::trim(studentId);
    if (trimmedId.empty()) {
        lastError = "学号不能为空！";
        return false;
    }
    if (!Common::isValidStudentID(trimmedId)) {
        lastError = "学号格式错误！";
        return false;
    }

    //检查学生是否存在
    Student existStudent = getStudentById(trimmedId);
    if (existStudent.studentId.empty()) {
        lastError = "删除失败：未查询到学号" + trimmedId + "对应的学生！";
        return false;
    }

    //构建DELETE SQL
    std::ostringstream sqlStream;
    sqlStream << "DELETE FROM student WHERE student_id = '" << trimmedId << "'";

    //执行SQL
    int affectedRows = DBHelper::getInstance().executeUpdate(sqlStream.str());
    if (affectedRows == -1) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "删除失败：" + dbErr.errorMsg;
        return false;
    }

    //更新宿舍人数
    if (affectedRows > 0 && dormManager != nullptr) {
        dormManager->updateCurrentCount(existStudent.dormId, -1);
        //显式提交事务，确保数据立即更新
        DBHelper::getInstance().commitTransaction();
    }

    return affectedRows >= 0;
}

//通过学号查询学生实现 
Student StudentManager::getStudentById(const std::string& studentId) {
    lastError.clear();
    Student emptyStudent;

    //校验学号参数
    std::string trimmedId = Common::trim(studentId);
    if (trimmedId.empty() || !Common::isValidStudentID(trimmedId)) {
        lastError = "学号格式错误！";
        return emptyStudent;
    }

    //构建查询SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT student_id, student_name, gender, age, major, dorm_id, student_phone, check_in_date "
        << "FROM student WHERE student_id = '" << trimmedId << "'";

    //执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return emptyStudent;
    }

    //解析结果集
    Student student;
    if (result->rowCount == 1) {
        student = rowToStudent(result->rows[0]);
    }
    else {
        lastError = "未查询到学号" + trimmedId + "对应的学生！";
    }

    //释放结果集
    DBHelper::getInstance().freeResultset(result);
    return student;
}

//查询所有学生实现 
std::vector<Student> StudentManager::getAllStudents(const PageParam& pageParam) {
    lastError.clear();
    std::vector<Student> studentList;

    //校验分页参数
    if (pageParam.pageIndex < 1 || pageParam.pageSize < 1) {
        lastError = "分页参数错误：页码和每页条数必须大于0！";
        return studentList;
    }

    //查询SQL
    std::ostringstream sqlStream;
    sqlStream << "SELECT student_id, student_name, gender, age, major, dorm_id, student_phone, check_in_date "
        << "FROM student "
        << "ORDER BY student_id ASC "
        << "LIMIT " << pageParam.getOffset() << ", " << pageParam.pageSize;

    //执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return studentList;
    }

    //解析结果集
    for (const auto& row : result->rows) {
        studentList.push_back(rowToStudent(row));
    }

    //释放结果集
    DBHelper::getInstance().freeResultset(result);
    return studentList;
}

//模糊查询学生实现 
std::vector<Student> StudentManager::searchStudents(const std::string& keyword, const PageParam& pageParam) {
    lastError.clear();
    std::vector<Student> studentList;

    //校验参数
    std::string trimmedKeyword = Common::trim(keyword);
    if (trimmedKeyword.empty()) {
        lastError = "查询关键词不能为空！";
        return studentList;
    }
    if (pageParam.pageIndex < 1 || pageParam.pageSize < 1) {
        lastError = "分页参数错误：页码和每页条数必须大于0！";
        return studentList;
    }

    //构建模糊查询SQL（学号/姓名模糊匹配）
    std::ostringstream sqlStream;
    sqlStream << "SELECT student_id, student_name, gender, age, major, dorm_id, student_phone, check_in_date "
        << "FROM student "
        << "WHERE student_id LIKE '%" << trimmedKeyword << "%' "
        << "OR student_name LIKE '%" << trimmedKeyword << "%' "
        << "ORDER BY student_id ASC "
        << "LIMIT " << pageParam.getOffset() << ", " << pageParam.pageSize;

    //执行查询
    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "查询失败：" + dbErr.errorMsg;
        return studentList;
    }
    for (const auto& row : result->rows) {
        studentList.push_back(rowToStudent(row));
    }
    DBHelper::getInstance().freeResultset(result);
    return studentList;
}

int StudentManager::getStudentTotalCount() {
    lastError.clear();

    std::string sql = "SELECT COUNT(*) AS total FROM student";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "获取总数失败：" + dbErr.errorMsg;
        return 0;
    }

    int totalCount = 0;
    if (result->rowCount > 0) {
        Common::stringToInt(result->rows[0]["total"], totalCount);
    }

    DBHelper::getInstance().freeResultset(result);
    return totalCount;
}

int StudentManager::getSearchTotalCount(const std::string& keyword) {
    lastError.clear();

    std::string trimmedKeyword = Common::trim(keyword);
    if (trimmedKeyword.empty()) {
        lastError = "查询关键词不能为空！";
        return 0;
    }

    std::ostringstream sqlStream;
    sqlStream << "SELECT COUNT(*) AS total FROM student "
        << "WHERE student_id LIKE '%" << trimmedKeyword << "%' "
        << "OR student_name LIKE '%" << trimmedKeyword << "%'";

    DBResultset* result = DBHelper::getInstance().executeQuery(sqlStream.str());
    if (result == nullptr) {
        DBErrorInfo dbErr = DBHelper::getInstance().getLastError();
        lastError = "获取查询总数失败：" + dbErr.errorMsg;
        return 0;
    }

    int totalCount = 0;
    if (result->rowCount > 0) {
        Common::stringToInt(result->rows[0]["total"], totalCount);
    }

    DBHelper::getInstance().freeResultset(result);
    return totalCount;
}

std::string StudentManager::getLastError() const {
    return lastError;
}

void StudentManager::setDormManager(DormManager* dormMgr) {
    dormManager = dormMgr;
}

//校验宿舍号是否存在 
bool StudentManager::isDormExist(const std::string& dormId) {
    std::string trimmedId = Common::trim(dormId);
    if (trimmedId.empty()) return false;

    std::string sql = "SELECT dorm_id FROM dorm WHERE dorm_id = '" + trimmedId + "' LIMIT 1";
    DBResultset* result = DBHelper::getInstance().executeQuery(sql);
    if (result == nullptr) return false;

    bool exist = result->rowCount > 0;
    DBHelper::getInstance().freeResultset(result);
    return exist;
}