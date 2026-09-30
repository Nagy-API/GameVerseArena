#pragma once

#include "AchievementNotificationQueue.hpp"
#include "AchievementService.hpp"
#include "AchievementToast.hpp"
#include "AppContext.hpp"
#include "AssetManager.hpp"
#include "AudioEngine.hpp"
#include "Database.hpp"
#include "GameLauncher.hpp"
#include "MatchRecorder.hpp"
#include "MatchRepository.hpp"
#include "MatchService.hpp"
#include "PingPongSession.hpp"
#include "ProfileService.hpp"
#include "SceneManager.hpp"
#include "SettingsController.hpp"
#include "SettingsService.hpp"
#include "StatisticsRepository.hpp"
#include "TicTacToeSession.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

struct ApplicationOptions {
    std::optional<std::filesystem::path> databasePath;
    bool silentAudio{false};
    // Developer verification mode: drives the real scenes with synthetic input, writes PNG
    // captures and a log to this directory, then exits (0 = every expectation held).
    std::optional<std::filesystem::path> smokeTestDirectory;
};

class SmokeTestDriver;

class Application {
public:
    Application(std::filesystem::path executableDirectory, ApplicationOptions options);
    int run();

private:
    friend class SmokeTestDriver;

    bool initialize();
    void dispatch(const sf::Event& event);
    void advanceFrame(sf::Time deltaTime, const std::filesystem::path* capturePath = nullptr);
    void updateView(sf::Vector2u windowSize);
    int runLoop();
    void reportFatal(const std::string& title, const std::string& message) const;

    std::filesystem::path executableDirectory_;
    ApplicationOptions options_;
    sf::RenderWindow window_;
    AssetManager assets_;
    std::unique_ptr<AudioEngine> audio_;
    classic_ttt::TicTacToeSession ticTacToeSession_;
    ping_pong::PingPongSession pingPongSession_;
    std::unique_ptr<persistence::Database> database_;
    std::unique_ptr<persistence::ProfileService> profileService_;
    std::unique_ptr<persistence::SettingsService> settingsService_;
    std::unique_ptr<SettingsController> settings_;
    std::unique_ptr<persistence::MatchService> matchService_;
    std::unique_ptr<persistence::MatchRecorder> ticTacToeRecorder_;
    std::unique_ptr<persistence::MatchRecorder> pingPongRecorder_;
    std::unique_ptr<persistence::MatchRepository> matchRepository_;
    std::unique_ptr<persistence::StatisticsRepository> statisticsRepository_;
    std::unique_ptr<persistence::AchievementService> achievementService_;
    achievements::AchievementNotificationQueue achievementNotifications_;
    std::int64_t selectedStatsProfileId_{};
    std::unique_ptr<AppContext> context_;
    std::unique_ptr<GameLauncher> launcher_;
    // Declared after everything the scenes reference so the scenes are destroyed first.
    SceneManager scenes_;
    std::unique_ptr<AchievementToast> achievementToast_;
};
