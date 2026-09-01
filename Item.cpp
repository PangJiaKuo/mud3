#include "Item.h"

Item::Item()
    : name_(""), description_(""), detailedDesc_(""),
      type_(ItemType::Misc), elementType_(ElementType::None),
      hiddenNumber_(0), numberClue_(""), hiddenText_(""), hint_(""),
      collected_(false), examined_(false), used_(false), location_("") {}

Item::Item(const std::string& name, const std::string& description, ItemType type)
    : name_(name), description_(description), detailedDesc_(""),
      type_(type), elementType_(ElementType::None),
      hiddenNumber_(0), numberClue_(""), hiddenText_(""), hint_(""),
      collected_(false), examined_(false), used_(false), location_("") {}

const std::string& Item::getName() const { return name_; }
void Item::setName(const std::string& name) { name_ = name; }

const std::string& Item::getDescription() const { return description_; }
void Item::setDescription(const std::string& desc) { description_ = desc; }

const std::string& Item::getDetailedDesc() const { return detailedDesc_; }
void Item::setDetailedDesc(const std::string& desc) { detailedDesc_ = desc; }

ItemType Item::getType() const { return type_; }
void Item::setType(ItemType type) { type_ = type; }

ElementType Item::getElementType() const { return elementType_; }
void Item::setElementType(ElementType elem) { elementType_ = elem; }

int Item::getHiddenNumber() const { return hiddenNumber_; }
void Item::setHiddenNumber(int num) { hiddenNumber_ = num; }

const std::string& Item::getNumberClue() const { return numberClue_; }
void Item::setNumberClue(const std::string& clue) { numberClue_ = clue; }

const std::string& Item::getHiddenText() const { return hiddenText_; }
void Item::setHiddenText(const std::string& text) { hiddenText_ = text; }

const std::string& Item::getHint() const { return hint_; }
void Item::setHint(const std::string& hint) { hint_ = hint; }

bool Item::isCollected() const { return collected_; }
void Item::setCollected(bool v) { collected_ = v; }

bool Item::isExamined() const { return examined_; }
void Item::setExamined(bool v) { examined_ = v; }

bool Item::isUsed() const { return used_; }
void Item::setUsed(bool v) { used_ = v; }

const std::string& Item::getLocation() const { return location_; }
void Item::setLocation(const std::string& loc) { location_ = loc; }

const std::map<std::string, std::string>& Item::getInteractions() const {
    return interactions_;
}

void Item::addInteraction(const std::string& tool, const std::string& result) {
    interactions_[tool] = result;
}

bool Item::operator==(const Item& other) const {
    return name_ == other.name_;
}

bool Item::operator<(const Item& other) const {
    return name_ < other.name_;
}
