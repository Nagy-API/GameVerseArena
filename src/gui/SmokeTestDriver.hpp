#pragma once

#include "SceneManager.hpp"
#include "TurnBasedGame.hpp"

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

class Application;

// Developer verification harness (GameVerseArenaGUI --database <tmp> --smoke-test <dir>).
// It drives the real scenes through Application::dispatch with synthetic SFML events,
// advances fixed 1/60 s frames, checks scene transitions and persisted results, and saves
// PNG captures at 1280x720 and the 960x540 minimum. Real input from the desktop is ignored
// while it runs, and it never touches the production database.
class SmokeTestDriver {
public:
    SmokeTestDriver(Application& application, std::filesystem::path outputDirectory);
    int run();

private:
    void key(sf::Keyboard::Key key, bool shift = false);
    void keyWithText(sf::Keyboard::Key key, char32_t character);
    void type(const std::string& text);
    void move(sf::Vector2f logical);
    void click(sf::Vector2f logical);
    void focusLost();
    void focusGained();
    void frames(int count);
    void wait(float seconds);
    void capture(const std::string& name);
    void captureBoth(const std::string& name);
    void setWindowSize(sf::Vector2u size);
    bool expect(bool condition, const std::string& message);
    bool expectScene(SceneId id, const std::string& message);
    SceneId activeScene() const;
    std::int64_t historyCount();
    void goToMainMenu();
    void openLibrary();
    void clickLibraryCard(std::size_t visibleIndex);

    void scenarioMainMenu();
    void scenarioSettings();
    void scenarioAbout();
    void scenarioProfiles();
    void scenarioLibrary();
    void scenarioTicTacToe();
    void scenarioPingPong();
    void scenarioBoardGames();
    void scenarioMoreBoardGames();

    // Shared board games: open one from the library search, choose the mode and side, and start.
    bool startBoardGame(const std::string& search, bool againstComputer, bool humanFirst,
                        const std::string& setupCapture = {});
    // Plays `move` through the board view: a click on its cell (after its number key, for
    // Numerical), or the arrow keys and Enter when `keyboard` is set.
    void playBoardMove(turn_based::MoveId move, bool keyboard);
    // Plays the first legal move for every human turn and waits for the computer, until the game ends.
    bool playBoardGameToEnd(bool keyboard);
    // Waits (in real time, while frames advance) until it is no longer the computer's turn.
    bool waitForComputerMove();

    void pumpRealEvents();

    Application& app_;
    std::filesystem::path output_;
    std::ofstream log_;
    int checks_{};
    int failures_{};
    int captures_{};
};
