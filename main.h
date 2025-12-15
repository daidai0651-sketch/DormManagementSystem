#ifndef MAIN_H
#define MAIN_H

#include "Common.h"
#include <string>
#include <graphics.h>

// 声明 main.cpp 中的函数，供 GUI.cpp 调用
extern void drawCurrentScreen(const std::string& tip = "", COLORREF tipColor = BLACK);

#endif