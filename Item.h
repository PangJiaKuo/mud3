#ifndef ITEM_H
#define ITEM_H

#include <string>
#include <map>
#include <vector>
#include <iostream>

enum class ItemType {
    Tool,
    Element,
    Clue,
    Key,
    Misc
};

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

    const std::string& getDetailedDesc() const;
    void setDetailedDesc(const std::string& desc);

    ItemType getType() const;
    void setType(ItemType type);

    ElementType getElementType() const;
    void setElementType(ElementType elem);

    int getHiddenNumber() const;
    void setHiddenNumber(int num);

    const std::string& getNumberClue() const;
    void setNumberClue(const std::string& clue);

    const std::string& getHiddenText() const;
    void setHiddenText(const std::string& text);

    const std::string& getHint() const;
    void setHint(const std::string& hint);

    bool isCollected() const;
    void setCollected(bool v);

    bool isExamined() const;
    void setExamined(bool v);

    bool isUsed() const;
    void setUsed(bool v);

    const std::string& getLocation() const;
    void setLocation(const std::string& loc);

    const std::map<std::string, std::string>& getInteractions() const;
    void addInteraction(const std::string& tool, const std::string& result);

    bool operator==(const Item& other) const;
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
