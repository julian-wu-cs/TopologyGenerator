#include "Application.h"
#include <iostream>

// 全局常量
const unsigned int WINDOW_WIDTH = 1600;
const unsigned int WINDOW_HEIGHT = 900;

// 为什么用英文，是因为ImGui对中文支持不好，干脆全部用英文算了
const char* WINDOW_TITLE = "Digital Topology Generator--My Big Assignment";

int main() {
    Application app(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
    app.run();
    return 0;
}