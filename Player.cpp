#include "Player.h"

Player::Player() {}

void Player::addItem(const Item& item) {
    inventory_.push_back(item);
}

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

void Player::extractNumber(ElementType elem, int num) {
    extractedNumbers_[elem] = num;
}

int Player::getExtractedNumber(ElementType elem) const {
    auto it = extractedNumbers_.find(elem);
    return (it != extractedNumbers_.end()) ? it->second : -1;
}

bool Player::isNumberExtracted(ElementType elem) const {
    return extractedNumbers_.find(elem) != extractedNumbers_.end();
}

void Player::reset() {
    inventory_.clear();
    collectedElements_.clear();
    extractedNumbers_.clear();
}
