#include "Common.h"
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <locale>

bool Common::isGB2312Chinese(unsigned char c1, unsigned char c2) {
    return (c1 >= 0xA1 && c1 <= 0xF7) && (c2 >= 0xA1 && c2 <= 0xFE);
}

std::string Common::trim(const std::string& str) {
    if (str.empty()) return "";
    auto start = str.begin();
    while (start != str.end() && std::isspace(static_cast<unsigned char>(*start))) ++start;
    auto end = str.end();
    do { --end; } while (std::distance(start, end) > 0 && std::isspace(static_cast<unsigned char>(*end)));
    return std::string(start, end + 1);
}

bool Common::startsWith(const std::string& str, const std::string& prefix) {
    if (prefix.size() > str.size()) return false;
    return std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool Common::endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

std::string Common::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string Common::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string Common::padLeft(const std::string& str, int length, char pad) {
    if ((int)str.size() >= length) return str;
    return std::string(length - (int)str.size(), pad) + str;
}

std::string Common::padRight(const std::string& str, int length, char pad) {
    if ((int)str.size() >= length) return str;
    return str + std::string(length - (int)str.size(), pad);
}

bool Common::stringToInt(const std::string& str, int& result) {
    std::string trimmed = Common::trim(str);
    if (trimmed.empty()) return false;
    std::istringstream iss(trimmed);
    return (iss >> result) && (iss.eof());
}

bool Common::stringToDouble(const std::string& str, double& result) {
    std::string trimmed = Common::trim(str);
    if (trimmed.empty()) return false;
    std::istringstream iss(trimmed);
    return (iss >> result) && (iss.eof());
}

std::string Common::intToString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string Common::doubleToString(double value, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    return oss.str();
}

std::string Common::dateToString(const Date& date, const std::string& sep) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << date.year
        << sep << std::setw(2) << date.month
        << sep << std::setw(2) << date.day;
    return oss.str();
}

Date Common::stringToDate(const std::string& str, const std::string& sep) {
    Date date;
    std::string trimmed = Common::trim(str);
    if (trimmed.empty()) return date;

    size_t firstSep = trimmed.find(sep);
    size_t secondSep = trimmed.rfind(sep);
    if (firstSep == std::string::npos || secondSep == std::string::npos || firstSep == secondSep) return date;

    std::string yearStr = trimmed.substr(0, firstSep);
    std::string monthStr = trimmed.substr(firstSep + 1, secondSep - firstSep - 1);
    std::string dayStr = trimmed.substr(secondSep + 1);

    int year, month, day;
    if (Common::stringToInt(yearStr, year) && Common::stringToInt(monthStr, month) && Common::stringToInt(dayStr, day)) {
        if (year >= 2000 && year <= 2100 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
            if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) return date;
            if (month == 2) {
                bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
                if ((isLeap && day > 29) || (!isLeap && day > 28)) return date;
            }
            date.year = year; date.month = month; date.day = day;
        }
    }
    return date;
}

std::string Common::getCurrentDateStr(const std::string& sep) {
    Date date;
    return Common::dateToString(date, sep);
}

std::string Common::getCurrentDateTimeStr() {
    std::time_t now = std::time(nullptr);
    std::tm tm_now;
#if defined(_MSC_VER)
    if (localtime_s(&tm_now, &now) != 0) {
        tm_now.tm_year = 70; tm_now.tm_mon = 0; tm_now.tm_mday = 1;
        tm_now.tm_hour = tm_now.tm_min = tm_now.tm_sec = 0;
    }
#elif defined(__unix__) || defined(__APPLE__)
    // POSIX: localtime_r
    if (localtime_r(&now, &tm_now) == nullptr) {
        tm_now.tm_year = 70; tm_now.tm_mon = 0; tm_now.tm_mday = 1;
        tm_now.tm_hour = tm_now.tm_min = tm_now.tm_sec = 0;
    }
#else
    std::tm* p = std::localtime(&now);
    if (p) tm_now = *p;
    else {
        tm_now.tm_year = 70; tm_now.tm_mon = 0; tm_now.tm_mday = 1;
        tm_now.tm_hour = tm_now.tm_min = tm_now.tm_sec = 0;
    }
#endif

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << (tm_now.tm_year + 1900)
        << "-" << std::setw(2) << (tm_now.tm_mon + 1)
        << "-" << std::setw(2) << tm_now.tm_mday
        << " " << std::setw(2) << tm_now.tm_hour
        << ":" << std::setw(2) << tm_now.tm_min
        << ":" << std::setw(2) << tm_now.tm_sec;
    return oss.str();
}



std::string Common::normalizeTimeFormat(const std::string& time) {
    std::string trimmedTime = Common::trim(time);
    if (trimmedTime.size() != 5) return "00:00";

    //英文冒号
    char separator = trimmedTime[2];
    if (separator == ':') {
        return trimmedTime;
    }
    else {
        return "00:00";
    }
}

Date::Date() {
    std::time_t now = std::time(nullptr);
#if defined(_MSC_VER)
    std::tm tm_now;
    if (localtime_s(&tm_now, &now) == 0) {
        year = tm_now.tm_year + 1900;
        month = tm_now.tm_mon + 1;
        day = tm_now.tm_mday;
        return;
    }
#elif defined(__unix__) || defined(__APPLE__)
    std::tm tm_now;
    if (localtime_r(&now, &tm_now) != nullptr) {
        year = tm_now.tm_year + 1900;
        month = tm_now.tm_mon + 1;
        day = tm_now.tm_mday;
        return;
    }
#else
    std::tm* p = std::localtime(&now);
    if (p) {
        year = p->tm_year + 1900;
        month = p->tm_mon + 1;
        day = p->tm_mday;
        return;
    }
#endif
    year = 1970; month = 1; day = 1;
}

bool Common::isValidStudentID(const std::string& id) {
    std::string trimmed = Common::trim(id);
    // 修改为固定7位
    if (trimmed.size() != 7) return false;

    std::string yearPart = trimmed.substr(0, 4);
    int year;
    if (!Common::stringToInt(yearPart, year) || year < 2020 || year > 2030) return false;
    for (char c : trimmed) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    return true;
}


bool Common::isValidAdminID(const std::string& id) {
    std::string trimmed = Common::trim(id);
    if (trimmed.size() < 4 || trimmed.size() > MAX_ADMIN_ID_LEN) return false;
    bool hasLetter = false, hasDigit = false;
    for (char c : trimmed) {
        if (std::isalpha(static_cast<unsigned char>(c))) hasLetter = true;
        else if (std::isdigit(static_cast<unsigned char>(c))) hasDigit = true;
        else return false;
    }
    return hasLetter && hasDigit;
}

bool Common::isValidPwd(const std::string& pwd) {
    std::string trimmed = Common::trim(pwd);
    if (trimmed.size() < 6 || trimmed.size() > MAX_PWD_LEN) return false;
    bool hasLetter = false, hasDigit = false;
    for (char c : trimmed) {
        if (std::isalpha(static_cast<unsigned char>(c))) hasLetter = true;
        else if (std::isdigit(static_cast<unsigned char>(c))) hasDigit = true;
        else return false;
    }
    return hasLetter && hasDigit;
}

bool Common::isValidName(const std::string& name) {
    std::string trimmed = Common::trim(name);
    if (trimmed.size() < 2 || trimmed.size() > MAX_NAME_LEN) return false;

    for (size_t i = 0; i < trimmed.size(); i++) {
        unsigned char uc = static_cast<unsigned char>(trimmed[i]);
        bool isLetter = std::isalpha(uc);

        //检查是否是中文字符
        bool isChinese = false;
        if (uc >= 0xA1 && uc <= 0xF7 && i + 1 < trimmed.size()) {
            unsigned char next = static_cast<unsigned char>(trimmed[i + 1]);
            if ((uc >= 0xA1 && uc <= 0xF7) && (next >= 0xA1 && next <= 0xFE)) {
                isChinese = true;
                i++; 
            }
        }


        if (!isChinese && !isLetter) return false;
    }
    return true;
}

bool Common::isValidPhone(const std::string& phone) {
    std::string trimmed = Common::trim(phone);
    if (trimmed.size() != MAX_PHONE_LEN) return false;
    const std::string prefixes[] = { "13", "14", "15", "17", "18", "19" };
    bool validPrefix = false;
    for (const auto& prefix : prefixes) if (Common::startsWith(trimmed, prefix)) { validPrefix = true; break; }
    if (!validPrefix) return false;
    for (char c : trimmed) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    return true;
}

bool Common::isValidIDCard(const std::string& idCard) {
    std::string trimmed = Common::trim(idCard);
    if (trimmed.size() != MAX_ID_CARD_LEN) return false;
    for (size_t i = 0; i < trimmed.size() - 1; ++i) if (!std::isdigit(static_cast<unsigned char>(trimmed[i]))) return false;
    char lastChar = trimmed.back();
    return (std::isdigit(static_cast<unsigned char>(lastChar)) || lastChar == 'X' || lastChar == 'x');
}

bool Common::isValidDormID(const std::string& dormID) {
    std::string trimmed = Common::trim(dormID);
    if (trimmed.size() < 3 || trimmed.size() > MAX_DORM_ID_LEN) return false;
    size_t sepPos = trimmed.find('-');
    if (sepPos == std::string::npos || sepPos == 0 || sepPos == trimmed.size() - 1) return false;
    std::string buildingPart = trimmed.substr(0, sepPos);
    std::string roomPart = trimmed.substr(sepPos + 1);
    for (char c : buildingPart) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    for (char c : roomPart) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    return true;
}

bool Common::isValidBuilding(const std::string& building) {
    std::string trimmed = Common::trim(building);
    if (trimmed.empty() || trimmed.size() > MAX_BUILDING_LEN) return false;

    for (size_t i = 0; i < trimmed.size(); i++) {
        unsigned char uc = static_cast<unsigned char>(trimmed[i]);
        bool isDigit = std::isdigit(uc);
        bool isValidChar = (trimmed[i] == '楼' || trimmed[i] == '栋');

        bool isChinese = false;
        if (uc >= 0xA1 && uc <= 0xF7 && i + 1 < trimmed.size()) {
            unsigned char next = static_cast<unsigned char>(trimmed[i + 1]);
            if ((uc >= 0xA1 && uc <= 0xF7) && (next >= 0xA1 && next <= 0xFE)) {
                isChinese = true;
                i++; 
            }
        }
        if (!isChinese && !isDigit && !isValidChar) return false;
    }
    return true;
}


bool Common::isValidRoomType(const std::string& roomType) {
    std::string trimmed = Common::trim(roomType);
    return trimmed == "4人" || trimmed == "6人" || trimmed == "8人";
}

bool Common::isValidMajor(const std::string& major) {
    std::string trimmed = Common::trim(major);
    if (trimmed.size() < 2 || trimmed.size() > MAX_MAJOR_LEN) return false;

    for (size_t i = 0; i < trimmed.size(); i++) {
        unsigned char uc = static_cast<unsigned char>(trimmed[i]);
        bool isLetter = std::isalpha(uc);
        bool isDigit = std::isdigit(uc);
        bool isValidChar = (trimmed[i] == '学' || trimmed[i] == '院' || trimmed[i] == '(' ||
            trimmed[i] == ')' || trimmed[i] == '系' || trimmed[i] == '-');

        bool isChinese = false;
        if (uc >= 0xA1 && uc <= 0xF7 && i + 1 < trimmed.size()) {
            unsigned char next = static_cast<unsigned char>(trimmed[i + 1]);
          
            if ((uc >= 0xA1 && uc <= 0xF7) && (next >= 0xA1 && next <= 0xFE)) {
                isChinese = true;
                i++;
            }
        }


        if (!isChinese && !isLetter && !isDigit && !isValidChar) return false;
    }
    return true;
}


bool Common::isValidDate(const std::string& dateStr) {
    Date date = Common::stringToDate(dateStr);
    return Common::dateToString(date) == dateStr;
}

bool Common::isValidDateTime(const std::string& dateTimeStr) {
    std::string trimmed = Common::trim(dateTimeStr);
    if (trimmed.size() != 19) return false;
    std::string datePart = trimmed.substr(0, 10);
    std::string timePart = trimmed.substr(11);
    if (!Common::isValidDate(datePart)) return false;
    size_t firstColon = timePart.find(':');
    size_t secondColon = timePart.rfind(':');
    if (firstColon == std::string::npos || secondColon == std::string::npos || firstColon == secondColon) return false;
    std::string hourStr = timePart.substr(0, firstColon);
    std::string minStr = timePart.substr(firstColon + 1, secondColon - firstColon - 1);
    std::string secStr = timePart.substr(secondColon + 1);
    int hour, min, sec;
    if (!Common::stringToInt(hourStr, hour) || !Common::stringToInt(minStr, min) || !Common::stringToInt(secStr, sec)) return false;
    return (hour >= 0 && hour < 24) && (min >= 0 && min < 60) && (sec >= 0 && sec < 60);
}

bool Common::isValidNumber(const std::string& str) {
    std::string trimmed = Common::trim(str);
    if (trimmed.empty()) return false;
    for (char c : trimmed) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    return true;
}

bool Common::isValidDecimal(const std::string& str) {
    std::string trimmed = Common::trim(str);
    if (trimmed.empty()) return false;
    bool hasDot = false;
    size_t start = 0;
    if (trimmed[0] == '+' || trimmed[0] == '-') {
        if (trimmed.size() == 1) return false;
        start = 1;
    }
    for (size_t i = start; i < trimmed.size(); ++i) {
        if (trimmed[i] == '.') {
            if (hasDot) return false;
            hasDot = true;
            if (i == start || i == trimmed.size() - 1) return false;
        }
        else if (!std::isdigit(static_cast<unsigned char>(trimmed[i]))) return false;
    }
    return true;
}

bool Common::isValidFeeMonth(const std::string& monthStr) {
    std::string trimmed = Common::trim(monthStr);
    if (trimmed.size() != MAX_FEE_MONTH_LEN) return false;
    if (trimmed[4] != '-') return false;
    std::string yearStr = trimmed.substr(0, 4);
    std::string monthStrPart = trimmed.substr(5);
    int year, month;
    if (!Common::stringToInt(yearStr, year) || !Common::stringToInt(monthStrPart, month)) return false;
    return (year >= 2020 && year <= 2030) && (month >= 1 && month <= 12);
}

bool Common::isPointInRect(int x, int y, int rectX, int rectY, int rectW, int rectH) {
    return (x >= rectX && x <= rectX + rectW && y >= rectY && y <= rectY + rectH);
}

void Common::delay(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}