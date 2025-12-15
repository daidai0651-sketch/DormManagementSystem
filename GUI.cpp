#include "Common.h"
#include "StudentManager.h"
#include "DormManager.h"
#include "FeeManager.h"
#include "RepairManager.h"
#include "VisitorManager.h"
#include "MultiTableQueryManager.h"
#include "MultiTableQueryTypes.h"
#include "main.h"

#include <graphics.h>
#include <conio.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <windows.h>

StudentManager studentMgr;
DormManager dormMgr;
FeeManager feeMgr;
RepairManager repairMgr;
VisitorManager visitorMgr;
MultiTableQueryManager multiTableMgr;

//初始化StudentManager的DormManager指针
struct InitDormManager {
    InitDormManager() {
        StudentManager::setDormManager(&dormMgr);
    }
};
static InitDormManager initDormManager;

void showStudentFeeQueryResult(const std::vector<StudentDormFeeInfo>& results);
void showStudentInfoQueryResult(const Student& student);

//绘制背景函数
void drawBackground() {
    extern const int APP_WIN_W;
    extern const int APP_WIN_H;
    extern IMAGE g_backgroundImage;
    extern bool g_backgroundLoaded;
    
    if (g_backgroundLoaded) {
        putimage(0, 0, APP_WIN_W, APP_WIN_H, &g_backgroundImage, 0, 0);
    } else {
        setbkcolor(COLOR_BG_MAIN);
        cleardevice();
    }
}

//绘制登录界面函数
void drawLoginScreen(const std::string& tip, COLORREF tipColor) {
    extern const int APP_WIN_W;
    extern const int APP_WIN_H;
    extern IMAGE g_backgroundImage;
    extern bool g_backgroundLoaded;
    
    //绘制背景
    drawBackground();
    
    //登录框参数
    const int LOGIN_BOX_W = 420;
    const int LOGIN_BOX_H = 220;
    const int LOGIN_BTN_W = 120;
    const int LOGIN_BTN_H = 40;
    
    //计算位置
    int boxX = (APP_WIN_W - LOGIN_BOX_W) / 2;
    int boxY = 160;
    int btnY = boxY + 130;
    int btnX1 = boxX + 60;
    int btnX2 = boxX + LOGIN_BOX_W - 60 - LOGIN_BTN_W;
    
    //绘制登录框背景
    setfillcolor(0xFFFFFF); //白色背景
    fillrectangle(boxX, boxY, boxX + LOGIN_BOX_W, boxY + LOGIN_BOX_H);
    
    //绘制标题
    settextcolor(BLACK);
    settextstyle(30, 0, "宋体");
    setbkmode(TRANSPARENT); //设置文字背景透明
    std::string title = "管理员登录";
    outtextxy(boxX + (LOGIN_BOX_W - textwidth(title.c_str())) / 2, boxY + 20, _T(title.c_str()));
    
    //绘制提示信息
    if (!tip.empty()) {
        // 确保提示信息显示在醒目位置
        settextcolor(tipColor);
        settextstyle(20, 0, _T("宋体"));
        setbkmode(TRANSPARENT); //设置文字背景透明
        RECT tipRect = { 50, 400, 550, 450 };
        drawtext(tip.c_str(), &tipRect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
    }
    
    //绘制按钮
    settextstyle(18, 0, "宋体");
    setbkmode(TRANSPARENT); //设置文字背景透明
    
    //登录按钮
    setfillcolor(0x009900); //绿色
    fillrectangle(btnX1, btnY, btnX1 + LOGIN_BTN_W, btnY + LOGIN_BTN_H);
    settextcolor(WHITE);
    outtextxy(btnX1 + 30, btnY + 10, _T("登录"));
    
    //退出按钮
    setfillcolor(0xCC0000); //红色
    fillrectangle(btnX2, btnY, btnX2 + LOGIN_BTN_W, btnY + LOGIN_BTN_H);
    settextcolor(WHITE);
    outtextxy(btnX2 + 30, btnY + 10, _T("退出"));
}

//使用main.cpp中定义的全局变量
extern UIState currentState;
extern PageParam g_pageParam;
extern std::string g_tipMsg;
extern COLORREF g_tipColor;

const int WINDOW_W = 1024;
const int WINDOW_H = 768;
const int BTN_W = 120;
const int BTN_H = 40;
const int ROW_H = 30;

std::string showInputBox(const std::string& title, const std::string& prompt) {
    const int DIALOG_W = 400;
    const int DIALOG_H = 200;
    const int BTN_W = 80;
    const int BTN_H = 30;
    const int INPUT_W = 200;
    
    int dialogX = (WINDOW_W - DIALOG_W) / 2;
    int dialogY = (WINDOW_H - DIALOG_H) / 2;
    
    setfillcolor(0xFFFFFF); //WHITE
    fillrectangle(dialogX, dialogY, dialogX + DIALOG_W, dialogY + DIALOG_H);
    
    setfillcolor(0x0066CC);
    fillrectangle(dialogX, dialogY, dialogX + DIALOG_W, dialogY + 30);
    
    settextcolor(WHITE);
    settextstyle(16, 0, _T("宋体"));
    outtextxy(dialogX + 10, dialogY + 7, _T(title.c_str()));
    
    settextcolor(BLACK);
    settextstyle(14, 0, _T("宋体"));
    outtextxy(dialogX + 20, dialogY + 50, _T(prompt.c_str()));
    
    int inputX = dialogX + (DIALOG_W - INPUT_W) / 2;
    int inputY = dialogY + 80;
    setfillcolor(0xFFFFFF); //WHITE
    fillrectangle(inputX, inputY, inputX + INPUT_W, inputY + 25);
    
    int okBtnX = dialogX + 80;
    int cancelBtnX = dialogX + 240;
    int btnY = dialogY + 130;

    setfillcolor(0x009900);
    fillrectangle(okBtnX, btnY, okBtnX + BTN_W, btnY + BTN_H);
    settextcolor(WHITE);
    settextstyle(14, 0, _T("宋体"));
    outtextxy(okBtnX + 25, btnY + 8, _T("确定"));
    
    setfillcolor(0xCC0000);
    fillrectangle(cancelBtnX, btnY, cancelBtnX + BTN_W, btnY + BTN_H);
    settextcolor(WHITE);
    outtextxy(cancelBtnX + 25, btnY + 8, _T("取消"));
    
    std::string inputText = "";
    bool inputActive = true;
    bool canceled = false;
    
    while (inputActive) {
        ExMessage msg;
        if (peekmessage(&msg, EX_MOUSE | EX_KEY | EX_CHAR)) {
            if (msg.message == WM_CHAR) {
                if (msg.ch == 8) {
                    if (!inputText.empty()) {
                        inputText.pop_back();
                    }
                } else if (msg.ch == 13) {
                    inputActive = false;
                } else {
                    if (inputText.length() < 50) {
                        inputText += (char)msg.ch;
                    }
                }
                
                setfillcolor(0xFFFFFF); //WHITE
                fillrectangle(inputX, inputY, inputX + INPUT_W, inputY + 25);
                settextcolor(BLACK);
                
                if (!inputText.empty()) {
                    settextstyle(14, 0, "宋体");
                    outtextxy(inputX + 5, inputY + 5, _T(inputText.c_str()));
                }
            }
            else if (msg.message == WM_LBUTTONDOWN) {
                if (msg.x >= okBtnX && msg.x <= okBtnX + BTN_W && 
                    msg.y >= btnY && msg.y <= btnY + BTN_H) {
                    inputActive = false;
                }
                else if (msg.x >= cancelBtnX && msg.x <= cancelBtnX + BTN_W && 
                         msg.y >= btnY && msg.y <= btnY + BTN_H) {
                    inputActive = false;
                    canceled = true;
                }
            }
        }
        
        Sleep(10);
    }
    
    //重新绘制当前界面来清除对话框，而不是绘制黑色矩形
    drawCurrentScreen("", BLACK);
    
    if (canceled) {
        return "";
    }
    
    return inputText;
}

void drawButton(int x, int y, const std::string& text) {
    ExMessage msg;
    peekmessage(&msg, EX_MOUSE);

    bool hover = (msg.x >= x && msg.x <= x + BTN_W && msg.y >= y && msg.y <= y + BTN_H);
    setfillcolor(hover ? 0xE0E0E0 : 0xFFFFFF);

    setlinecolor(hover ? 0xE0E0E0 : 0xFFFFFF);
    fillrectangle(x, y, x + BTN_W, y + BTN_H);

    settextcolor(BLACK);
    setbkmode(TRANSPARENT);

    //使用_T()宏正确处理字符串
    int tw = textwidth(_T(text.c_str()));
    int th = textheight(_T(text.c_str()));
    int tx = x + (BTN_W - tw) / 2;
    int ty = y + (BTN_H - th) / 2;
    outtextxy(tx, ty, _T(text.c_str()));
}

void drawTableHeader(int x, int y, const std::vector<std::string>& headers, int colWidth) {
    setfillcolor(0xCCCCCC);
    int cols = static_cast<int>(headers.size());
    for (int i = 0; i < cols; ++i) {
        int lx = x + i * colWidth;
        int rx = x + (i + 1) * colWidth;
        fillrectangle(lx, y, rx, y + ROW_H);
        //使用_T()宏正确处理字符串
        outtextxy(lx + 5, y + 5, _T(headers[static_cast<size_t>(i)].c_str()));
    }
}

void drawTitleBar(const std::string& title) {
    settextcolor(BLACK);
    settextstyle(30, 0, "宋体");
    outtextxy(50, 30, _T(title.c_str()));
    settextstyle(20, 0, "宋体");
    drawButton(WINDOW_W - 150, 30, "返回主菜单");
}

void drawFooter(int totalCount) {
    settextcolor(g_tipColor);
    outtextxy(50, WINDOW_H - 80, _T(g_tipMsg.c_str()));

    g_pageParam.totalCount = totalCount;
    g_pageParam.calcTotalPage();

    std::string pageInfo = "第 " + std::to_string(g_pageParam.pageIndex) + " / " + std::to_string(g_pageParam.totalPage) + " 页";
    settextcolor(BLACK);
    outtextxy(WINDOW_W / 2 - 50, WINDOW_H - 60, _T(pageInfo.c_str()));

    drawButton(WINDOW_W / 2 - 200, WINDOW_H - 70, "上一页");
    drawButton(WINDOW_W / 2 + 100, WINDOW_H - 70, "下一页");
}

bool isClickedReturn(int x, int y) {
    if (Common::isPointInRect(x, y, WINDOW_W - 150, 30, BTN_W, BTN_H)) {
        currentState = UIState::MAIN;
        g_tipMsg = "";
        return true;
    }
    return false;
}

bool isClickedPagination(int x, int y) {
    if (Common::isPointInRect(x, y, WINDOW_W / 2 - 200, WINDOW_H - 70, BTN_W, BTN_H)) {
        if (g_pageParam.pageIndex > 1) g_pageParam.pageIndex--;
        return true;
    }
    else if (Common::isPointInRect(x, y, WINDOW_W / 2 + 100, WINDOW_H - 70, BTN_W, BTN_H)) {
        if (g_pageParam.pageIndex < g_pageParam.totalPage) g_pageParam.pageIndex++;
        return true;
    }
    return false;
}

void drawMainMenu() {
    drawBackground();

    settextcolor(BLACK);
    settextstyle(50, 0, "宋体");
    std::string title = "宿舍管理系统";
    outtextxy((WINDOW_W - textwidth(title.c_str())) / 2, 100, _T(title.c_str()));

    settextstyle(24, 0, "宋体");
    int startY = 200;
    int gap = 60;

    drawButton((WINDOW_W - BTN_W) / 2, startY, "学生管理");
    drawButton((WINDOW_W - BTN_W) / 2, startY + gap, "宿舍管理");
    drawButton((WINDOW_W - BTN_W) / 2, startY + gap * 2, "收费管理");
    drawButton((WINDOW_W - BTN_W) / 2, startY + gap * 3, "报修管理");
    drawButton((WINDOW_W - BTN_W) / 2, startY + gap * 4, "访客管理");
    drawButton((WINDOW_W - BTN_W) / 2, startY + gap * 5, "系统统计");
}

void drawStudentManage() {
    drawBackground();
    drawTitleBar("学生管理");

    int y = 100;
    drawButton(50, y, "添加学生");
    drawButton(180, y, "修改学生");
    drawButton(310, y, "删除学生");
    drawButton(440, y, "查询学生信息");
    drawButton(570, y, "学生缴费查询");

    y += 60;
    
    //定义表格列标题
    std::vector<std::string> headers;
    headers = { "学号", "姓名", "性别", "年龄", "专业", "宿舍", "电话", "入学日期" };
    
    int colWidth = (WINDOW_W - 100) / static_cast<int>(headers.size());
    drawTableHeader(50, y, headers, colWidth);

    y += ROW_H;
    
    //显示普通学生列表
    auto students = studentMgr.getAllStudents(g_pageParam);
    
    for (const auto& s : students) {
        int x = 50;
        settextcolor(BLACK);
        settextstyle(20, 0, "宋体");
        
        //使用_T()宏正确处理字符串
        outtextxy(x + 5, y + 5, _T(s.studentId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(s.studentName.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(s.gender.c_str())); x += colWidth;
        
        std::string ageStr = std::to_string(s.age);
        outtextxy(x + 5, y + 5, _T(ageStr.c_str())); x += colWidth;
        
        outtextxy(x + 5, y + 5, _T(s.major.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(s.dormId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(s.studentPhone.c_str())); x += colWidth;
        
        std::string dateStr = Common::dateToString(s.checkInDate);
        outtextxy(x + 5, y + 5, _T(dateStr.c_str()));
        
        y += ROW_H;
    }
    
    drawFooter(studentMgr.getStudentTotalCount());
}

void drawDormManage() {
    drawBackground();
    drawTitleBar("宿舍管理");
    int y = 100;
    drawButton(50, y, "添加宿舍");
    drawButton(180, y, "修改宿舍");
    drawButton(310, y, "删除宿舍");

    y += 60;
    std::vector<std::string> headers = { "宿舍号", "楼号", "类型", "最大容量", "当前人数", "管理电话" };
    int colWidth = (WINDOW_W - 100) / static_cast<int>(headers.size());
    drawTableHeader(50, y, headers, colWidth);

    auto dorms = dormMgr.getAllDorms(g_pageParam);
    y += ROW_H;
    for (const auto& d : dorms) {
        int totalWidth = static_cast<int>(headers.size()) * colWidth;
        int x = 50;
        
        //使用_T()宏正确处理字符串
        outtextxy(x + 5, y + 5, _T(d.dormId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(d.building.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(d.roomType.c_str())); x += colWidth;
        
        std::string capStr = std::to_string(d.maxCapacity);
        outtextxy(x + 5, y + 5, _T(capStr.c_str())); x += colWidth;
        
        std::string occStr = std::to_string(d.currentOccupancy);
        outtextxy(x + 5, y + 5, _T(occStr.c_str())); x += colWidth;
        
        outtextxy(x + 5, y + 5, _T(d.dormManager.c_str()));
        
        y += ROW_H;
    }
    drawFooter(dormMgr.getDormTotalCount());
}

void drawFeeManage() {
    drawBackground();
    drawTitleBar("收费管理");

    int y = 100;
    drawButton(50, y, "添加收费");
    drawButton(180, y, "修改状态");
    drawButton(310, y, "删除记录");

    y += 60;
    std::vector<std::string> headers = { "收费ID", "学号", "宿舍", "月份", "水费", "电费", "合计", "状态", "支付日期" };
    int colWidth = (WINDOW_W - 100) / static_cast<int>(headers.size());
    drawTableHeader(50, y, headers, colWidth);

    auto fees = feeMgr.getAllFees(g_pageParam);

    y += ROW_H;
    for (const auto& f : fees) {
        settextcolor(BLACK);
        int totalWidth = static_cast<int>(headers.size()) * colWidth;
        int x = 50;
        settextstyle(20, 0, "宋体");
        
        //使用_T()宏正确处理字符串
        outtextxy(x + 5, y + 5, _T(f.feeId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(f.studentId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(f.dormId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(f.feeMonth.c_str())); x += colWidth;
        
        std::stringstream waterStream;
        waterStream << std::fixed << std::setprecision(2) << f.waterFee;
        outtextxy(x + 5, y + 5, _T(waterStream.str().c_str())); x += colWidth;
        
        std::stringstream elecStream;
        elecStream << std::fixed << std::setprecision(2) << f.electricFee;
        outtextxy(x + 5, y + 5, _T(elecStream.str().c_str())); x += colWidth;
        
        std::stringstream totalStream;
        totalStream << std::fixed << std::setprecision(2) << f.totalFee;
        outtextxy(x + 5, y + 5, _T(totalStream.str().c_str())); x += colWidth;
        
        std::string statusStr = (f.payStatus == PayStatus::PAID) ? "已付" : "未付";
        outtextxy(x + 5, y + 5, _T(statusStr.c_str())); x += colWidth;
        
        //未付状态时支付日期为空，已付状态时显示支付日期
        std::string dateStr = (f.payStatus == PayStatus::PAID) ? Common::dateToString(f.payDate) : "";
        outtextxy(x + 5, y + 5, _T(dateStr.c_str()));

        y += ROW_H;
    }

    drawFooter(feeMgr.getFeeTotalCount());
}

void drawRepairManage() {
    drawBackground();
    drawTitleBar("报修管理");

    int y = 100;
    drawButton(50, y, "添加报修");
    drawButton(180, y, "处理报修");
    drawButton(310, y, "删除记录");

    y += 60;
    std::vector<std::string> headers = { "报修ID", "学号", "宿舍", "内容", "申请日期", "状态", "处理日期" };
    int colWidth = (WINDOW_W - 100) / static_cast<int>(headers.size());
    drawTableHeader(50, y, headers, colWidth);

    auto repairs = repairMgr.getAllRepairs(g_pageParam);

    y += ROW_H;
    for (const auto& r : repairs) {
        settextcolor(BLACK);
        int totalWidth = static_cast<int>(headers.size()) * colWidth;
        int x = 50;
        settextstyle(20, 0, "宋体");
        
        //使用_T()宏正确处理字符串
        outtextxy(x + 5, y + 5, _T(r.repairId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(r.studentId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(r.dormId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(r.repairContent.c_str())); x += colWidth;
        
        std::string dateStr = Common::dateToString(r.repairDate);
        outtextxy(x + 5, y + 5, _T(dateStr.c_str())); x += colWidth;
        
        std::string statusStr;
        if (r.handleStatus == RepairStatus::UNHANDLED) statusStr = "未处理";
        else if (r.handleStatus == RepairStatus::HANDLING) statusStr = "处理中";
        else statusStr = "已完成";
        outtextxy(x + 5, y + 5, _T(statusStr.c_str())); x += colWidth;
        
        //处理日期显示：如果状态为未处理，显示"未处理"，否则显示实际日期
        std::string handleDateStr;
        if (r.handleStatus == RepairStatus::UNHANDLED) {
            handleDateStr = "未处理";
        } else {
            //已处理或处理中状态，显示处理日期
            //检查是否是有效日期（非0-0-0表示实际设置了日期）
            if (r.handleDate.year > 0 && r.handleDate.month > 0 && r.handleDate.day > 0) {
                handleDateStr = Common::dateToString(r.handleDate);
            } else {
                handleDateStr = "未设置";
            }
        }
        outtextxy(x + 5, y + 5, _T(handleDateStr.c_str()));

        y += ROW_H;
    }

    drawFooter(repairMgr.getRepairTotalCount());
}

void drawVisitorManage() {
    drawBackground();
    drawTitleBar("访客管理");

    int y = 100;
    drawButton(50, y, "登记访客");
    drawButton(180, y, "访客离开");
    drawButton(310, y, "删除记录");

    y += 60;
    std::vector<std::string> headers = { "访客ID", "姓名", "性别", "身份证", "宿舍号", "访问事由", "来访时间", "离开时间", "登记人" };
    int colWidth = (WINDOW_W - 100) / static_cast<int>(headers.size());
    drawTableHeader(50, y, headers, colWidth);

    auto visitors = visitorMgr.getAllVisitors(g_pageParam);

    y += ROW_H;
    for (const auto& v : visitors) {
        settextcolor(BLACK);
        int totalWidth = static_cast<int>(headers.size()) * colWidth;
        int x = 50;
        settextstyle(20, 0, "宋体");
        
        //使用_T()宏正确处理字符串
        outtextxy(x + 5, y + 5, _T(v.visitorId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(v.visitorName.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(v.gender.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T("***")); x += colWidth;
        outtextxy(x + 5, y + 5, _T(v.dormId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(v.visitReason.c_str())); x += colWidth;
        
        std::string visitTimeOnly = v.visitTime.substr(11, 5);
        outtextxy(x + 5, y + 5, _T(visitTimeOnly.c_str())); x += colWidth;
        
        std::string leaveTimeOnly;
        if (v.leaveTime.length() >= 16) {
            leaveTimeOnly = v.leaveTime.substr(11, 5);
        } else {
            leaveTimeOnly = "未离开";
        }
        outtextxy(x + 5, y + 5, _T(leaveTimeOnly.c_str())); x += colWidth;
        
        outtextxy(x + 5, y + 5, _T(v.registerAdmin.c_str()));

        y += ROW_H;
    }

    drawFooter(visitorMgr.getVisitorTotalCount());
}

void handleMainMenu(int x, int y) {
    int startY = 200;
    int gap = 60;
    int centerX = (WINDOW_W - BTN_W) / 2;

    if (Common::isPointInRect(x, y, centerX, startY, BTN_W, BTN_H)) {
        currentState = UIState::STUDENT_MANAGE;
        g_pageParam.pageIndex = 1;
        g_tipMsg = "";
        return; //确保函数返回
    }
    else if (Common::isPointInRect(x, y, centerX, startY + gap, BTN_W, BTN_H)) {
        currentState = UIState::DORM_MANAGE;
        g_pageParam.pageIndex = 1;
        g_tipMsg = "";
        return;
    }
    else if (Common::isPointInRect(x, y, centerX, startY + gap * 2, BTN_W, BTN_H)) {
        currentState = UIState::FEE_MANAGE;
        g_pageParam.pageIndex = 1;
        g_tipMsg = "";
        return;
    }
    else if (Common::isPointInRect(x, y, centerX, startY + gap * 3, BTN_W, BTN_H)) {
        currentState = UIState::REPAIR_MANAGE;
        g_pageParam.pageIndex = 1;
        g_tipMsg = "";
        return;
    }
    else if (Common::isPointInRect(x, y, centerX, startY + gap * 4, BTN_W, BTN_H)) {
        currentState = UIState::VISITOR_MANAGE;
        g_pageParam.pageIndex = 1;
        g_tipMsg = "";
        return;
    }
    else if (Common::isPointInRect(x, y, centerX, startY + gap * 5, BTN_W, BTN_H)) {
        currentState = UIState::STATISTICS;
        g_tipMsg = "";
        return;
    }
}

void handleStudentEvent(int x, int y) {
    if (isClickedReturn(x, y) || isClickedPagination(x, y)) return;

    if (Common::isPointInRect(x, y, 50, 100, BTN_W, BTN_H)) {
        Student s;
        s.studentId = showInputBox("添加学生", "输入学号(如:2024001):");
        if (s.studentId.empty()) return;
        s.studentName = showInputBox("添加学生", "姓名:");
        s.gender = showInputBox("添加学生", "性别(男/女):");
        if (s.gender.empty()) {
            g_tipMsg = "性别不能为空"; g_tipColor = RED; return;
        }
        std::string ageStr = showInputBox("添加学生", "年龄(18-30):");
        if (!Common::stringToInt(ageStr, s.age)) {
            g_tipMsg = "年龄格式错误"; g_tipColor = RED; return;
        }
        s.major = showInputBox("添加学生", "专业:");
        s.dormId = showInputBox("添加学生", "宿舍号:");
        s.studentPhone = showInputBox("添加学生", "电话(可选):");

        std::string dateStr = showInputBox("添加学生", "入学日期(YYYY-MM-DD):");
        if (!dateStr.empty()) {
            if (Common::isValidDate(dateStr)) {
                s.checkInDate = Common::stringToDate(dateStr);
            }
            else {
                g_tipMsg = "日期格式错误"; g_tipColor = RED; return;
            }
        }

        if (studentMgr.addStudent(s)) {
            g_tipMsg = "添加成功"; g_tipColor = 0x00AA00;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
        else {
            g_tipMsg = "添加失败: " + studentMgr.getLastError(); g_tipColor = RED;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
    }
    else if (Common::isPointInRect(x, y, 180, 100, BTN_W, BTN_H)) {
        std::string id = showInputBox("修改学生", "输入要修改的学号:");
        if (id.empty()) return;
        Student s = studentMgr.getStudentById(id);
        if (s.studentId.empty()) {
            g_tipMsg = "查询失败: " + studentMgr.getLastError(); g_tipColor = RED; return;
        }

        s.studentName = showInputBox("修改学生", "姓名(当前:" + s.studentName + "):");
        s.gender = showInputBox("修改学生", "性别(当前:" + s.gender + "):");
        if (s.gender.empty()) {
            g_tipMsg = "性别不能为空"; g_tipColor = RED; return;
        }
        std::string ageStr = showInputBox("修改学生", "年龄(当前:" + std::to_string(s.age) + "):");
        if (!ageStr.empty()) {
            if (!Common::stringToInt(ageStr, s.age)) {
                g_tipMsg = "年龄格式错误"; g_tipColor = RED; return;
            }
        }
        s.major = showInputBox("修改学生", "专业(当前:" + s.major + "):");
        s.dormId = showInputBox("修改学生", "宿舍(当前:" + s.dormId + "):");
        s.studentPhone = showInputBox("修改学生", "电话(当前:" + s.studentPhone + "):");

        if (studentMgr.updateStudent(s)) {
            g_tipMsg = "修改成功"; g_tipColor = 0x00AA00;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
        else {
            g_tipMsg = "修改失败: " + studentMgr.getLastError(); g_tipColor = RED;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
    }
    else if (Common::isPointInRect(x, y, 310, 100, BTN_W, BTN_H)) {
        std::string id = showInputBox("删除学生", "输入要删除的学号:");
        if (!id.empty()) {
            if (studentMgr.deleteStudent(id)) {
                g_tipMsg = "删除成功"; g_tipColor = 0x00AA00;
                drawCurrentScreen("", BLACK); //立即重绘界面
            }
            else {
                g_tipMsg = "删除失败: " + studentMgr.getLastError(); g_tipColor = RED;
                drawCurrentScreen("", BLACK); //立即重绘界面
            }
        }
    }
    else if (Common::isPointInRect(x, y, 440, 100, BTN_W, BTN_H)) {
        //查询学生信息
        std::string studentId = showInputBox("查询学生信息", "输入学号:");
        
        //检查用户是否点击了取消
        if (studentId.empty()) {
            //用户点击了取消，直接返回不进行查询
            return;
        }
        
        //查询特定学生信息
        Student s = studentMgr.getStudentById(studentId);
        if (!s.studentId.empty()) {
            g_tipMsg = "查询成功: " + s.studentName + " - " + s.major;
            g_tipColor = 0x00AA00;
            
            //显示学生详细信息对话框（统一界面）
            showStudentInfoQueryResult(s);
        } else {
            g_tipMsg = "未找到该学生信息"; g_tipColor = RED;
        }
        drawCurrentScreen("", BLACK); //立即重绘界面
    }
    
    else if (Common::isPointInRect(x, y, 570, 100, BTN_W, BTN_H)) {
        //学生缴费查询
        std::string studentId = showInputBox("学生缴费查询", "输入学号:");
        
        //检查用户是否点击了取消或输入为空
        if (studentId.empty()) {
            g_tipMsg = "学号不能为空"; g_tipColor = RED;
            drawCurrentScreen("", BLACK);
            return;
        }
        
        //查询学生缴费信息
        auto studentFeeInfo = multiTableMgr.queryStudentDormFee(studentId);
        
        if (!studentFeeInfo.empty()) {
            //显示查询结果对话框
            showStudentFeeQueryResult(studentFeeInfo);
        } else {
            g_tipMsg = "未找到相关记录"; g_tipColor = RED;
        }
        drawCurrentScreen("", BLACK); //立即重绘界面
    }
}

void handleDormEvent(int x, int y) {
    if (isClickedReturn(x, y) || isClickedPagination(x, y)) return;

    if (Common::isPointInRect(x, y, 50, 100, BTN_W, BTN_H)) {
        Dorm d;
        d.dormId = showInputBox("添加宿舍", "宿舍号(如1-101):");
        if (d.dormId.empty()) return;
        d.building = showInputBox("添加宿舍", "楼号(如1栋):");
        
        std::string roomTypeInput = showInputBox("添加宿舍", "类型(输入数字:4/6/8):");
        if (roomTypeInput.empty()) return;
        
        if (roomTypeInput != "4" && roomTypeInput != "6" && roomTypeInput != "8") {
            g_tipMsg = "宿舍类型错误，只能输入4、6或8"; g_tipColor = RED; return;
        }
        
        d.roomType = roomTypeInput;
        
        std::string capStr = showInputBox("添加宿舍", "最大容量:");
        if (!Common::stringToInt(capStr, d.maxCapacity)) { g_tipMsg = "容量格式错误"; g_tipColor = RED; return; }
        d.dormManager = showInputBox("添加宿舍", "管理电话(可选):");

        if (dormMgr.addDorm(d)) {
            g_tipMsg = "添加成功"; g_tipColor = 0x00AA00;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
        else {
            g_tipMsg = "添加失败: " + dormMgr.getLastError(); g_tipColor = RED;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
    }
    else if (Common::isPointInRect(x, y, 180, 100, BTN_W, BTN_H)) {
        std::string id = showInputBox("修改宿舍", "输入要修改的宿舍号:");
        if (id.empty()) return;
        Dorm d = dormMgr.getDormById(id);
        if (d.dormId.empty()) {
            g_tipMsg = "查询失败: " + dormMgr.getLastError(); g_tipColor = RED; return;
        }

        d.building = showInputBox("修改宿舍", "楼号(当前:" + d.building + "):");
        
        std::string roomTypeInput = showInputBox("修改宿舍", "类型(输入数字:4/6/8, 当前:" + d.roomType + "):");
        if (!roomTypeInput.empty()) {
            if (roomTypeInput != "4" && roomTypeInput != "6" && roomTypeInput != "8") {
                g_tipMsg = "宿舍类型错误，只能输入4、6或8"; g_tipColor = RED; return;
            }
            d.roomType = roomTypeInput;
        }
        
        std::string phone = showInputBox("修改宿舍", "管理电话(当前:" + d.dormManager + "):");
        d.dormManager = phone;

        if (dormMgr.updateDorm(d)) {
            g_tipMsg = "修改成功"; g_tipColor = 0x00AA00;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
        else {
            g_tipMsg = "修改失败: " + dormMgr.getLastError(); g_tipColor = RED;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
    }
    else if (Common::isPointInRect(x, y, 310, 100, BTN_W, BTN_H)) {
        std::string id = showInputBox("删除宿舍", "输入要删除的宿舍号:");
        if (!id.empty()) {
            if (dormMgr.deleteDorm(id)) {
                g_tipMsg = "删除成功"; g_tipColor = 0x00AA00;
                drawCurrentScreen("", BLACK); //立即重绘界面
            }
            else {
                g_tipMsg = "删除失败: " + dormMgr.getLastError(); g_tipColor = RED;
                drawCurrentScreen("", BLACK); //立即重绘界面
            }
        }
    }
}

void handleFeeEvent(int x, int y) {
    if (isClickedReturn(x, y) || isClickedPagination(x, y)) return;

    if (Common::isPointInRect(x, y, 50, 100, BTN_W, BTN_H)) {
        Fee f;
        f.studentId = showInputBox("添加收费", "学号:");
        f.dormId = showInputBox("添加收费", "宿舍:");
        f.feeMonth = showInputBox("添加收费", "月份(YYYY-MM):");
        std::string waterStr = showInputBox("添加收费", "水费:");
        std::string elecStr = showInputBox("添加收费", "电费:");

        if (!Common::stringToDouble(waterStr, f.waterFee) || !Common::stringToDouble(elecStr, f.electricFee)) {
            g_tipMsg = "费用格式错误"; g_tipColor = RED; return;
        }

        if (feeMgr.addFee(f)) {
            g_tipMsg = "添加成功"; g_tipColor = 0x00AA00;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
        else {
            g_tipMsg = "添加失败: " + feeMgr.getLastError(); g_tipColor = RED;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
    }
    else if (Common::isPointInRect(x, y, 180, 100, BTN_W, BTN_H)) {
        std::string id = showInputBox("修改状态", "输入收费ID:");
        if (id.empty()) return;

        //创建一个空的日期对象，让数据库在状态变为已付时自动设置当前日期
        Date emptyDate;
        if (feeMgr.updatePayStatus(id, emptyDate)) {
            g_tipMsg = "状态修改成功，已支付"; g_tipColor = 0x00AA00;
            //立即重绘界面以显示更新后的状态
            drawCurrentScreen("", BLACK);
        }
        else {
            g_tipMsg = "修改失败: " + feeMgr.getLastError(); g_tipColor = RED;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
    }
    else if (Common::isPointInRect(x, y, 310, 100, BTN_W, BTN_H)) {
        std::string id = showInputBox("删除记录", "输入收费ID:");
        if (!id.empty()) {
            if (feeMgr.deleteFee(id)) {
                g_tipMsg = "删除成功"; g_tipColor = 0x00AA00;
                drawCurrentScreen("", BLACK); //立即重绘界面
            }
            else {
                g_tipMsg = "删除失败: " + feeMgr.getLastError(); g_tipColor = RED;
                drawCurrentScreen("", BLACK); //立即重绘界面
            }
        }
    }
}

void handleRepairEvent(int x, int y) {
    if (isClickedReturn(x, y) || isClickedPagination(x, y)) return;

    if (Common::isPointInRect(x, y, 50, 100, BTN_W, BTN_H)) {
        Repair r;
        r.studentId = showInputBox("添加报修", "学号:");
        r.dormId = showInputBox("添加报修", "宿舍:");
        r.repairContent = showInputBox("添加报修", "内容:");

        if (repairMgr.addRepair(r)) {
            g_tipMsg = "提交成功"; g_tipColor = 0x00AA00;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
        else {
            g_tipMsg = "提交失败: " + repairMgr.getLastError(); g_tipColor = RED;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
    }
    else if (Common::isPointInRect(x, y, 180, 100, BTN_W, BTN_H)) {
        std::string id = showInputBox("处理报修", "输入报修ID:");
        if (id.empty()) return;

        if (repairMgr.updateHandleStatus(id, RepairStatus::COMPLETED)) {
            g_tipMsg = "处理成功：状态已更新，处理日期已设置为当前日期"; g_tipColor = 0x00AA00;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
        else {
            g_tipMsg = "处理失败: " + repairMgr.getLastError(); g_tipColor = RED;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
    }
    else if (Common::isPointInRect(x, y, 310, 100, BTN_W, BTN_H)) {
        std::string id = showInputBox("删除记录", "输入报修ID:");
        if (!id.empty()) {
            if (repairMgr.deleteRepair(id)) {
                g_tipMsg = "删除成功"; g_tipColor = 0x00AA00;
                drawCurrentScreen("", BLACK); //立即重绘界面
            }
            else {
                g_tipMsg = "删除失败: " + repairMgr.getLastError(); g_tipColor = RED;
                drawCurrentScreen("", BLACK); //立即重绘界面
            }
        }
    }
}

void handleVisitorEvent(int x, int y) {
    if (isClickedReturn(x, y) || isClickedPagination(x, y)) return;

    if (Common::isPointInRect(x, y, 50, 100, BTN_W, BTN_H)) {
        Visitor v;
        
        v.visitorName = showInputBox("登记访客", "访客姓名:");
        if (v.visitorName.empty()) return;
        
        v.gender = showInputBox("登记访客", "性别(男/女):");
        if (v.gender.empty()) return;
        if (v.gender != "男" && v.gender != "女") {
            g_tipMsg = "性别格式错误（应为'男'或'女'）"; g_tipColor = RED; return;
        }
        
        v.idCard = showInputBox("登记访客", "身份证号:");
        if (v.idCard.empty()) return;
        
        v.dormId = showInputBox("登记访客", "宿舍号:");
        if (v.dormId.empty()) return;
        
        v.visitReason = showInputBox("登记访客", "访问事由:");
        if (v.visitReason.empty()) return;

        std::string timeStr = showInputBox("登记访客", "来访时间(HH:MM 格式，如09:30，只支持英文冒号):");
        if (timeStr.empty()) return;
        if (timeStr.length() < 5) {
            v.visitTime = Common::getCurrentDateTimeStr().substr(11, 5);
        }
        else {
            v.visitTime = Common::normalizeTimeFormat(timeStr);
        }

        v.registerAdmin = showInputBox("登记访客", "登记管理员姓名:");
        if (v.registerAdmin.empty()) return;

        if (visitorMgr.addVisitor(v)) {
            g_tipMsg = "添加成功"; g_tipColor = 0x00AA00;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
        else {
            g_tipMsg = "添加失败: " + visitorMgr.getLastError(); g_tipColor = RED;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
    }
    else if (Common::isPointInRect(x, y, 180, 100, BTN_W, BTN_H)) {
        std::string id = showInputBox("访客离开", "输入访客ID:");
        if (id.empty()) return;

        std::string leaveTime = showInputBox("访客离开", "离开时间(HH:MM):");

        std::string leaveTimeOnly;

        if (leaveTime.empty()) {
            leaveTimeOnly = Common::getCurrentDateTimeStr().substr(11, 5);
        }
        else {
            leaveTimeOnly = leaveTime;
        }

        if (visitorMgr.recordLeave(id, Date(), leaveTimeOnly)) {
            g_tipMsg = "离开登记成功"; g_tipColor = 0x00AA00;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
        else {
            g_tipMsg = "登记失败: " + visitorMgr.getLastError(); g_tipColor = RED;
            drawCurrentScreen("", BLACK); //立即重绘界面
        }
    }

    else if (Common::isPointInRect(x, y, 310, 100, BTN_W, BTN_H)) {
        std::string id = showInputBox("删除记录", "输入访客ID:");
        if (!id.empty()) {
            if (visitorMgr.deleteVisitor(id)) {
                g_tipMsg = "删除成功"; g_tipColor = 0x00AA00;
                drawCurrentScreen("", BLACK); //立即重绘界面
            }
            else {
                g_tipMsg = "删除失败: " + visitorMgr.getLastError(); g_tipColor = RED;
                drawCurrentScreen("", BLACK); //立即重绘界面
            }
        }
    }
}


void drawStatistics() {
    drawBackground();
    drawTitleBar("系统统计");

    settextcolor(BLACK);
    settextstyle(24, 0, "宋体");
    
    int startY = 150;
    int gap = 50;
    
    int studentCount = studentMgr.getStudentTotalCount();
    int dormCount = dormMgr.getDormTotalCount();
    int feeCount = feeMgr.getFeeTotalCount();
    int repairCount = repairMgr.getRepairTotalCount();
    int visitorCount = visitorMgr.getVisitorTotalCount();
    
    //获取更精确的统计数据
    int activeVisitorCount = visitorMgr.getActiveVisitorCount();
    int unpaidFeeCount = feeMgr.getUnpaidFeeCount();
    
    //使用_T()宏正确处理字符串
    std::string studentText = "学生总数: " + std::to_string(studentCount) + " 人";
    std::string dormText = "宿舍总数: " + std::to_string(dormCount) + " 间";
    std::string feeText = "收费记录: " + std::to_string(feeCount) + " 条";
    std::string repairText = "报修记录: " + std::to_string(repairCount) + " 条";
    std::string visitorText = "访客记录: " + std::to_string(visitorCount) + " 条";
    
    outtextxy(100, startY, _T(studentText.c_str()));
    outtextxy(100, startY + gap, _T(dormText.c_str()));
    outtextxy(100, startY + gap * 2, _T(feeText.c_str()));
    outtextxy(100, startY + gap * 3, _T(repairText.c_str()));
    outtextxy(100, startY + gap * 4, _T(visitorText.c_str()));
    
    setfillcolor(0xE6F3FF);
    fillrectangle(WINDOW_W - 300, startY, WINDOW_W - 100, startY + gap * 4);
    settextcolor(0x0066CC);
    settextstyle(20, 0, "宋体");
    outtextxy(WINDOW_W - 280, startY + 10, _T("系统概览"));
    
    settextcolor(0x333333);
    settextstyle(14, 0, "宋体");
    
    std::string usageRate = "宿舍使用率: ";
    if (dormCount > 0) {
        double totalCapacity = dormCount * 4.0;
        double usageRateValue = (studentCount / totalCapacity) * 100;
        usageRate += std::to_string(static_cast<int>(usageRateValue)) + "%";
    } else {
        usageRate += "0%";
    }
    outtextxy(WINDOW_W - 280, startY + 50, _T(usageRate.c_str()));
    
    int unhandledRepairs = repairMgr.getUnfinishedRepairCount();
    std::string repairInfo = "未处理报修: " + std::to_string(unhandledRepairs) + " 条";
    outtextxy(WINDOW_W - 280, startY + 80, _T(repairInfo.c_str()));
    
    int unpaidFees = unpaidFeeCount;
    std::string feeInfo = "未支付费用: " + std::to_string(unpaidFees) + " 条";
    outtextxy(WINDOW_W - 280, startY + 110, _T(feeInfo.c_str()));
    
    int activeVisitors = activeVisitorCount;
    std::string visitorInfo = "当前访客: " + std::to_string(activeVisitors) + " 人";
    outtextxy(WINDOW_W - 280, startY + 140, _T(visitorInfo.c_str()));
    
    settextcolor(0x666666);
    settextstyle(14, 0, "宋体");
    std::string timeInfo = "更新时间: " + Common::getCurrentDateTimeStr();
    outtextxy(100, startY + gap * 5 + 20, _T(timeInfo.c_str()));
}






//显示学生缴费查询结果对话框
void showStudentFeeQueryResult(const std::vector<StudentDormFeeInfo>& results) {
    const int DIALOG_W = 900;
    const int DIALOG_H = 500;
    const int BTN_W = 80;
    const int BTN_H = 30;
    const int ROW_H = 25;
    
    int dialogX = (WINDOW_W - DIALOG_W) / 2;
    int dialogY = (WINDOW_H - DIALOG_H) / 2;
    
    //绘制对话框背景
    setfillcolor(0xFFFFFF); //WHITE
    fillrectangle(dialogX, dialogY, dialogX + DIALOG_W, dialogY + DIALOG_H);
    
    //绘制标题栏
    setfillcolor(0x0066CC);
    fillrectangle(dialogX, dialogY, dialogX + DIALOG_W, dialogY + 30);
    
    settextcolor(WHITE);
    settextstyle(16, 0, _T("宋体"));
    outtextxy(dialogX + 10, dialogY + 7, _T("学生缴费查询结果"));
    
    //绘制关闭按钮
    setfillcolor(0xCC0000);
    fillrectangle(dialogX + DIALOG_W - 30, dialogY + 5, dialogX + DIALOG_W - 10, dialogY + 25);
    settextcolor(WHITE);
    settextstyle(12, 0, "宋体");
    outtextxy(dialogX + DIALOG_W - 25, dialogY + 8, _T("X"));
    
    //绘制表格标题
    int y = dialogY + 40;
    std::vector<std::string> headers = {"学号", "姓名", "宿舍号", "月份", "水费", "电费", "总费用", "缴费状态"};
    int colWidth = (DIALOG_W - 20) / static_cast<int>(headers.size());
    
    setfillcolor(0xCCCCCC);
    for (int i = 0; i < static_cast<int>(headers.size()); ++i) {
        int lx = dialogX + 10 + i * colWidth;
        int rx = dialogX + 10 + (i + 1) * colWidth;
        fillrectangle(lx, y, rx, y + ROW_H);
        outtextxy(lx + 5, y + 5, _T(headers[static_cast<size_t>(i)].c_str()));
    }
    
    y += ROW_H;
    
    //显示查询结果
    settextcolor(BLACK);
    settextstyle(12, 0, _T("宋体"));
    
    for (const auto& info : results) {
        int x = dialogX + 10;
        
        outtextxy(x + 5, y + 5, _T(info.studentId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(info.studentName.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(info.dormId.c_str())); x += colWidth;
        outtextxy(x + 5, y + 5, _T(info.feeMonth.c_str())); x += colWidth;
        
        std::stringstream waterStream;
        waterStream << std::fixed << std::setprecision(2) << info.waterFee;
        outtextxy(x + 5, y + 5, _T(waterStream.str().c_str())); x += colWidth;
        
        std::stringstream elecStream;
        elecStream << std::fixed << std::setprecision(2) << info.electricFee;
        outtextxy(x + 5, y + 5, _T(elecStream.str().c_str())); x += colWidth;
        
        std::stringstream totalStream;
        totalStream << std::fixed << std::setprecision(2) << info.totalFee;
        outtextxy(x + 5, y + 5, _T(totalStream.str().c_str())); x += colWidth;
        
        std::string statusStr = (info.payStatus == PayStatus::PAID) ? "已缴费" : "未缴费";
        outtextxy(x + 5, y + 5, _T(statusStr.c_str()));
        
        y += ROW_H;
        
        //如果结果太多，显示滚动条
        if (y > dialogY + DIALOG_H - 50) {
            break; //只显示第一页结果
        }
    }
    
    //显示结果数量
    std::string resultCount = "共找到 " + std::to_string(results.size()) + " 条记录";
    outtextxy(dialogX + 10, dialogY + DIALOG_H - 30, _T(resultCount.c_str()));
    
    //等待用户点击关闭
    bool dialogActive = true;
    while (dialogActive) {
        ExMessage msg;
        if (peekmessage(&msg, EX_MOUSE)) {
            if (msg.message == WM_LBUTTONDOWN) {
                //检查是否点击关闭按钮
                if (msg.x >= dialogX + DIALOG_W - 30 && msg.x <= dialogX + DIALOG_W - 10 &&
                    msg.y >= dialogY + 5 && msg.y <= dialogY + 25) {
                    dialogActive = false;
                }
            }
        }
        Sleep(10);
    }
    
    //重新绘制当前界面来清除对话框
    drawCurrentScreen("", BLACK);
}

//显示学生信息查询结果对话框
void showStudentInfoQueryResult(const Student& student) {
    const int DIALOG_W = 600;
    const int DIALOG_H = 400;
    const int BTN_W = 80;
    const int BTN_H = 30;
    const int ROW_H = 25;

    int dialogX = (WINDOW_W - DIALOG_W) / 2;
    int dialogY = (WINDOW_H - DIALOG_H) / 2;

    //绘制对话框背景
    setfillcolor(0xFFFFFF); //WHITE
    fillrectangle(dialogX, dialogY, dialogX + DIALOG_W, dialogY + DIALOG_H);

    //绘制标题栏
    setfillcolor(0x0066CC);
    fillrectangle(dialogX, dialogY, dialogX + DIALOG_W, dialogY + 30);

    settextcolor(WHITE);
    settextstyle(16, 0, _T("宋体"));
    outtextxy(dialogX + 10, dialogY + 7, _T("学生信息查询结果"));

    //绘制关闭按钮
    setfillcolor(0xCC0000);
    fillrectangle(dialogX + DIALOG_W - 30, dialogY + 5, dialogX + DIALOG_W - 10, dialogY + 25);
    settextcolor(WHITE);
    settextstyle(12, 0, "宋体");
    outtextxy(dialogX + DIALOG_W - 25, dialogY + 8, _T("X"));

    //显示学生信息
    int y = dialogY + 50;
    settextcolor(BLACK);
    settextstyle(14, 0, _T("宋体"));

    //在对话框中绘制学生信息
    std::vector<std::string> infoLines = {
        "学号: " + student.studentId,
        "姓名: " + student.studentName,
        "性别: " + student.gender,
        "年龄: " + std::to_string(student.age),
        "专业: " + student.major,
        "宿舍号: " + student.dormId,
        "电话: " + student.studentPhone,
        "入学日期: " + Common::dateToString(student.checkInDate)
    };

    for (const auto& line : infoLines) {
        outtextxy(dialogX + 20, y, _T(line.c_str()));
        y += 30;
    }

    //等待用户点击关闭
    bool dialogActive = true;
    while (dialogActive) {
        ExMessage msg;
        if (peekmessage(&msg, EX_MOUSE)) {
            if (msg.message == WM_LBUTTONDOWN) {
                //检查是否点击关闭按钮
                if (msg.x >= dialogX + DIALOG_W - 30 && msg.x <= dialogX + DIALOG_W - 10 &&
                    msg.y >= dialogY + 5 && msg.y <= dialogY + 25) {
                    dialogActive = false;
                }
            }
        }
        Sleep(10);
    }
    //重新绘制当前界面来清除对话框
    drawCurrentScreen("", BLACK);
}