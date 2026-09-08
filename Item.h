// Item：场景中的物品/工具/元素/线索
// 每个物品有名称、描述、类型、位置，元素物品还隐藏了数字线索
#ifndef ITEM_H
#define ITEM_H

#include <string>
#include <map>
#include <vector>
#include <iostream>

// 物品类型 —— 决定交互方式
enum class ItemType {
    Tool,     // 工具：铲子、渔网、火钳、放大镜（可拾取、可使用）
    Element,  // 元素：地水火风（需工具获取，观察后提取数字）
    Clue,     // 线索：日记、羊皮纸、无字书（提供剧情/提示）
    Key,      // 关键道具：铁门（最终机关）
    Misc      // 杂物：书桌、书架、壁炉等场景装饰（不可拾取）
};

// 四元素枚举
enum class ElementType {
    None,
    Earth,  // 地
    Water,  // 水
    Fire,   // 火
    Wind    // 风
};

class Item {
public:
    Item();
    Item(const std::string& name, const std::string& description,
         ItemType type = ItemType::Misc);

    // 基础属性
    const std::string& getName() const;
    void setName(const std::string& name);

    const std::string& getDescription() const;       // 简短描述
    void setDescription(const std::string& desc);

    const std::string& getDetailedDesc() const;      // 观察时显示的详细描述
    void setDetailedDesc(const std::string& desc);

    ItemType getType() const;
    void setType(ItemType type);

    // 元素相关（仅 Element 类型有意义）
    ElementType getElementType() const;
    void setElementType(ElementType elem);

    int getHiddenNumber() const;                     // 隐藏数字（4/2/3/1）
    void setHiddenNumber(int num);

    const std::string& getNumberClue() const;        // 数字的线索文本
    void setNumberClue(const std::string& clue);

    // 提示与文本
    const std::string& getHiddenText() const;
    void setHiddenText(const std::string& text);

    const std::string& getHint() const;              // 谜题提示
    void setHint(const std::string& hint);

    // 状态标记
    bool isCollected() const;                        // 是否已被拾取
    void setCollected(bool v);

    bool isExamined() const;                         // 是否已观察并提取数字（防重复提取）
    void setExamined(bool v);

    bool isUsed() const;                             // 是否已被使用（如盆栽被挖过）
    void setUsed(bool v);

    // 位置
    const std::string& getLocation() const;          // 物品在场景中的位置
    void setLocation(const std::string& loc);

    // 交互映射（工具→结果）
    const std::map<std::string, std::string>& getInteractions() const;
    void addInteraction(const std::string& tool, const std::string& result);

    bool operator==(const Item& other) const;        // 按名称判等
    bool operator<(const Item& other) const;         // 按名称排序

private:
    std::string name_;
    std::string description_;
    std::string detailedDesc_;
    ItemType type_;
    ElementType elementType_;
    int hiddenNumber_;
    std::string numberClue_;
    std::string hiddenText_;
    std::string hint_;
    bool collected_;
    bool examined_;
    bool used_;
    std::string location_;
    std::map<std::string, std::string> interactions_;
};

#endif