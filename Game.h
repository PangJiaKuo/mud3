#ifndef GAME_H
#define GAME_H

#include "Item.h"
#include "Player.h"
#include <string>
#include <map>
#include <vector>

enum class GameState {
    Menu,
    Playing,
    GameOver,
    Won
};

enum class PuzzlePhase {
    Exploring,
    HaveAllElements,
    HaveAllNumbers,
    PasswordEntered,
    Completed
};

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    void initWorld();
    void initGameFlags();

    void showIntro();
    void showMainMenu();
    void showSceneDescription();
    void showEnding();
    void playVideo(const std::string& filename, const std::string& skipMessage);
    void processCommand(const std::string& input);
    std::vector<std::string> tokenize(const std::string& input);

    void cmdObserve(const std::vector<std::string>& args);
    void cmdTake(const std::vector<std::string>& args);
    void cmdUse(const std::vector<std::string>& args);
    void cmdEnter(const std::vector<std::string>& args);
    void cmdPress(const std::vector<std::string>& args);
    void cmdHint();
    void cmdSave();
    void cmdLoad();
    void cmdRestart();
    void cmdHelp();
    void cmdLook();
    void cmdInventory();

    Item* findWorldItem(const std::string& name);
    const Item* findWorldItem(const std::string& name) const;

    void extractNumberFromItem(Item& item);
    void checkAllNumbersExtracted();
    void checkAllElementsCollected();
    void onCorrectPassword();
    void onWrongPassword();

    std::string getElementName(ElementType elem) const;
    std::string getElementSymbol(ElementType elem) const;
    ElementType stringToElement(const std::string& str) const;
    std::string toLower(const std::string& str) const;

    void saveToFile(const std::string& filename = "savegame.dat");
    void loadFromFile(const std::string& filename = "savegame.dat");

    std::string getAmbientMessage() const;

    std::map<std::string, Item> worldItems_;
    std::vector<std::string> worldItemOrder_;
    Player player_;

    GameState state_;
    PuzzlePhase phase_;

    int passwordAttempts_;
    bool hintNoteDropped_;
    bool passwordSolved_;
    std::vector<ElementType> pressedSymbols_;

    std::map<std::string, bool> flags_;

    int turnCount_;

    std::vector<std::string> ambientMessages_;
    mutable int ambientIndex_;
};

#endif
