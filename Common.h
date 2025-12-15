#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <thread>

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const int BUTTON_WIDTH = 120;
const int BUTTON_HEIGHT = 40;
const int INPUT_BOX_HEIGHT = 35;
const int LABEL_HEIGHT = 30;
const int LIST_ITEM_HEIGHT = 30;
const int MARGIN = 20;
const int INTERVAL = 15;

const int COLOR_BG_MAIN = 0xF0F8FF;
const int COLOR_BG_SUB = 0xFFFFFF;
const int COLOR_BG_INPUT = 0xFAFAFA;
const int COLOR_TEXT_MAIN = 0x000000;
const int COLOR_TEXT_SUB = 0x666666;
const int COLOR_TEXT_HINT = 0x999999;
const int COLOR_BUTTON_NORMAL = 0x4169E1;
const int COLOR_BUTTON_HOVER = 0x1E90FF;
const int COLOR_BUTTON_CLICK = 0x0066CC;
const int COLOR_BORDER = 0xDCDCDC;
const int COLOR_HIGHLIGHT_CUSTOM = 0xFFFF66;

const int MAX_STUDENT_ID_LEN = 20;
const int MAX_ADMIN_ID_LEN = 20;
const int MAX_PWD_LEN = 20;
const int MAX_NAME_LEN = 20;
const int MAX_PHONE_LEN = 11;
const int MAX_ID_CARD_LEN = 18;
const int MAX_DORM_ID_LEN = 10;
const int MAX_BUILDING_LEN = 10;
const int MAX_ROOM_TYPE_LEN = 10;
const int MAX_MAJOR_LEN = 30;
const int MAX_REPAIR_CONTENT_LEN = 200;
const int MAX_VISIT_REASON_LEN = 100;
const int MAX_FEE_MONTH_LEN = 7;

enum class PayStatus { UNPAID = 0, PAID = 1 };
enum class RepairStatus { UNHANDLED = 0, HANDLING = 1, COMPLETED = 2 };

enum class UIState {
    LOGIN = 0,
    MAIN = 1,
    STUDENT_MANAGE = 2,
    DORM_MANAGE = 3,
    FEE_MANAGE = 4,
    REPAIR_MANAGE = 5,
    VISITOR_MANAGE = 6,
    STATISTICS = 7,
    MULTI_TABLE_QUERY = 8,
    EXIT = 9
};

struct Date {
    int year;
    int month;
    int day;
    Date();

    Date(int y, int m, int d) : year(y), month(m), day(d) {}

    bool operator==(const Date& other) const {
        return year == other.year && month == other.month && day == other.day;
    }
    bool operator<(const Date& other) const {
        if (year != other.year) return year < other.year;
        if (month != other.month) return month < other.month;
        return day < other.day;
    }
};

struct PageParam {
    int pageIndex;
    int pageSize;
    int totalCount;
    int totalPage;
    PageParam() : pageIndex(1), pageSize(10), totalCount(0), totalPage(0) {}
    void calcTotalPage() {
        totalPage = (totalCount + pageSize - 1) / pageSize;
        if (totalPage < 1) totalPage = 1;
        if (pageIndex > totalPage) pageIndex = totalPage;
    }
    int getOffset() const { return (pageIndex - 1) * pageSize; }
};

namespace Common {
    std::string trim(const std::string& str);
    bool startsWith(const std::string& str, const std::string& prefix);
    bool endsWith(const std::string& str, const std::string& suffix);
    std::string toLower(const std::string& str);
    std::string toUpper(const std::string& str);
    std::string padLeft(const std::string& str, int length, char pad = ' ');
    std::string padRight(const std::string& str, int length, char pad = ' ');
    bool stringToInt(const std::string& str, int& result);
    bool stringToDouble(const std::string& str, double& result);
    std::string intToString(int value);
    std::string doubleToString(double value, int precision = 2);
    std::string dateToString(const Date& date, const std::string& sep = "-");
    Date stringToDate(const std::string& str, const std::string& sep = "-");
    std::string getCurrentDateStr(const std::string& sep = "-");
    std::string getCurrentDateTimeStr();
    std::string normalizeTimeFormat(const std::string& time);

    bool isValidStudentID(const std::string& id);
    bool isValidAdminID(const std::string& id);
    bool isValidPwd(const std::string& pwd);
    bool isValidName(const std::string& name);
    bool isValidPhone(const std::string& phone);
    bool isValidIDCard(const std::string& idCard);
    bool isValidDormID(const std::string& dormID);
    bool isValidBuilding(const std::string& building);
    bool isValidRoomType(const std::string& roomType);
    bool isValidMajor(const std::string& major);
    bool isValidDate(const std::string& dateStr);
    bool isValidDateTime(const std::string& dateTimeStr);
    bool isValidNumber(const std::string& str);
    bool isValidDecimal(const std::string& str);
    bool isValidFeeMonth(const std::string& monthStr);

    bool isPointInRect(int x, int y, int rectX, int rectY, int rectW, int rectH);
    bool isGB2312Chinese(unsigned char c1, unsigned char c2);
    void delay(int ms);
}

#endif 