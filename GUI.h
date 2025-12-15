#ifndef GUI_H
#define GUI_H

#include "Common.h"
#include "StudentManager.h"
#include "MultiTableQueryManager.h"
#include <vector>
#include <windows.h>

extern UIState currentState;
extern PageParam g_pageParam;
extern std::string g_tipMsg;
extern COLORREF g_tipColor;

extern const int APP_WIN_W;
extern const int APP_WIN_H;
extern IMAGE g_backgroundImage;
extern bool g_backgroundLoaded;

void drawBackground();
void drawLoginScreen(const std::string& tip = "", COLORREF tipColor = BLACK);
void drawMainMenu();
void drawStudentManage();
void drawDormManage();
void drawFeeManage();
void drawRepairManage();
void drawVisitorManage();

void drawStatistics();
void drawMultiTableQuery();
void drawCurrentScreen(const std::string& tip = "", COLORREF tipColor = BLACK);

void handleMainMenu(int x, int y);
void handleStudentEvent(int x, int y);
void handleDormEvent(int x, int y);
void handleFeeEvent(int x, int y);
void handleRepairEvent(int x, int y);
void handleVisitorEvent(int x, int y);

// 多表查询结果对话框函数声明
void showStudentDormQueryResult(const std::vector<StudentDormFeeInfo>& results);
void showStudentFeeQueryResult(const std::vector<StudentDormFeeInfo>& results);
void showStudentInfoQueryResult(const Student& student);

#endif
