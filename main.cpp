#include <iostream>
#include <string>
#include <graphics.h>
#include <conio.h>

#include "Common.h"
#include "DBHelper.h"
#include "AdminManager.h"

#include "GUI.h"
#include "MultiTableQueryTypes.h"


// 全局变量定义
UIState currentState;
PageParam g_pageParam;
std::string g_tipMsg;
COLORREF g_tipColor = 0x000000; 

// 多表查询类型枚举定义已移至MultiTableQueryTypes.h

// 当前多表查询类型
MultiTableQueryType currentQueryType = MultiTableQueryType::STUDENT_DORM_INFO;

const int APP_WIN_W = 1024;
const int APP_WIN_H = 768;

// 背景图片路径和变量
const char* BACKGROUND_IMAGE_PATH = "background.bmp";
IMAGE g_backgroundImage;
bool g_backgroundLoaded = false;

// 加载背景图片函数
bool loadBackgroundImage() {
    if (g_backgroundLoaded) return true;
    
    // 尝试加载背景图片
    loadimage(&g_backgroundImage, BACKGROUND_IMAGE_PATH);
    
    // 检查是否加载成功
    if (g_backgroundImage.getwidth() > 0 && g_backgroundImage.getheight() > 0) {
        g_backgroundLoaded = true;
        std::cout << "背景图片加载成功: " << BACKGROUND_IMAGE_PATH << std::endl;
        return true;
    }
    
    std::cout << "背景图片加载失败，将使用默认背景色" << std::endl;
    return false;
}

// Login 界面布局
const int LOGIN_BOX_W = 420;
const int LOGIN_BOX_H = 220;
const int LOGIN_BTN_W = 120;
const int LOGIN_BTN_H = 40;

AdminManager adminMgr; // 用于登录校验

// 在 Login 界面绘制简单 UI
void drawLoginScreen(const std::string& tip, COLORREF tipColor);

// 绘制当前界面函数声明
void drawCurrentScreen(const std::string& tip, COLORREF tipColor);



// 判断点是否在矩形内（静态）
inline bool ptInRect(int x, int y, int rx, int ry, int rw, int rh) {
    return (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh);
}

// 处理 Login 界面的鼠标点击（静态）
// 返回值：true-状态已改变需要重绘，false-状态未改变
bool handleLoginClick(int x, int y, std::string& outTip, COLORREF& outTipColor) {
    int boxX = (APP_WIN_W - LOGIN_BOX_W) / 2;
    int boxY = 160;
    int btnY = boxY + 130;
    int btnX1 = boxX + 60;
    int btnX2 = boxX + LOGIN_BOX_W - 60 - LOGIN_BTN_W;

    if (ptInRect(x, y, btnX1, btnY, LOGIN_BTN_W, LOGIN_BTN_H)) {
        char idBuf[128] = { 0 };
        char pwdBuf[128] = { 0 };

        InputBox(idBuf, sizeof(idBuf), "请输入管理员账号(例: admin01):", "管理员登录");
        std::string adminId = std::string(idBuf);
        if (adminId.empty()) {
            outTip = "账号不能为空";
            outTipColor = RED;
            // 设置全局提示信息，确保能显示
            g_tipMsg = outTip;
            g_tipColor = outTipColor;
            return true;
        }

        InputBox(pwdBuf, sizeof(pwdBuf), "请输入密码:", "管理员登录");
        std::string pwd = std::string(pwdBuf);
        if (pwd.empty()) {
            outTip = "密码不能为空";
            outTipColor = RED;
            // 设置全局提示信息
            g_tipMsg = outTip;
            g_tipColor = outTipColor;
            return true;
        }

        if (adminMgr.verifyLogin(adminId, pwd)) {
            currentState = UIState::MAIN;
            outTip = "";
            outTipColor = BLACK;
            return true;
        }
        else {
            // 确保设置全局提示变量
            g_tipMsg = "登录失败: " + adminMgr.getLastError();
            g_tipColor = RED;
            return true; // 触发重绘
        }
    }

    if (ptInRect(x, y, btnX2, btnY, LOGIN_BTN_W, LOGIN_BTN_H)) {
        currentState = UIState::EXIT;
        return true;
    }

    return false;
}

// 绘制当前界面
void drawCurrentScreen(const std::string& tip, COLORREF tipColor) {
    if (currentState == UIState::LOGIN) {
        drawLoginScreen(tip, tipColor);
    }
    else if (currentState == UIState::MAIN) {
        drawBackground();
        drawMainMenu();
    }
    else if (currentState == UIState::STUDENT_MANAGE) {
        drawBackground();
        drawStudentManage();
    }
    else if (currentState == UIState::DORM_MANAGE) {
        drawBackground();
        drawDormManage();
    }
    else if (currentState == UIState::FEE_MANAGE) {
        drawBackground();
        drawFeeManage();
    }
    else if (currentState == UIState::REPAIR_MANAGE) {
        drawBackground();
        drawRepairManage();
    }
    else if (currentState == UIState::VISITOR_MANAGE) {
        drawBackground();
        drawVisitorManage();
    }
    else if (currentState == UIState::STATISTICS) {
        drawBackground();
        drawStatistics();
    }
}

// 分发鼠标点击事件
void dispatchMouseClick(int x, int y) {
    if (currentState == UIState::LOGIN) {
        // 确保使用全局提示变量
        bool changed = handleLoginClick(x, y, g_tipMsg, g_tipColor);
        
        if (changed) {
            // 强制重绘界面
            drawCurrentScreen(g_tipMsg, g_tipColor);
            // 刷新界面显示
            FlushBatchDraw();
            
            // 如果是错误提示，保持显示3秒
            if (!g_tipMsg.empty() && g_tipColor == RED) {
                Common::delay(3000);
                g_tipMsg = "";
                g_tipColor = BLACK;
                drawCurrentScreen("", BLACK); // 清除提示
            }
        }
        return;
    }
    // 添加主菜单事件处理
    if (currentState == UIState::MAIN) {
        handleMainMenu(x, y);
    }
    else if (currentState == UIState::STUDENT_MANAGE) {
        handleStudentEvent(x, y);
    }
    else if (currentState == UIState::DORM_MANAGE) {
        handleDormEvent(x, y);
    }
    else if (currentState == UIState::FEE_MANAGE) {
        handleFeeEvent(x, y);
    }
    else if (currentState == UIState::REPAIR_MANAGE) {
        handleRepairEvent(x, y);
    }
    else if (currentState == UIState::VISITOR_MANAGE) {
        handleVisitorEvent(x, y);
    }
    else if (currentState == UIState::STATISTICS) {
        // 统计页面只需要处理返回主菜单按钮
        if (Common::isPointInRect(x, y, APP_WIN_W - 150, 30, 120, 40)) {
            currentState = UIState::MAIN;
        }
    }

}

int main() {
    std::string dbHost = "127.0.0.1";
    std::string dbUser = "root";
    std::string dbPwd = "780219";
    std::string dbName = "dorm_management";
    unsigned int dbPort = 3306;

    if (!DBHelper::getInstance().connect(dbHost, dbUser, dbPwd, dbName, dbPort)) {
        std::cerr << "数据库连接失败: " << DBHelper::getInstance().getLastError().errorMsg << std::endl;
        std::cerr << "程序将以离线模式启动 GUI（仍可测试界面，但涉及 DB 的功能会失败）。" << std::endl;
    }
    else {
        std::cout << "数据库连接成功" << std::endl;
    }

    initgraph(APP_WIN_W, APP_WIN_H);
    
    // 加载背景图片
    loadBackgroundImage();
    
    currentState = UIState::LOGIN;

    ExMessage msg;
    const int loopDelayMs = 30;

    drawLoginScreen("", COLOR_TEXT_MAIN);

    while (true) {
        bool needRedraw = false;
        
        while (peekmessage(&msg, EX_MOUSE)) {
            if (msg.message == WM_LBUTTONDOWN) {
                int cx = msg.x;
                int cy = msg.y;
                dispatchMouseClick(cx, cy);
                needRedraw = true;
            }
        }

        if (needRedraw) {
            drawCurrentScreen("", BLACK);
        }

        if (currentState == UIState::EXIT) break;

        Common::delay(loopDelayMs);
    }

    cleardevice();
    closegraph();

    if (DBHelper::getInstance().isConnected()) {
        DBHelper::getInstance().disconnect();
    }

    std::cout << "程序已退出" << std::endl;
    return 0;
}