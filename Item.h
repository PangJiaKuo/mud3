#ifndef ITEM_H
#define ITEM_H

#include <string>
#include <map>
#include <vector>
#include <iostream>

// 物品类型
enum class ItemType {
    Tool,     // 工具：铲子、渔网等
    Element,  // 元素：地水火风
    Clue,     // 线索：日记、羊皮纸等
    Key,      // 关键道具：铁门
    Misc      // 杂物：场景装饰
};

// 元素类型
enum class ElementType {
    None,
    Earth,
    Water,
    Fire,
    Wind
};

class Item {
public:
    Item();
    Item(const std::string& name, const std::string& description,
         ItemType type = ItemType::Misc);

    const std::string& getName() const;
    void setName(const std::string& name);

    const std::string& getDescription() const;
    void setDescription(const std::string& desc);

    const std::string& getDetailedDesc() const;   // 观察时显示的详细描述
    void setDetailedDesc(const std::string& desc);

    ItemType getType() const;
    void setType(ItemType type);

    ElementType getElementType() const;
    void setElementType(ElementType elem);

    int getHiddenNumber() const;                  // 元素隐藏的数字
    void setHiddenNumber(int num);

    const std::string& getNumberClue() const;    // 数字线索描述
    void setNumberClue(const std::string& clue);

    const std::string& getHiddenText() const;
    void setHiddenText(const std::string& text);

    const std::string& getHint() const;
    void setHint(const std::string& hint);

    bool isCollected() const;
    void setCollected(bool v);

    bool isExamined() const;                      // 是否已观察并提取数字
    void setExamined(bool v);

    bool isUsed() const;
    void setUsed(bool v);

    const std::string& getLocation() const;
    void setLocation(const std::string& loc);

    const std::map<std::string, std::string>& getInteractions() const;
    void addInteraction(const std::string& tool, const std::string& result);

    bool operator==(const Item& other) const;     // 按名称比较
    bool operator<(const Item& other) const;

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
