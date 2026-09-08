// 入口：设置控制台UTF-8编码，启动游戏主循环
#include "Game.h"
#include "windows.h"

int main() {
    SetConsoleCP(65001);        // 控制台输入编码设为 UTF-8
    SetConsoleOutputCP(65001);  // 控制台输出编码设为 UTF-8
    Game game;
    game.run();
    return 0;
}