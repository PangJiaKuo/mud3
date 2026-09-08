// Player类实现：玩家背包与元素收集状态
#include "Player.h"

Player::Player() {}

void Player::addItem(const Item& item) {
    inventory_.push_back(item);
}

// erase-remove 惯用法删除所有同名物品
void Player::removeItem(const std::string& itemName) {
    inventory_.erase(
        std::remove_if(inventory_.begin(), inventory_.end(),
            [&itemName](const Item& item) { return item.getName() == itemName; }),
        inventory_.end());
}

bool Player::hasItem(const std::string& itemName) const {
    return std::any_of(inventory_.begin(), inventory_.end(),
        [&itemName](const Item& item) { return item.getName() == itemName; });
}

// 按名称查找背包物品，未找到返回nullptr
Item* Player::findItem(const std::string& itemName) {
    auto it = std::find_if(inventory_.begin(), inventory_.end(),
        [&itemName](const Item& item) { return item.getName() == itemName; });
    return (it != inventory_.end()) ? &(*it) : nullptr;
}

const Item* Player::findItem(const std::string& itemName) const {
    auto it = std::find_if(inventory_.begin(), inventory_.end(),
        [&itemName](const Item& item) { return item.getName() == itemName; });
    return (it != inventory_.end()) ? &(*it) : nullptr;
}

const std::vector<Item>& Player::getInventory() const {
    return inventory_;
}

// 收集元素（去重，仅记录类型）
void Player::collectElement(ElementType elem) {
    if (collectedElements_.find(elem) == collectedElements_.end()) {
        Item temp;
        temp.setElementType(elem);
        collectedElements_[elem] = temp;
    }
}

bool Player::hasElement(ElementType elem) const {
    return collectedElements_.find(elem) != collectedElements_.end();
}

const std::map<ElementType, Item>& Player::getCollectedElements() const {
    return collectedElements_;
}

// 记录元素对应的隐藏数字
void Player::extractNumber(ElementType elem, int num) {
    extractedNumbers_[elem] = num;
}

// 未提取返回-1
int Player::getExtractedNumber(ElementType elem) const {
    auto it = extractedNumbers_.find(elem);
    return (it != extractedNumbers_.end()) ? it->second : -1;
}

bool Player::isNumberExtracted(ElementType elem) const {
    return extractedNumbers_.find(elem) != extractedNumbers_.end();
}

// 清空所有进度（重新开始时调用）
void Player::reset() {
    inventory_.clear();
    collectedElements_.clear();
    extractedNumbers_.clear();
}
