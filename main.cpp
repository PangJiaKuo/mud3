#include "Game.h"
#include"windows.h"
int main() {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    Game game;
    game.run();
    return 0;
}
