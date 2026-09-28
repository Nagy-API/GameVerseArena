#pragma once

#include "SceneManager.hpp"

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

    void scenarioMainMenu();
    void scenarioSettings();
    void scenarioAbout();
    void scenarioProfiles();
    void scenarioTicTacToe();
    void scenarioPingPong();

    void pumpRealEvents();

    Application& app_;
    std::filesystem::path output_;
    std::ofstream log_;
    int checks_{};
    int failures_{};
    int captures_{};
};
