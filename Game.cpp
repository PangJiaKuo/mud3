// Game类实现：MUD密室逃脱游戏主逻辑
#include "Game.h"
#include<string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <windows.h>
#include <conio.h>

namespace {
// ====== 颜色工具：基于 Windows Console API ======
// 颜色枚举（对应 FOREGROUND_* 常量按位组合）
enum class Color : WORD {
    Default   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,            // 白
    Title     = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,      // 亮青：标题/边框
    Narration = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // 亮白：旁白
    Dialogue  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,       // 亮黄：对话
    System    = FOREGROUND_GREEN | FOREGROUND_INTENSITY,                        // 亮绿：系统提示
    Item      = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,        // 亮紫：物品/元素
    Error     = FOREGROUND_RED | FOREGROUND_INTENSITY,                          // 亮红：错误
    Hint      = FOREGROUND_GREEN | FOREGROUND_INTENSITY,                        // 亮绿：提示
    Ambient   = FOREGROUND_GREEN | FOREGROUND_BLUE,                             // 暗青：环境氛围
    Mute      = FOREGROUND_INTENSITY,                                           // 暗灰：次要信息
};

// RAII：构造时设色，析构时还原默认
struct ColorScope {
    ColorScope(Color c) {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h, static_cast<WORD>(c));
    }
    ~ColorScope() {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h, static_cast<WORD>(Color::Default));
    }
};

// 便捷输出宏：ColorScope + std::cout，单行
#define COUT(c, x) do { ColorScope _cs(c); std::cout << x; } while(0)
#define COUTLN(c, x) do { ColorScope _cs(c); std::cout << x << "\n"; } while(0)

// 计算字符串显示宽度（中文2，英文1）
int displayWidth(const std::string& s) {
    int w = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = s[i];
        if (c < 0x80)       { w += 1; i += 1; }
        else if ((c & 0xE0) == 0xC0) { w += 2; i += 2; }
        else if ((c & 0xF0) == 0xE0) { w += 2; i += 3; }
        else                { w += 2; i += 4; }
    }
    return w;
}

// 右侧填充空格至指定显示宽度
std::string padRight(const std::string& s, int width) {
    int dw = displayWidth(s);
    if (dw >= width) return s;
    return s + std::string(width - dw, ' ');
}

// 居中对齐至指定显示宽度
std::string centerText(const std::string& s, int width) {
    int dw = displayWidth(s);
    if (dw >= width) return s;
    int left = (width - dw) / 2;
    int right = width - dw - left;
    return std::string(left, ' ') + s + std::string(right, ' ');
}
}

Game::Game()
    : state_(GameState::Menu), phase_(PuzzlePhase::Exploring),
      passwordAttempts_(0), hintNoteDropped_(false), passwordSolved_(false),
      turnCount_(0), ambientIndex_(0) {
    initWorld();
    initGameFlags();
}

Game::~Game() {}

// 初始化场景中所有物品：工具、元素、线索、杂物、铁门
void Game::initWorld() {
    worldItems_.clear();
    worldItemOrder_.clear();

    // 工具物品加入场景
    auto addItem = [this](const Item& item) {
        worldItems_[item.getName()] = item;
        worldItemOrder_.push_back(item.getName());
    };

    Item shovel("小铲子", "一把黄铜制的铲子，手柄上刻着'挖掘真相'四字。", ItemType::Tool);
    shovel.setLocation("书桌抽屉");
    shovel.setDetailedDesc("黄铜打磨而成，小巧玲珑。手柄上的刻字似乎在暗示它的用途——有些真相，埋藏于泥土之下。");
    addItem(shovel);

    Item net("渔网", "细麻绳编织的渔网，手柄光滑。", ItemType::Tool);
    net.setLocation("水族箱旁的墙壁挂钩");
    net.setDetailedDesc("麻绳编织细密，足以捞起水底的小物件。手柄被岁月打磨得温润如玉。");
    addItem(net);

    Item tongs("火钳", "铁制的火钳，尖端因长期接触高温而发黑。", ItemType::Tool);
    tongs.setLocation("壁炉右侧地面");
    tongs.setDetailedDesc("厚重的铁钳，适合翻动壁炉中的灰烬。尖端发黑发亮，显然经常与火焰为伴。");
    addItem(tongs);

    // 四元素：地水火风，各自隐藏数字
    Item earthStone("地石板", "巴掌大的青石，正面刻着一个'地'字。", ItemType::Element);
    earthStone.setElementType(ElementType::Earth);
    earthStone.setLocation("盆栽土壤下");
    earthStone.setHiddenNumber(4);
    earthStone.setNumberClue("背面有四个整齐排列的浅坑，仿佛大地的呼吸孔。");
    earthStone.setDetailedDesc("青石温润，刻痕深有力道。翻过来看，背面有四个整齐排列的小坑——仿佛大地的呼吸孔。");
    addItem(earthStone);

    Item shell("水贝壳", "手掌大小的螺旋贝壳，表面刻着一个'水'字。", ItemType::Element);
    shell.setElementType(ElementType::Water);
    shell.setLocation("水族箱底部");
    shell.setHiddenNumber(2);
    shell.setNumberClue("壳面有两条天然凸纹，如同水流的波纹。");
    shell.setDetailedDesc("贝壳螺旋优美，刻字流畅。对着壁灯的光仔细端详，壳面上天然生成的两条凸纹清晰可见——像两条水流交汇。");
    addItem(shell);

    Item ironSheet("火铁片", "掌心大小的锈铁片，表面刻着一个'火'字。", ItemType::Element);
    ironSheet.setElementType(ElementType::Fire);
    ironSheet.setLocation("壁炉灰烬中");
    ironSheet.setHiddenNumber(3);
    ironSheet.setNumberClue("表面有三个圆孔，如同三团跃动的火焰。");
    ironSheet.setDetailedDesc("铁片锈迹斑驳，但刻字处仍可见金属光泽。举起对着光源，三个小圆孔透过光线——像三团跃动的火焰。");
    addItem(ironSheet);

    Item feather("风羽毛", "灰色大雁羽毛，羽管上刻着一个'风'字。", ItemType::Element);
    feather.setElementType(ElementType::Wind);
    feather.setLocation("无字书中");
    feather.setHiddenNumber(1);
    feather.setNumberClue("羽轴上有一道细微的刻痕，如风过无痕却唯一。");
    feather.setDetailedDesc("羽毛轻柔，羽管刻字精细。用放大镜仔细查看羽轴，可见一道细微的刻痕——如风过无痕却唯一。");
    addItem(feather);

    Item diary("日记本", "一本皮面笔记本，锁扣已锈坏。", ItemType::Clue);
    diary.setLocation("书桌左上角");
    diary.setDetailedDesc("翻开日记，内页夹着一张书签，上书：'数字藏于形，秩序生于自然。'\n日记中还记录了赫尔墨斯的各种炼坤研究笔记，提到了'四元素依循自然之序'的重要观念。");
    diary.setHint("仔细观察每件物品的形态特征，数字并非直接写出，而是藏于物品的结构之中。");
    addItem(diary);

    Item magnifier("放大镜", "一个打磨精致的黄铜放大镜。", ItemType::Tool);
    magnifier.setLocation("书桌散落的物品中");
    magnifier.setDetailedDesc("放大镜镜片光洁，手柄处刻有精细的花纹。它或许能帮助你看清细小的刻痕。");
    addItem(magnifier);

    Item parchment("羊皮纸", "泛黄的羊皮纸，上面书写着试炼规则。", ItemType::Clue);
    parchment.setLocation("书桌中央");
    parchment.setDetailedDesc("羊皮正面写着：'年轻的学者，欢迎来到我的试炼场。四元素——地、水、火、风——藏于这间书房之中。找到它们，理解它们的秩序，你便能重获自由。记住：万物皆由四元素构成，而四元素由'数'与'序'统一。'\n\n翻过背面，你看到一幅环形图：地→水→火→风，四个箭头首尾相接，形成一个循环。");
    parchment.setHint("羊皮纸背面的循环图暗示了元素的排列顺序。");
    parchment.setCollected(true);
    addItem(parchment);

    Item book("无字书", "一本书封面没有标题的书，颜色比其他书更白。", ItemType::Clue);
    book.setLocation("书架上");
    book.setDetailedDesc("这本书封面素白无字，与周围的彩色书籍格格不入。翻开内页，你发现其中夹着一根灰色的大雁羽毛！");
    book.setHint("书架上颜色异常的那本书可能藏着秘密。");
    addItem(book);

    Item furnace("壁炉", "石壁上的壁炉，炉膛内有余烬闪烁。", ItemType::Misc);
    furnace.setLocation("左侧石壁");
    furnace.setDetailedDesc("壁炉石质雕花古朴，余烬中似乎有东西在闪光。壁炉边缘可以看到一把【火钳】。");
    addItem(furnace);

    Item fishTank("水族箱", "半人高的水族箱，内有游鱼与水草。", ItemType::Misc);
    fishTank.setLocation("右侧靠墙");
    fishTank.setDetailedDesc("水族箱中，几条小鱼悠闲地游着，水草轻轻摇曳。箱底的沙砾间似乎有什么在闪光。墙上的挂钩上挂着【渔网】。");
    addItem(fishTank);

    Item plant("盆栽", "角落陶盆，种着一株枯槁的小树。", ItemType::Misc);
    plant.setLocation("角落");
    plant.setDetailedDesc("陶盆中的小树已经枯萎，但土壤看起来松动。仔细看，土壤下似乎埋着什么。");
    addItem(plant);

    Item bookshelf("书架", "占满整面墙的书架，书籍按颜色排列。", ItemType::Misc);
    bookshelf.setLocation("整面墙");
    bookshelf.setDetailedDesc("书籍从棕色到白色渐变排列，井然有序。但其中一本纯白色的【无字书】没有标题，显得格外突兀。");
    addItem(bookshelf);

    Item desk("书桌", "厚重的橡木书桌，桌面散落着书卷。", ItemType::Misc);
    desk.setLocation("房间中央");
    desk.setDetailedDesc("书桌上散落【日记本】【放大镜】和笔墨纸砚等杂物，在旁边还有一卷异常显眼的【羊皮纸】。抽屉未上锁，里面有一把【小铲子】。");
    addItem(desk);

    // 铁门：最终机关，需输入密码并按序按压元素符号
    Item ironDoor("铁门", "厚重铸铁的门，表面刻有四元素符号。", ItemType::Key);
    ironDoor.setLocation("唯一的出口");
    ironDoor.setDetailedDesc("铁门上有四个可旋转的数字圆盘（0-9），下方是四个圆形凹槽，分别刻着地、水、火、风四个元素符号。门紧紧锁着。");
    ironDoor.setHint("你需要按正确顺序按压四个符号。");
    addItem(ironDoor);

    Item stoneTable("石台", "房间中央一座齐腰高的石台。", ItemType::Misc);
    stoneTable.setLocation("房间中央");
    stoneTable.setDetailedDesc("台面光滑如镜，在烛光中反射出柔和的光泽。此刻它只是静静伫立，等待着什么。");
    addItem(stoneTable);
}

// 初始化各类进度标志
void Game::initGameFlags() {
    flags_["got_shovel"] = false;
    flags_["got_net"] = false;
    flags_["got_tongs"] = false;
    flags_["got_magnifier"] = false;
    flags_["got_diary"] = false;
    flags_["got_parchment"] = true;
    flags_["examined_diary"] = false;
    flags_["examined_parchment"] = true;
    flags_["elements_gathered"] = false;
    flags_["numbers_extracted"] = false;
    flags_["password_solved"] = false;
    flags_["hint_dropped"] = false;
    flags_["wind_collected"] = false;
}

// 主循环：根据状态分发到菜单/游戏/结局
void Game::run() {
    ambientMessages_ = {
        "壁灯的火焰摇曳着，在墙上投下跳动的影子。",
        "水族箱里的鱼撞了一下玻璃，发出轻柔的声响。",
        "空气中弥漫着陈年古籍的气息，混合着一丝神秘的香料味。",
        "一阵轻风不知从何处飘来，翻动了桌上的书页。",
        "石台上似乎泛起微光，但转瞬即逝。",
        "远处传来古钟的滴答声，在石墙间回响。",
        "盆栽的枯枝在微光中投下诡异的影子。",
        "书架上的书籍似乎在微微颤动，仿佛有生命。"
    };

    while (true) {
        if (state_ == GameState::Menu) {
            showMainMenu();
        } else if (state_ == GameState::Playing) {
            COUT(Color::Title, "\n> ");
            std::string input;
            std::getline(std::cin, input);
            if (input.empty()) continue;

            if (turnCount_ > 0 && turnCount_ % 5 == 0) {
                COUTLN(Color::Ambient, "\n" + getAmbientMessage());
            }

            processCommand(input);
            turnCount_++;

        } else if (state_ == GameState::Won) {
            showEnding();
            state_ = GameState::Menu;
        } else if (state_ == GameState::GameOver) {
            state_ = GameState::Menu;
        }
    }
}

// 主菜单：开始新游戏/读档/说明/退出
void Game::showMainMenu() {
    constexpr int kInner = 46;
    std::string border;
    border.reserve(kInner * 3);
    for (int i = 0; i < kInner; ++i)
        border += "═";

    std::cout << "\n";
    COUTLN(Color::Title, " " + border);
    COUTLN(Color::Title, " " + centerText("密室逃脱：炼坤术士的试炼", kInner) + "     ");
    COUTLN(Color::Title, " " + padRight("", kInner));
    COUTLN(Color::System, " " + padRight("  1. 开始新游戏", kInner) + " ");
    COUTLN(Color::System, " " + padRight("  2. 读取存档", kInner) + " ");
    COUTLN(Color::System, " " + padRight("  3. 操作说明", kInner) + " ");
    COUTLN(Color::System, " " + padRight("  4. 退出游戏", kInner) + " ");
    COUTLN(Color::Title, " " + padRight("", kInner));
    COUTLN(Color::Title, " " + border);
    COUT(Color::Hint, "请选择 [1-4]: ");

    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1") {
        initWorld();
        initGameFlags();
        player_.reset();
        phase_ = PuzzlePhase::Exploring;
        passwordAttempts_ = 0;
        hintNoteDropped_ = false;
        passwordSolved_ = false;
        pressedSymbols_.clear();
        turnCount_ = 0;
        state_ = GameState::Playing;
        showIntro();
    } else if (choice == "2") {
        loadFromFile();
        state_ = GameState::Playing;
        showSceneDescription();
    } else if (choice == "3") {
        cmdHelp();
    } else if (choice == "4") {
        COUTLN(Color::Dialogue, "\n愿炼坤术的智慧与你同在。再见。");
        exit(0);
    }
}
// 调用播放视频文件，可按键跳过
void Game::playVideo(const std::string& filename, const std::string& skipMessage)
{
    // 获取exe所在目录
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string exeDir(exePath);
    size_t pos = exeDir.find_last_of("\\/");
    if (pos != std::string::npos)
        exeDir = exeDir.substr(0, pos);

    // 视频完整路径
    std::string fullPath = exeDir + "\\" + filename;

    // 判断文件是否存在
    std::ifstream testFile(fullPath);
    if (!testFile.good())
        return;
    testFile.close();

    // 安全转宽字符（支持中文路径）
    auto ToWide = [](const std::string& str) -> std::wstring {
        int bufLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
        std::wstring wstr(bufLen, 0);
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], bufLen);
        return wstr;
    };

    std::wstring wVideoPath = ToWide(fullPath);
    // 播放器路径：mpc-hc.exe 和游戏exe放在同一个文件夹
    std::wstring playerPath = ToWide(exeDir + "\\mpc-hc.exe");

    // 启动参数：播放器打开视频。路径含空格必须加引号，否则CreateProcess会把首个空格前当作exe路径
    std::wstring cmdLine = L"\"" + playerPath + L"\" \"" + wVideoPath + L"\"";

    // CreateProcess 创建进程，保存句柄
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi{};

    BOOL createOk = CreateProcessW(
        nullptr,
        &cmdLine[0],
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!createOk)
    {
        std::cout << "无法启动播放器\n";
        return;
    }

    // 输出跳过提示
    std::cout << "\n" << skipMessage << "\n";
    std::cout << "（按任意键跳过...）" << std::flush;

    DWORD startTime = GetTickCount();
    const DWORD maxWaitTime = 5 * 60 * 1000;
    bool skipByKey = false;

    while (true)
    {
        // 检测按键跳过
        if (_kbhit())
        {
            _getch();
            skipByKey = true;
            break;
        }
        // 超时自动退出等待
        if (GetTickCount() - startTime > maxWaitTime)
        {
            break;
        }
        // 如果播放器自己关闭了（进程结束），直接跳出
        DWORD exitCode = 0;
        if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode != STILL_ACTIVE)
        {
            break;
        }

        Sleep(100);
    }

    // 杀掉播放器进程
    DWORD exitCode{};
    if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode == STILL_ACTIVE)
    {
        TerminateProcess(pi.hProcess, 0);
        WaitForSingleObject(pi.hProcess, INFINITE);
    }

    // 关闭句柄
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    std::cout << "\n";
}

// 开场剧情介绍
void Game::showIntro() {
    playVideo("opening.mp4", "按任意键跳过开场动画");
    std::cout << "\n";
    COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    std::cout << "\n";
    COUTLN(Color::Narration, "你是一位在坤家学院研究古代炼坤术的年轻学者。");
    COUTLN(Color::Narration, "某日，你收到一封用火漆封缄的邀请函，落款是传说中的");
    COUTLN(Color::Narration, "炼坤术士赫尔墨斯——一个据说已经活了三百年的神秘人物。");
    std::cout << "\n";
    COUTLN(Color::Narration, "你如约而至。宅邸比想象中更古老，藤蔓爬满石墙。");
    COUTLN(Color::Narration, "书房的门虚掩着，你推门而入，满目皆是古籍、仪器与奇异的标本。");
    COUTLN(Color::Narration, "正当你惊叹时，身后的门\"咔嗒\"一声自动锁死。");
    std::cout << "\n";
    COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    COUTLN(Color::System, "  试炼开始！");
    COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    std::cout << "\n";

    showSceneDescription();
}

// 显示当前场景可互动区域
void Game::showSceneDescription() {
    std::cout << "\n";
    COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    COUTLN(Color::Title, "  当前场景：赫尔墨斯的书房");
    COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    std::cout << "\n";
    COUTLN(Color::Mute, "书房为八角形石砌房间，直径约八步。墙壁嵌有昏暗的壁灯，");
    COUTLN(Color::Mute, "火焰摇曳，在墙上投下跳动的影子。地面铺着暗红色旧地毯。");
    COUTLN(Color::Mute, "中央有一座齐腰高的石台，台面光滑如镜。");
    std::cout << "\n";

    COUTLN(Color::Narration, "你注意到以下可互动的区域：");
    std::cout << "\n";

    std::vector<std::string> displayedItems = {
        "书桌", "书架", "壁炉", "水族箱", "盆栽", "铁门", "石台"
    };

    std::for_each(displayedItems.begin(), displayedItems.end(),
        [this](const std::string& name) {
            Item* item = findWorldItem(name);
            if (!item) return;
            std::string loc = item->getLocation();
            COUT(Color::Item, "  ◆ " + name);
            if (!loc.empty()) {
                COUT(Color::Mute, "  [" + loc + "]");
            }
            std::cout << "\n";
        });

    auto showAvailable = [this](const std::string& itemName, const std::string& location) {
        if (worldItems_.find(itemName) != worldItems_.end() && !worldItems_[itemName].isCollected()) {
            COUTLN(Color::Item, "  ◆ " + itemName + "  [" + location + "]");
        }
    };

    if (hintNoteDropped_ && worldItems_.find("线索纸条") != worldItems_.end()) {
        COUTLN(Color::Item, "  ◆ 线索纸条  [地上]");
    }

    std::cout << "\n";
    COUTLN(Color::Hint, "输入 '观察 [物品]' 查看物品详情");
    COUTLN(Color::Hint, "输入 '帮助' 查看所有可用指令");
    COUTLN(Color::Hint, "输入 '提示' 获取当前进度提示");
    std::cout << "\n";
}

// 通关结局文本
void Game::showEnding() {
    playVideo("ending.mp4", "按任意键跳过结局动画");
    std::cout << "\n";
    COUTLN(Color::Title, "══════════════════════════════════════════════");
    COUTLN(Color::Title, "                 通关结局                    ");
    COUTLN(Color::Title, "══════════════════════════════════════════════");
    std::cout << "\n";

    COUTLN(Color::Narration, "铁门缓缓向两侧滑开，露出外面点着烛火的走廊。");
    COUTLN(Color::Narration, "赫尔墨斯从阴影中走出，身披深绿长袍，面容苍老却目光如炬：");
    std::cout << "\n";

    COUTLN(Color::Dialogue, "\"你做到了。不是靠蛮力，而是靠观察、推理与耐心——");
    COUTLN(Color::Dialogue, "这正是炼坤术的真谛。那件'稀世珍宝'，");
    COUTLN(Color::Dialogue, "其实就是你刚刚通过的这场试炼本身。\"");
    std::cout << "\n";

    COUTLN(Color::Narration, "他指向书房中央的石台——你回头望去，");
    COUTLN(Color::Narration, "发现石台上缓缓升起一座四元素徽章，");
    COUTLN(Color::Narration, "金银铜铁四色交织，缓缓旋转，形成了球状");
    std::cout << "\n";

    COUTLN(Color::Dialogue, "\"现在，它是你的了。记住：");
    COUTLN(Color::Dialogue, "炼坤术不是点石成金，而是点化心智。\"");
    std::cout << "\n";

    COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    COUTLN(Color::System, "  恭喜通关！");
    COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    std::cout << "\n";
}

// 命令分发：首词识别指令，其余作为参数
void Game::processCommand(const std::string& input) {
    std::vector<std::string> tokens = tokenize(input);
    if (tokens.empty()) return;

    std::string cmd = toLower(tokens[0]);
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    if (cmd == "观察" || cmd == "observe" || cmd == "look" || cmd == "l") {
        cmdObserve(args);
    } else if (cmd == "拿" || cmd == "take" || cmd == "pick") {
        cmdTake(args);
    } else if (cmd == "用" || cmd == "use") {
        cmdUse(args);
    } else if (cmd == "输入" || cmd == "enter" || cmd == "input") {
        cmdEnter(args);
    } else if (cmd == "按" || cmd == "press") {
        cmdPress(args);
    } else if (cmd == "提示" || cmd == "hint") {
        cmdHint();
    } else if (cmd == "保存" || cmd == "save") {
        cmdSave();
    } else if (cmd == "加载" || cmd == "load") {
        cmdLoad();
    } else if (cmd == "重来" || cmd == "restart") {
        cmdRestart();
    } else if (cmd == "帮助" || cmd == "help" || cmd == "h") {
        cmdHelp();
    } else if (cmd == "背包" || cmd == "inventory" || cmd == "i") {
        cmdInventory();
    } else if (cmd == "场景" || cmd == "scene") {
        cmdLook();
    } else {
        COUTLN(Color::Error, "未知指令。输入 '帮助' 查看所有可用指令。");
    }
}

// 按空白拆分输入为token
std::vector<std::string> Game::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::stringstream ss(input);
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// 观察：查看物品详情，对元素物品触发数字提取
void Game::cmdObserve(const std::vector<std::string>& args) {
    if (args.empty()) {
        COUTLN(Color::Hint, "请告诉我要观察什么。");
        return;
    }

    std::string target = args[0];
    for (size_t i = 1; i < args.size(); ++i) {
        target += " " + args[i];
    }

    Item* item = findWorldItem(target);
    if (!item) {
        auto it = std::find_if(worldItems_.begin(), worldItems_.end(),
            [&target](const std::pair<const std::string, Item>& p) {
                return p.first.find(target) != std::string::npos ||
                       target.find(p.first) != std::string::npos;
            });
        if (it != worldItems_.end()) {
            item = &it->second;
        }
    }

    if (!item) {
        Item* playerItem = player_.findItem(target);
        if (playerItem) {
            COUTLN(Color::Narration, "你观察着手中的" + playerItem->getName() + "。");
            std::string desc = playerItem->getDetailedDesc().empty()
                ? playerItem->getDescription()
                : playerItem->getDetailedDesc();
            COUTLN(Color::Mute, desc);

            if (playerItem->getType() == ItemType::Element && !playerItem->isExamined()) {
                playerItem->setExamined(true);
                extractNumberFromItem(*playerItem);
            }
            return;
        }
        COUTLN(Color::Error, "你没有看到任何关于'" + target + "'的东西。");
        return;
    }

    COUTLN(Color::Narration, "你观察着" + item->getName() + "。");
    std::string desc = item->getDetailedDesc().empty()
        ? item->getDescription()
        : item->getDetailedDesc();
    COUTLN(Color::Mute, desc);

    if (item->getName() == "日记本" && !flags_["examined_diary"]) {
        flags_["examined_diary"] = true;
    }

    if (item->getName() == "无字书" && !flags_["wind_collected"]) {
        COUTLN(Color::System, "\n当你翻阅这本书时，一根灰色的大雁羽毛从书页间滑落！");
        flags_["wind_collected"] = true;
        if (worldItems_.find("风羽毛") != worldItems_.end() && !worldItems_["风羽毛"].isCollected()) {
            worldItems_["风羽毛"].setCollected(true);
            player_.addItem(worldItems_["风羽毛"]);
            player_.collectElement(ElementType::Wind);
            checkAllElementsCollected();
            COUTLN(Color::Item, "你获得了元素：【风】。");
        }
    }

    if (item->getType() == ItemType::Element && item->isCollected() && !item->isExamined()) {
        extractNumberFromItem(*item);
    }

    if (item->getName() == "线索纸条") {
        std::cout << "\n";
        COUTLN(Color::Hint, "纸条上写着：");
        std::cout << "\n";
        COUTLN(Color::Dialogue, "  \"地有四边，水有双流，火生三焰，风唯一向。\"");
        std::cout << "\n";
        COUTLN(Color::Item, "（地=4, 水=2, 火=3, 风=1）");
    }
}

// 拾取：工具类直接收入背包，元素/场景物件不可直接拿
void Game::cmdTake(const std::vector<std::string>& args) {
    if (args.empty()) {
        COUTLN(Color::Hint, "请告诉我要拿什么。");
        return;
    }

    std::string target = args[0];
    for (size_t i = 1; i < args.size(); ++i) {
        target += " " + args[i];
    }

    Item* item = findWorldItem(target);
    if (!item) {
        auto it = std::find_if(worldItems_.begin(), worldItems_.end(),
            [&target](const std::pair<const std::string, Item>& p) {
                return p.first.find(target) != std::string::npos;
            });
        if (it != worldItems_.end()) {
            item = &it->second;
        }
    }

    if (!item) {
        COUTLN(Color::Error, "你没有看到'" + target + "'可以拿取。");
        return;
    }

    if (item->getName() == "无字书") {
        std::cout << "你拿起了无字书。\n";
        std::cout << "但更重要的是——你注意到了书中夹着的东西！\n";
        if (!flags_["wind_collected"]) {
            flags_["wind_collected"] = true;
            if (worldItems_.find("风羽毛") != worldItems_.end() && !worldItems_["风羽毛"].isCollected()) {
                worldItems_["风羽毛"].setCollected(true);
                player_.addItem(worldItems_["风羽毛"]);
                player_.collectElement(ElementType::Wind);
                checkAllElementsCollected();
                std::cout << "一根灰色羽毛从书中滑落——上面刻着'风'字！\n";
                COUTLN(Color::Item, "你获得了元素：【风】。");
            }
        }
    } else if (item->getType() == ItemType::Misc || item->getType() == ItemType::Key || item->getType() == ItemType::Clue) {
        COUTLN(Color::Error, "这个无法直接拿走。你需要用其他方式来操作。");
        return;
    }

    if (item->isCollected()) {
        std::cout << "你已经拿过" << item->getName() << "了。\n";
        return;
    }

    if (item->getType() == ItemType::Element) {
        COUTLN(Color::Error, "你不能直接拿取" + item->getName() + "。你需要用合适的工具获取它。");
        return;
    }

    item->setCollected(true);
    player_.addItem(*item);

    if (item->getName() == "小铲子") flags_["got_shovel"] = true;
    if (item->getName() == "渔网") flags_["got_net"] = true;
    if (item->getName() == "火钳") flags_["got_tongs"] = true;
    if (item->getName() == "放大镜") flags_["got_magnifier"] = true;
    if (item->getName() == "日记本") flags_["got_diary"] = true;

    worldItems_.erase(item->getName());
    worldItemOrder_.erase(
        std::remove(worldItemOrder_.begin(), worldItemOrder_.end(), item->getName()),
        worldItemOrder_.end());

    std::cout << "你拿起了" << item->getName() << "。\n";
}

// 使用工具作用于目标：铲子挖土、渔网捞水、火钳翻灰、放大镜观察
void Game::cmdUse(const std::vector<std::string>& args) {
    if (args.empty()) {
        COUTLN(Color::Hint, "请指定工具和目标。例如：'用 小铲子 盆栽'");
        return;
    }

    std::vector<std::string> knownTools = {
        "小铲子", "铲子", "渔网", "火钳", "放大镜",
        "日记本", "羊皮纸", "无字书", "羽毛"
    };

    std::string combined;
    for (size_t i = 0; i < args.size(); ++i) {
        combined += args[i];
        if (i < args.size() - 1) combined += " ";
    }

    std::string actualToolName;
    std::string actualTargetName;

    for (const auto& toolCandidate : knownTools) {
        if (combined.find(toolCandidate) == 0) {
            actualToolName = toolCandidate;
            actualTargetName = combined.substr(toolCandidate.length());
            while (!actualTargetName.empty() && actualTargetName[0] == ' ') {
                actualTargetName = actualTargetName.substr(1);
            }
            break;
        }
    }

    if (actualToolName.empty()) {
        actualToolName = args[0];
        for (size_t i = 1; i < args.size(); ++i) {
            actualTargetName += (i > 1 ? " " : "") + args[i];
        }
    }

    if (actualTargetName.empty()) {
        COUTLN(Color::Hint, "请指定工具的使用目标。例如：'用 小铲子 盆栽'");
        return;
    }

    Item* actualTool = player_.findItem(actualToolName);

    if (!actualTool) {
        COUTLN(Color::Error, "你没有" + actualToolName + "。请先拾取它。");
        return;
    }

    Item* target = findWorldItem(actualTargetName);
    if (!target) {
        auto it = std::find_if(worldItems_.begin(), worldItems_.end(),
            [&actualTargetName](const std::pair<const std::string, Item>& p) {
                return p.first.find(actualTargetName) != std::string::npos;
            });
        if (it != worldItems_.end()) {
            target = &it->second;
        }
    }

    if (!target) {
        const Item* playerItem = player_.findItem(actualTargetName);
        if (playerItem) {
            std::cout << "你使用" << actualToolName << "没有什么作用。\n";
        } else {
            COUTLN(Color::Error, "没有找到'" + actualTargetName + "'。");
        }
        return;
    }

    if (actualToolName == "小铲子" || actualToolName == "铲子") {
        if (target->getName() == "盆栽") {
            if (worldItems_.find("地石板") != worldItems_.end() && !worldItems_["地石板"].isCollected()) {
                worldItems_["地石板"].setCollected(true);
                player_.addItem(worldItems_["地石板"]);
                player_.collectElement(ElementType::Earth);
                checkAllElementsCollected();
                COUTLN(Color::Narration, "你用铲子拨开盆栽的土壤，一块青石露了出来——上面刻着'地'字！");
                COUTLN(Color::Item, "你获得了元素：【地】。");
                target->setUsed(true);
            } else {
                COUTLN(Color::Mute, "你在盆栽里翻找，没有新的发现。");
            }
            return;
        } else if (target->getName() == "壁炉") {
            std::cout << "壁炉太过坚硬，铲子派不上用场。\n";
            return;
        } else if (target->getName() == "水族箱") {
            std::cout << "水族箱里都是水，铲子帮不上忙。\n";
            return;
        } else if (target->getName() == "书桌" || target->getName() == "抽屉") {
            COUTLN(Color::Hint, "抽屉没有上锁，直接用手拉开即可。");
            return;
        }
    }

    if (actualToolName == "渔网") {
        if (target->getName() == "水族箱" || actualTargetName == "水") {
            if (worldItems_.find("水贝壳") != worldItems_.end() && !worldItems_["水贝壳"].isCollected()) {
                worldItems_["水贝壳"].setCollected(true);
                player_.addItem(worldItems_["水贝壳"]);
                player_.collectElement(ElementType::Water);
                checkAllElementsCollected();
                std::cout << "你将渔网探入水族箱，在沙砾间捞起了一枚螺旋贝壳——上面刻着'水'字！\n";
                COUTLN(Color::Item, "你获得了元素：【水】。");
                target->setUsed(true);
            } else {
                std::cout << "你在水族箱中捞了一阵，什么也没找到。\n";
            }
            return;
        } else if (target->getName() == "壁炉") {
            COUTLN(Color::Error, "渔网无法操作壁炉。");
            return;
        } else if (target->getName() == "盆栽") {
            COUTLN(Color::Error, "渔网无法挖土。");
            return;
        }
    }

    if (actualToolName == "火钳") {
        if (target->getName() == "壁炉" || target->getName() == "灰烬" || actualTargetName == "火") {
            if (worldItems_.find("火铁片") != worldItems_.end() && !worldItems_["火铁片"].isCollected()) {
                worldItems_["火铁片"].setCollected(true);
                player_.addItem(worldItems_["火铁片"]);
                player_.collectElement(ElementType::Fire);
                checkAllElementsCollected();
                COUTLN(Color::Narration, "你用火钳翻动壁炉的灰烬，夹出了一块灼热的铁片——上面刻着'火'字！");
                COUTLN(Color::Item, "你获得了元素：【火】。");
                target->setUsed(true);
            } else {
                COUTLN(Color::Mute, "你翻动灰烬，没有发现新的东西。");
            }
            return;
        } else if (target->getName() == "水族箱") {
            std::cout << "火钳无法用来操作水族箱。\n";
            return;
        } else if (target->getName() == "盆栽") {
            std::cout << "火钳不适合挖掘。\n";
            return;
        }
    }

    if (actualToolName == "放大镜") {
        if (target->getType() == ItemType::Element && target->isCollected()) {
            if (!target->isExamined()) {
                target->setExamined(true);
                extractNumberFromItem(*target);
            } else {
                COUTLN(Color::Hint, "你用放大镜仔细查看，但已经没有新的发现了。");
            }
        } else if (target->getType() == ItemType::Element) {
            std::cout << "你需要先获取这件物品才能仔细观察。\n";
        } else {
            COUTLN(Color::Hint, "放大镜在这里没有帮助。");
        }
        return;
    }

    std::cout << "你不知道如何用" << actualToolName << "来操作" << actualTargetName << "。\n";
    std::cout << "（提示：使用 '观察' 命令查看物品，使用 '拿' 拾取工具。）\n";
}

// 向铁门输入4位密码：正确密码4231
void Game::cmdEnter(const std::vector<std::string>& args) {
    if (args.empty()) {
        COUTLN(Color::Hint, "请输入密码。例如：'输入 4231'");
        return;
    }

    std::string input;
    for (const auto& arg : args) {
        input += arg;
    }

    if (input.length() != 4) {
        std::cout << "密码需要是4位数字。你输入的是：" << input << "\n";
        return;
    }

    for (char c : input) {
        if (!isdigit(static_cast<unsigned char>(c))) {
            std::cout << "密码只能包含数字。\n";
            return;
        }
    }

    if (input == "4231") {
        onCorrectPassword();
    } else {
        onWrongPassword();
    }
}

// 按压铁门元素符号：必须按地→水→火→风顺序
void Game::cmdPress(const std::vector<std::string>& args) {
    if (args.empty()) {
        COUTLN(Color::Hint, "请选择要按压的元素符号。可选项：地、水、火、风");
        return;
    }

    std::string elemStr = args[0];
    for (size_t i = 1; i < args.size(); ++i) {
        elemStr += " " + args[i];
    }

    if (!passwordSolved_) {
        COUTLN(Color::Error, "铁门纹丝不动。你似乎需要先输入正确的密码。");
        return;
    }

    ElementType elem = stringToElement(elemStr);
    if (elem == ElementType::None) {
        COUTLN(Color::Error, "没有这个元素符号。可选项：地、水、火、风");
        return;
    }

    auto it = std::find(pressedSymbols_.begin(), pressedSymbols_.end(), elem);
    if (it != pressedSymbols_.end()) {
        std::cout << getElementName(elem) << "符号已经被按下了。\n";
        return;
    }

    ElementType expectedOrder[] = {ElementType::Earth, ElementType::Water, ElementType::Fire, ElementType::Wind};
    size_t nextIndex = pressedSymbols_.size();
    if (nextIndex >= 4) {
        std::cout << "所有符号都已按下。\n";
        return;
    }

    ElementType expected = expectedOrder[nextIndex];
    if (elem != expected) {
        COUTLN(Color::Error, "元素符号发出刺耳的摩擦声，拒绝被按下。");
        std::cout << "（顺序不对，符号归位了。所有符号弹回原位。）\n";
        pressedSymbols_.clear();
        return;
    }

    pressedSymbols_.push_back(elem);
    std::cout << getElementSymbol(elem) << " 符号被按下，发出清脆的\"咔哒\"声。\n";

    if (pressedSymbols_.size() == 4) {
        phase_ = PuzzlePhase::Completed;
        state_ = GameState::Won;
    }
}

// 根据当前阶段输出引导提示
void Game::cmdHint() {
    COUTLN(Color::Hint, "\n【提示系统】");
    std::cout << "\n";

    if (phase_ == PuzzlePhase::Exploring) {
        bool hasAllTools = flags_["got_shovel"] && flags_["got_net"] && flags_["got_tongs"];
        if (!hasAllTools) {
            COUTLN(Color::Hint, "你需要先找到合适的工具。");
            COUTLN(Color::Mute, "书桌上散落着一些物品，抽屉里可能藏着铲子。");
            COUTLN(Color::Mute, "水族箱旁的墙上挂着渔网。");
            COUTLN(Color::Mute, "壁炉边有一把火钳。");
        } else {
            bool hasAllElems = player_.hasElement(ElementType::Earth) &&
                               player_.hasElement(ElementType::Water) &&
                               player_.hasElement(ElementType::Fire) &&
                               player_.hasElement(ElementType::Wind);
            if (!hasAllElems) {
                COUTLN(Color::Hint, "你需要收集四元素。");
                COUTLN(Color::Mute, "  - 盆栽的土壤下可能埋藏着什么（用小铲子挖）");
                COUTLN(Color::Mute, "  - 水族箱底的沙砾间或许有发现（用渔网捞）");
                COUTLN(Color::Mute, "  - 壁炉的灰烬中可能藏着东西（用火钳翻）");
                COUTLN(Color::Mute, "  - 书架上似乎有一本书颜色不对（观察或拿取）");
            }
        }
    }

    if (phase_ == PuzzlePhase::HaveAllElements) {
        bool allNums = player_.isNumberExtracted(ElementType::Earth) &&
                       player_.isNumberExtracted(ElementType::Water) &&
                       player_.isNumberExtracted(ElementType::Fire) &&
                       player_.isNumberExtracted(ElementType::Wind);
        if (!allNums) {
            COUTLN(Color::Hint, "每件元素物品上都隐藏着一个数字，不是直接写出的。");
            COUTLN(Color::Hint, "使用 '观察' 命令仔细查看每件物品的形态特征。");
            COUTLN(Color::Hint, "放大镜也许能帮你看清细小的刻痕。");
            COUTLN(Color::System, "提示：数字藏于物品的结构特征中（坑、纹、孔、刻痕）。");
        }
    }

    if (phase_ == PuzzlePhase::HaveAllNumbers) {
        if (!passwordSolved_) {
            COUTLN(Color::Hint, "你已经有了四个数字。现在需要确定它们的顺序。");
            COUTLN(Color::Hint, "回忆线索：");
            COUTLN(Color::Mute, "  - 羊皮纸背面画着 地→水→火→风 的循环图");
            COUTLN(Color::Mute, "  - 日记中提到四元素的自然秩序");
            std::cout << "\n";
            COUTLN(Color::Hint, "四个数字分别是：");
            {
                ColorScope cs(Color::Item);
                std::cout << "  地:" << player_.getExtractedNumber(ElementType::Earth)
                          << "  水:" << player_.getExtractedNumber(ElementType::Water)
                          << "  火:" << player_.getExtractedNumber(ElementType::Fire)
                          << "  风:" << player_.getExtractedNumber(ElementType::Wind) << "\n";
            }
            std::cout << "\n";
            COUTLN(Color::System, "按 地→水→火→风 顺序排列即可得到密码。");
            COUTLN(Color::System, "使用 '输入' 命令提交密码，例如：输入 4231");
        }
    }

    if (phase_ == PuzzlePhase::PasswordEntered) {
        COUTLN(Color::Hint, "密码已正确输入。现在需要按顺序按压铁门的四个元素符号。");
        COUTLN(Color::System, "顺序：地 → 水 → 火 → 风");
        COUTLN(Color::System, "使用 '按 地'、'按 水'、'按 火'、'按 风' 依次按压。");
    }

    if (phase_ == PuzzlePhase::Completed) {
        COUTLN(Color::System, "试炼已完成。走向铁门，迎接你的结局。");
    }
}

// 保存进度到 savegame.dat
void Game::cmdSave() {
    try {
        saveToFile();
        COUTLN(Color::System, "游戏已保存至 savegame.dat");
    } catch (const std::exception& e) {
        COUTLN(Color::Error, std::string("保存失败：") + e.what());
    }
}

// 从存档加载进度
void Game::cmdLoad() {
    try {
        loadFromFile();
        state_ = GameState::Playing;
        COUTLN(Color::System, "游戏进度已加载。");
        showSceneDescription();
    } catch (const std::exception& e) {
        COUTLN(Color::Error, std::string("加载失败：") + e.what());
    }
}

// 重新开始：清空进度并回到开场
void Game::cmdRestart() {
    COUT(Color::Hint, "确定要重新开始游戏吗？（当前进度将丢失）[y/n]: ");
    std::string confirm;
    std::getline(std::cin, confirm);
    if (confirm == "y" || confirm == "Y") {
        initWorld();
        initGameFlags();
        player_.reset();
        phase_ = PuzzlePhase::Exploring;
        passwordAttempts_ = 0;
        hintNoteDropped_ = false;
        passwordSolved_ = false;
        pressedSymbols_.clear();
        turnCount_ = 0;
        state_ = GameState::Playing;
        showIntro();
    } else {
        COUTLN(Color::System, "继续当前游戏。");
    }
}

void Game::cmdHelp() {
    std::cout << "\n";
    COUTLN(Color::Title, "══════════════════════════════════════════════");
    COUTLN(Color::Title, "           指令说明                          ");
    COUTLN(Color::Title, "══════════════════════════════════════════════");
    COUTLN(Color::Item, "   场景(scene)           - 查看当前场景描述                  ");
    COUTLN(Color::Item, "   观察(observe) [物品]   - 查看物品详细信息             ");
    COUTLN(Color::Item, "   拿(take) [物品]        - 拾取物品                    ");
    COUTLN(Color::Item, "   用(use) [工具] [目标]   - 使用工具操作目标        ");
    COUTLN(Color::Item, "   输入(enter) [数字]      - 向铁门输入密码               ");
    COUTLN(Color::Item, "   按(press) [元素]       - 按压铁门的元素符号           ");
    COUTLN(Color::Item, "   提示(hint)             - 获取当前谜题的引导           ");
    COUTLN(Color::Item, "   背包(inventory)        - 查看物品栏                  ");
    COUTLN(Color::Item, "   保存(save)             - 保存游戏进度                 ");
    COUTLN(Color::Item, "   加载(load)             - 读取游戏进度                 ");
    COUTLN(Color::Item, "   重来(restart)          - 重新开始游戏                 ");
    COUTLN(Color::Item, "   帮助(help)             - 显示此帮助信息               ");
    COUTLN(Color::Title, "══════════════════════════════════════════════");
    std::cout << "\n";
    COUTLN(Color::Hint, "与场景物品进行交互时中间请用【空格】隔断");
}

void Game::cmdLook() {
    showSceneDescription();
}

void Game::cmdInventory() {
    COUTLN(Color::Title, "\n=== 背包 ===");
    if (player_.getInventory().empty()) {
        COUTLN(Color::Mute, "（空）");
    } else {
        std::for_each(player_.getInventory().begin(), player_.getInventory().end(),
            [](const Item& item) {
                COUTLN(Color::Item, "  - " + item.getName());
            });
    }
    std::cout << "\n";
    COUT(Color::System, "已收集的元素：");
    if (player_.getCollectedElements().empty()) {
        COUTLN(Color::Mute, "（无）");
    } else {
        std::for_each(player_.getCollectedElements().begin(), player_.getCollectedElements().end(),
            [this](const std::pair<const ElementType, Item>& p) {
                COUT(Color::Item, "【" + getElementName(p.first) + "】 ");
            });
        std::cout << "\n";
    }
}

// 精确名匹配失败时退化为子串匹配
Item* Game::findWorldItem(const std::string& name) {
    auto it = worldItems_.find(name);
    if (it != worldItems_.end()) return &it->second;

    for (auto& pair : worldItems_) {
        if (pair.first.find(name) != std::string::npos) {
            return &pair.second;
        }
    }
    return nullptr;
}

const Item* Game::findWorldItem(const std::string& name) const {
    auto it = worldItems_.find(name);
    if (it != worldItems_.end()) return &it->second;

    for (const auto& pair : worldItems_) {
        if (pair.first.find(name) != std::string::npos) {
            return &pair.second;
        }
    }
    return nullptr;
}

// 观察元素物品：标记已观察并记录隐藏数字
void Game::extractNumberFromItem(Item& item) {
    item.setExamined(true);
    if (item.getType() != ItemType::Element) return;

    int num = item.getHiddenNumber();
    ElementType elem = item.getElementType();
    player_.extractNumber(elem, num);

    COUTLN(Color::Item, "\n【数字发现】");
    COUT(Color::Narration, "你仔细观察" + item.getName() + "，");
    COUTLN(Color::Narration, "注意到" + item.getNumberClue());
    COUTLN(Color::System, "由此你推断出，这件物品隐藏的数字是：" + std::to_string(num));
    std::cout << "\n";

    checkAllNumbersExtracted();
}

// 检查四元素数字是否已全部提取，若是则推进阶段
void Game::checkAllNumbersExtracted() {
    ElementType elements[] = {ElementType::Earth, ElementType::Water, ElementType::Fire, ElementType::Wind};
    bool allExtracted = std::all_of(std::begin(elements), std::end(elements),
        [this](ElementType elem) { return player_.isNumberExtracted(elem); });

    if (allExtracted) {
        phase_ = PuzzlePhase::HaveAllNumbers;
        flags_["numbers_extracted"] = true;
        COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        COUTLN(Color::System, "  你已提取所有元素的数字！");
        {
            ColorScope cs(Color::Item);
            std::cout << "  地:" << player_.getExtractedNumber(ElementType::Earth)
                      << "  水:" << player_.getExtractedNumber(ElementType::Water)
                      << "  火:" << player_.getExtractedNumber(ElementType::Fire)
                      << "  风:" << player_.getExtractedNumber(ElementType::Wind) << "\n";
        }
        COUTLN(Color::Hint, "  现在需要确定它们的排列顺序...");
        COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        std::cout << "\n";
    }
}

// 检查四元素是否集齐，若是则推进到HaveAllElements阶段
void Game::checkAllElementsCollected() {
    ElementType elements[] = {ElementType::Earth, ElementType::Water, ElementType::Fire, ElementType::Wind};
    bool allCollected = std::all_of(std::begin(elements), std::end(elements),
        [this](ElementType elem) { return player_.hasElement(elem); });

    if (allCollected && !flags_["elements_gathered"]) {
        phase_ = PuzzlePhase::HaveAllElements;
        flags_["elements_gathered"] = true;
        COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        COUTLN(Color::System, "  四元素已全部收集！");
        COUTLN(Color::Item, "  【地】【水】【火】【风】");
        COUTLN(Color::Hint, "  现在仔细观察每件物品，提取隐藏的数字。");
        COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        std::cout << "\n";
    }
}

// 密码正确：进入按压符号阶段
void Game::onCorrectPassword() {
    passwordSolved_ = true;
    phase_ = PuzzlePhase::PasswordEntered;
    flags_["password_solved"] = true;
    std::cout << "\n";
    COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    COUTLN(Color::System, "  密码正确！");
    COUTLN(Color::Narration, "  铁门内传来机括转动的声响，");
    COUTLN(Color::Item, "  四个元素符号依次亮起！");
    COUTLN(Color::Hint, "  现在按正确顺序依次按压四个元素符号：");
    COUTLN(Color::System, "  地 → 水 → 火 → 风");
    COUTLN(Color::Title, "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    std::cout << "\n";
}

// 密码错误：累计3次则掉落线索纸条并自动填数字防卡死
void Game::onWrongPassword() {
    passwordAttempts_++;
    COUTLN(Color::Error, "铁门发出沉闷的拒绝声，密码错误。");
    COUTLN(Color::Mute, "（已尝试 " + std::to_string(passwordAttempts_) + " 次）");

    if (passwordAttempts_ >= 3 && !hintNoteDropped_) {
        hintNoteDropped_ = true;
        flags_["hint_dropped"] = true;

        Item note("线索纸条", "一张从书桌抽屉弹出的纸条。", ItemType::Clue);
        note.setLocation("地上");
        note.setDetailedDesc("纸条上写着：\n\n"
            "\"地有四边，水有双流，火生三焰，风唯一向。\"\n\n"
            "这似乎是在暗示每件元素物品上的数字。\n"
            "（地=4, 水=2, 火=3, 风=1）");
        note.setCollected(true);

        worldItems_["线索纸条"] = note;
        worldItemOrder_.push_back("线索纸条");

        COUTLN(Color::System, "\n【防卡死提示】");
        COUTLN(Color::Narration, "书桌的抽屉\"啪\"地弹开，一张纸条飘落地上！");
        std::cout << "\n";
        COUTLN(Color::Hint, "纸条上写着：");
        COUTLN(Color::Dialogue, "  \"地有四边，水有双流，火生三焰，风唯一向。\"");
        COUTLN(Color::Item, "  （地=4, 水=2, 火=3, 风=1）");
        std::cout << "\n";

        if (!player_.isNumberExtracted(ElementType::Earth)) {
            player_.extractNumber(ElementType::Earth, 4);
            if (worldItems_.find("地石板") != worldItems_.end())
                worldItems_["地石板"].setExamined(true);
        }
        if (!player_.isNumberExtracted(ElementType::Water)) {
            player_.extractNumber(ElementType::Water, 2);
            if (worldItems_.find("水贝壳") != worldItems_.end())
                worldItems_["水贝壳"].setExamined(true);
        }
        if (!player_.isNumberExtracted(ElementType::Fire)) {
            player_.extractNumber(ElementType::Fire, 3);
            if (worldItems_.find("火铁片") != worldItems_.end())
                worldItems_["火铁片"].setExamined(true);
        }
        if (!player_.isNumberExtracted(ElementType::Wind)) {
            player_.extractNumber(ElementType::Wind, 1);
            if (worldItems_.find("风羽毛") != worldItems_.end())
                worldItems_["风羽毛"].setExamined(true);
        }

        checkAllNumbersExtracted();
    }
}

std::string Game::getElementName(ElementType elem) const {
    switch (elem) {
        case ElementType::Earth: return "地";
        case ElementType::Water: return "水";
        case ElementType::Fire: return "火";
        case ElementType::Wind: return "风";
        default: return "";
    }
}

std::string Game::getElementSymbol(ElementType elem) const {
    switch (elem) {
        case ElementType::Earth: return "⛰";
        case ElementType::Water: return "💧";
        case ElementType::Fire: return "🔥";
        case ElementType::Wind: return "🌬";
        default: return "?";
    }
}

ElementType Game::stringToElement(const std::string& str) const {
    if (str.find("地") != std::string::npos || str.find("土") != std::string::npos) return ElementType::Earth;
    if (str.find("水") != std::string::npos) return ElementType::Water;
    if (str.find("火") != std::string::npos) return ElementType::Fire;
    if (str.find("风") != std::string::npos) return ElementType::Wind;

    std::string lower = toLower(str);
    if (lower == "earth" || lower == "e") return ElementType::Earth;
    if (lower == "water" || lower == "w") return ElementType::Water;
    if (lower == "fire" || lower == "f") return ElementType::Fire;
    if (lower == "wind") return ElementType::Wind;

    return ElementType::None;
}

std::string Game::toLower(const std::string& str) const {
    std::string result = str;
    for (size_t i = 0; i < result.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(result[i]);
        if (c < 0x80) {
            result[i] = static_cast<char>(std::tolower(c));
        } else {
            unsigned char first = c;
            int skip = 0;
            if ((first & 0xE0) == 0xC0) skip = 1;
            else if ((first & 0xF0) == 0xE0) skip = 2;
            else if ((first & 0xF8) == 0xF0) skip = 3;
            i += skip;
        }
    }
    return result;
}

// 二进制保存：背包、元素、数字、阶段、标志、按压序列
void Game::saveToFile(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开保存文件");
    }

    auto writeString = [&file](const std::string& s) {
        size_t len = s.size();
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(s.data(), len);
    };

    auto writeBool = [&file](bool v) {
        file.write(reinterpret_cast<const char*>(&v), sizeof(v));
    };

    auto writeInt = [&file](int v) {
        file.write(reinterpret_cast<const char*>(&v), sizeof(v));
    };

    size_t invSize = player_.getInventory().size();
    file.write(reinterpret_cast<const char*>(&invSize), sizeof(invSize));
    for (const auto& item : player_.getInventory()) {
        writeString(item.getName());
        writeString(item.getDescription());
        writeString(item.getDetailedDesc());
        writeInt(static_cast<int>(item.getType()));
        writeInt(static_cast<int>(item.getElementType()));
        writeInt(item.getHiddenNumber());
        writeBool(item.isCollected());
        writeBool(item.isExamined());
    }

    ElementType elements[] = {ElementType::Earth, ElementType::Water, ElementType::Fire, ElementType::Wind};
    for (auto elem : elements) {
        bool has = player_.hasElement(elem);
        writeBool(has);
        bool extracted = player_.isNumberExtracted(elem);
        writeBool(extracted);
        int num = player_.getExtractedNumber(elem);
        writeInt(num);
    }

    writeInt(static_cast<int>(phase_));
    writeInt(passwordAttempts_);
    writeBool(hintNoteDropped_);
    writeBool(passwordSolved_);
    writeInt(turnCount_);
    writeBool(flags_["got_shovel"]);
    writeBool(flags_["got_net"]);
    writeBool(flags_["got_tongs"]);
    writeBool(flags_["got_magnifier"]);
    writeBool(flags_["got_diary"]);
    writeBool(flags_["examined_diary"]);
    writeBool(flags_["elements_gathered"]);
    writeBool(flags_["numbers_extracted"]);
    writeBool(flags_["password_solved"]);
    writeBool(flags_["hint_dropped"]);
    writeBool(flags_["wind_collected"]);

    size_t pressedSize = pressedSymbols_.size();
    file.write(reinterpret_cast<const char*>(&pressedSize), sizeof(pressedSize));
    for (auto elem : pressedSymbols_) {
        writeInt(static_cast<int>(elem));
    }

    file.close();
}

// 二进制读取：重建世界后再恢复背包/元素/阶段/标志
void Game::loadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开存档文件");
    }

    initWorld();
    initGameFlags();
    player_.reset();

    auto readString = [&file]() -> std::string {
        size_t len;
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string s(len, '\0');
        if (len > 0) file.read(&s[0], len);
        return s;
    };

    auto readBool = [&file]() -> bool {
        bool v;
        file.read(reinterpret_cast<char*>(&v), sizeof(v));
        return v;
    };

    auto readInt = [&file]() -> int {
        int v;
        file.read(reinterpret_cast<char*>(&v), sizeof(v));
        return v;
    };

    size_t invSize;
    file.read(reinterpret_cast<char*>(&invSize), sizeof(invSize));
    for (size_t i = 0; i < invSize; ++i) {
        std::string name = readString();
        std::string desc = readString();
        std::string detailed = readString();
        int typeInt = readInt();
        int elemInt = readInt();
        int hiddenNum = readInt();
        bool collected = readBool();
        bool examined = readBool();

        Item item(name, desc, static_cast<ItemType>(typeInt));
        item.setDetailedDesc(detailed);
        item.setElementType(static_cast<ElementType>(elemInt));
        item.setHiddenNumber(hiddenNum);
        item.setCollected(collected);
        item.setExamined(examined);

        player_.addItem(item);

        if (item.getType() == ItemType::Tool || item.getName() == "无字书" || item.getName() == "放大镜" ||
            item.getName() == "日记本") {
            worldItems_.erase(item.getName());
            worldItemOrder_.erase(
                std::remove(worldItemOrder_.begin(), worldItemOrder_.end(), item.getName()),
                worldItemOrder_.end());
        } else if (collected) {
            auto wit = worldItems_.find(item.getName());
            if (wit != worldItems_.end()) {
                wit->second.setCollected(true);
                wit->second.setExamined(examined);
            }
        }
    }

    ElementType elements[] = {ElementType::Earth, ElementType::Water, ElementType::Fire, ElementType::Wind};
    for (auto elem : elements) {
        bool has = readBool();
        bool extracted = readBool();
        int num = readInt();
        if (has) player_.collectElement(elem);
        if (extracted) player_.extractNumber(elem, num);
    }

    phase_ = static_cast<PuzzlePhase>(readInt());
    passwordAttempts_ = readInt();
    hintNoteDropped_ = readBool();
    passwordSolved_ = readBool();
    turnCount_ = readInt();
    flags_["got_shovel"] = readBool();
    flags_["got_net"] = readBool();
    flags_["got_tongs"] = readBool();
    flags_["got_magnifier"] = readBool();
    flags_["got_diary"] = readBool();
    flags_["examined_diary"] = readBool();
    flags_["elements_gathered"] = readBool();
    flags_["numbers_extracted"] = readBool();
    flags_["password_solved"] = readBool();
    flags_["hint_dropped"] = readBool();
    flags_["wind_collected"] = readBool();

    size_t pressedSize;
    file.read(reinterpret_cast<char*>(&pressedSize), sizeof(pressedSize));
    pressedSymbols_.clear();
    for (size_t i = 0; i < pressedSize; ++i) {
        int elemInt = readInt();
        pressedSymbols_.push_back(static_cast<ElementType>(elemInt));
    }

    if (flags_["got_parchment"]) {
        auto pit = worldItems_.find("羊皮纸");
        if (pit != worldItems_.end()) {
            pit->second.setCollected(true);
        }
    }

    if (hintNoteDropped_) {
        Item note("线索纸条", "一张从书桌抽屉弹出的纸条。", ItemType::Clue);
        note.setLocation("地上");
        note.setDetailedDesc("纸条上写着：\n\"地有四边，水有双流，火生三焰，风唯一向。\"");
        note.setCollected(true);
        worldItems_["线索纸条"] = note;
        if (std::find(worldItemOrder_.begin(), worldItemOrder_.end(), "线索纸条") == worldItemOrder_.end()) {
            worldItemOrder_.push_back("线索纸条");
        }
    }

    file.close();
}

// 轮询返回环境氛围文本（每5回合触发一次）
std::string Game::getAmbientMessage() const {
    if (ambientMessages_.empty()) return "";
    std::string msg = ambientMessages_[ambientIndex_ % ambientMessages_.size()];
    ambientIndex_++;
    return msg;
}