#ifndef GAME_H
#define GAME_H

#include "Item.h"
#include "Player.h"
#include <string>
#include <map>
#include <vector>

// 游戏状态
enum class GameState {
    Menu,      // 主菜单
    Playing,   // 游戏中
    GameOver,
    Won        // 通关
};

// 谜题推进阶段
enum class PuzzlePhase {
    Exploring,         // 探索阶段：收集工具与元素
    HaveAllElements,   // 已集齐四元素
    HaveAllNumbers,    // 已提取全部数字
    PasswordEntered,  // 密码正确
    Completed         // 试炼完成
};

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    void initWorld();        // 初始化场景物品
    void initGameFlags();    // 初始化进度标志

    void showIntro();
    void showMainMenu();
    void showSceneDescription();
    void showEnding();
    void playVideo(const std::string& filename, const std::string& skipMessage);
    void processCommand(const std::string& input);
    std::vector<std::string> tokenize(const std::string& input);

    void cmdObserve(const std::vector<std::string>& args);
    void cmdTake(const std::vector<std::string>& args);
    void cmdUse(const std::vector<std::string>& args);
    void cmdEnter(const std::vector<std::string>& args);
    void cmdPress(const std::vector<std::string>& args);
    void cmdHint();
    void cmdSave();
    void cmdLoad();
    void cmdRestart();
    void cmdHelp();
    void cmdLook();
    void cmdInventory();

    Item* findWorldItem(const std::string& name);
    const Item* findWorldItem(const std::string& name) const;

    void extractNumberFromItem(Item& item);   // 观察元素物品并提取数字
    void checkAllNumbersExtracted();
    void checkAllElementsCollected();
    void onCorrectPassword();
    void onWrongPassword();                    // 错误3次掉落线索纸条

    std::string getElementName(ElementType elem) const;
    std::string getElementSymbol(ElementType elem) const;
    ElementType stringToElement(const std::string& str) const;
    std::string toLower(const std::string& str) const;

    void saveToFile(const std::string& filename = "savegame.dat");
    void loadFromFile(const std::string& filename = "savegame.dat");

    std::string getAmbientMessage() const;    // 轮询返回环境氛围文本

    std::map<std::string, Item> worldItems_;   // 场景中所有物品（按名索引）
    std::vector<std::string> worldItemOrder_;  // 物品添加顺序
    Player player_;

    GameState state_;
    PuzzlePhase phase_;

    int passwordAttempts_;
    bool hintNoteDropped_;                    // 线索纸条是否已掉落
    bool passwordSolved_;
    std::vector<ElementType> pressedSymbols_; // 已按下的元素符号序列

    std::map<std::string, bool> flags_;       // 各类进度标志

    int turnCount_;                            // 回合计数（用于触发环境文本）

    std::vector<std::string> ambientMessages_;
    mutable int ambientIndex_;
};

#endif
