// Player：玩家状态 —— 背包、元素收集、数字提取
// 不涉及位置/移动，只有物品管理和解谜进度
#ifndef PLAYER_H
#define PLAYER_H

#include "Item.h"
#include <vector>
#include <map>
#include <string>
#include <algorithm>

class Player {
public:
    Player();

    // ── 背包管理 ──
    void addItem(const Item& item);
    void removeItem(const std::string& itemName);
    bool hasItem(const std::string& itemName) const;
    Item* findItem(const std::string& itemName);
    const Item* findItem(const std::string& itemName) const;
    const std::vector<Item>& getInventory() const;

    // ── 元素收集（去重，仅记录类型） ──
    void collectElement(ElementType elem);
    bool hasElement(ElementType elem) const;
    const std::map<ElementType, Item>& getCollectedElements() const;

    // ── 隐藏数字提取（一个元素对应一个数字） ──
    void extractNumber(ElementType elem, int num);
    int getExtractedNumber(ElementType elem) const;   // 未提取返回 -1
    bool isNumberExtracted(ElementType elem) const;

    // 重置所有状态（重新开始游戏时调用）
    void reset();

private:
    std::vector<Item> inventory_;                    // 背包物品列表
    std::map<ElementType, Item> collectedElements_;  // 已收集的元素（按类型索引）
    std::map<ElementType, int> extractedNumbers_;    // 已提取的数字（按元素类型索引）
};

#endif