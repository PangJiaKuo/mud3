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

    void addItem(const Item& item);
    void removeItem(const std::string& itemName);
    bool hasItem(const std::string& itemName) const;
    Item* findItem(const std::string& itemName);
    const Item* findItem(const std::string& itemName) const;

    const std::vector<Item>& getInventory() const;

    void collectElement(ElementType elem);
    bool hasElement(ElementType elem) const;
    const std::map<ElementType, Item>& getCollectedElements() const;

    void extractNumber(ElementType elem, int num);
    int getExtractedNumber(ElementType elem) const;
    bool isNumberExtracted(ElementType elem) const;

    void reset();

private:
    std::vector<Item> inventory_;
    std::map<ElementType, Item> collectedElements_;
    std::map<ElementType, int> extractedNumbers_;
};

#endif
