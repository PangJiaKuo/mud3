// Game：游戏主控类 —— 场景管理 / 命令处理 / 谜题阶段 / 存档 / 视频播放
#ifndef GAME_H
#define GAME_H

#include "Item.h"
#include "Player.h"
#include <string>
#include <map>
#include <vector>

// 游戏全局状态
enum class GameState {
    Menu,      // 主菜单
    Playing,   // 游戏中
    GameOver,  // 失败（密码错误超限）
    Won        // 通关
};

// 谜题推进阶段 —— 解谜必须按顺序通过
//   Exploring → HaveAllElements → HaveAllNumbers → PasswordEntered → Completed
enum class PuzzlePhase {
    Exploring,         // 探索：收集工具与四元素
    HaveAllElements,   // 四元素集齐，提示观察提取数字
    HaveAllNumbers,    // 四个数字全部提取，提示排列顺序
    PasswordEntered,   // 密码正确，进入按压符号阶段
    Completed          // 按压顺序正确，通关
};

class Game {
public:
    Game();
    ~Game();

    void run();  // 主循环：菜单 → 游戏 → 结局

private:
    // ── 初始化 ──
    void initWorld();        // 创建所有场景物品（工具、元素、线索、杂物、铁门）
    void initGameFlags();    // 初始化进度标志（全部 false）

    // ── 界面 ──
    void showIntro();              // 开场剧情 + 播放视频
    void showMainMenu();           // 主菜单：新游戏/读档/帮助/退出
    void showSceneDescription();   // 显示当前场景及可互动区域
    void showEnding();             // 通关结局 + 播放视频
    void playVideo(const std::string& filename, const std::string& skipMessage);

    // ── 命令处理 ──
    void processCommand(const std::string& input);
    std::vector<std::string> tokenize(const std::string& input);

    void cmdObserve(const std::vector<std::string>& args);  // 观察物品
    void cmdTake(const std::vector<std::string>& args);     // 拾取物品
    void cmdUse(const std::vector<std::string>& args);      // 使用工具
    void cmdEnter(const std::vector<std::string>& args);    // 输入密码
    void cmdPress(const std::vector<std::string>& args);    // 按压元素符号
    void cmdHint();       // 提示
    void cmdSave();       // 保存
    void cmdLoad();       // 加载
    void cmdRestart();    // 重新开始
    void cmdHelp();       // 帮助
    void cmdLook();       // 查看场景
    void cmdInventory();  // 查看背包

    // ── 物品查找（精确匹配 → 子串匹配） ──
    Item* findWorldItem(const std::string& name);
    const Item* findWorldItem(const std::string& name) const;

    // ── 谜题逻辑 ──
    void extractNumberFromItem(Item& item);   // 观察元素物品 → 提取隐藏数字
    void checkAllNumbersExtracted();          // 检查四个数字是否全部提取
    void checkAllElementsCollected();         // 检查四元素是否全部收集
    void onCorrectPassword();                 // 密码正确 → 进入按压符号阶段
    void onWrongPassword();                   // 密码错误 → 3次掉线索，5次 GameOver

    // ── 工具函数 ──
    std::string getElementName(ElementType elem) const;     // "地"/"水"/"火"/"风"
    std::string getElementSymbol(ElementType elem) const;   // ⛰/💧/🔥/🌬
    ElementType stringToElement(const std::string& str) const;
    std::string toLower(const std::string& str) const;      // 转小写（跳过中文）

    // ── 存档 ──
    void saveToFile(const std::string& filename = "savegame.dat");
    void loadFromFile(const std::string& filename = "savegame.dat");

    // ── 环境氛围 ──
    std::string getAmbientMessage() const;  // 每5回合轮询输出一条环境描述

    // ── 数据成员 ──
    std::map<std::string, Item> worldItems_;   // 场景物品（按名索引）
    std::vector<std::string> worldItemOrder_;  // 物品添加顺序（用于显示）

    Player player_;

    GameState state_;           // 当前游戏状态
    PuzzlePhase phase_;         // 当前谜题阶段

    int passwordAttempts_;                       // 密码错误次数
    bool hintNoteDropped_;                       // 线索纸条是否已掉落（防重复）
    bool passwordSolved_;                        // 密码是否已正确输入
    std::vector<ElementType> pressedSymbols_;    // 已按下的元素符号（需按序：地水火风）

    std::map<std::string, bool> flags_;          // 进度标志（存档用）

    int turnCount_;                              // 回合计数（每5回合触发环境文本）

    std::vector<std::string> ambientMessages_;   // 环境氛围文本池
    mutable int ambientIndex_;                   // 当前轮到的文本索引
};

#endif