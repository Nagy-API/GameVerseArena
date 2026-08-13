#include "Application.hpp"

#include "AboutScene.hpp"
#include "GameLibraryScene.hpp"
#include "MainMenuScene.hpp"
#include "SettingsScene.hpp"
#include "Theme.hpp"
#include "TicTacToeGameScene.hpp"
#include "TicTacToeSetupScene.hpp"
#include "PingPongGameScene.hpp"
#include "PingPongSetupScene.hpp"
#include "ProfilesScene.hpp"
#include "ProfileStatsScene.hpp"
#include "MatchHistoryScene.hpp"
#include "ProfileAchievementsScene.hpp"
#include "DatabasePaths.hpp"
#include "AchievementToast.hpp"

#include <SFML/Graphics/View.hpp>
#include <SFML/System/Clock.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

Application::Application(std::filesystem::path executableDirectory,
                         std::optional<std::filesystem::path> databasePath)
    : executableDirectory_(std::move(executableDirectory)),
      databasePath_(std::move(databasePath)),
      window_(sf::VideoMode({1280u, 720u}), "GameVerseArena")
{
    window_.setMinimumSize(sf::Vector2u{960u, 540u});
    window_.setFramerateLimit(60);
    window_.setKeyRepeatEnabled(false);
}

int Application::run()
{
    if (!initialize()) {
        window_.close();
        return 1;
    }

    updateView(window_.getSize());
    sf::Clock clock;

    while (window_.isOpen()) {
        while (const auto event = window_.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window_.close();
                continue;
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                updateView(resized->size);
                scenes_.active().onResize(resized->size);
            }

            if (achievementToast_ && achievementToast_->handleEvent(*event, window_)) {
                continue;
            }

            if (window_.isOpen()) {
                scenes_.active().handleEvent(*event, window_);
            }
        }

        const auto deltaTime = sf::seconds(std::min(clock.restart().asSeconds(), 0.05f));
        scenes_.active().update(deltaTime);
        achievementToast_->update(deltaTime);

        window_.clear(Theme::background);
        scenes_.active().render(window_);
        achievementToast_->render(window_);
        window_.display();
    }

    return 0;
}

bool Application::initialize()
{
    std::string error;
    if (!assets_.load(executableDirectory_ / "assets", error)) {
        std::cerr << "GameVerseArenaGUI: " << error << '\n';
#ifdef _WIN32
        MessageBoxA(nullptr, error.c_str(), "GameVerseArenaGUI - Missing asset", MB_OK | MB_ICONERROR);
#endif
        return false;
    }

    try {
        const auto path = databasePath_.has_value()
            ? *databasePath_
            : persistence::DatabasePaths::productionDatabasePath();
        database_ = std::make_unique<persistence::Database>(path);
        profileService_ = std::make_unique<persistence::ProfileService>(*database_);
        profileService_->bootstrap();
        matchService_ = std::make_unique<persistence::MatchService>(*database_);
        ticTacToeRecorder_ = std::make_unique<persistence::MatchRecorder>(*matchService_);
        pingPongRecorder_ = std::make_unique<persistence::MatchRecorder>(*matchService_);
        matchRepository_ = std::make_unique<persistence::MatchRepository>(*database_);
        statisticsRepository_ = std::make_unique<persistence::StatisticsRepository>(*database_);
        achievementService_ = std::make_unique<persistence::AchievementService>(*database_);
        achievementService_->backfillAll();
        achievementToast_ = std::make_unique<AchievementToast>(
            assets_.regularFont(), assets_.semiboldFont(), achievementNotifications_);
    } catch (const std::exception& exception) {
        const std::string error = std::string("Player profiles could not be initialized.\n\n") + exception.what();
        std::cerr << "GameVerseArenaGUI: " << error << '\n';
#ifdef _WIN32
        MessageBoxA(nullptr, error.c_str(), "GameVerseArenaGUI - Profile database error", MB_OK | MB_ICONERROR);
#endif
        return false;
    }

    scenes_.add(SceneId::MainMenu, std::make_unique<MainMenuScene>(
        assets_.regularFont(), assets_.semiboldFont(), scenes_, window_));
    scenes_.add(SceneId::Profiles, std::make_unique<ProfilesScene>(
        assets_.regularFont(), assets_.semiboldFont(), scenes_, *profileService_, selectedStatsProfileId_));
    scenes_.add(SceneId::ProfileStats, std::make_unique<ProfileStatsScene>(
        assets_.regularFont(), assets_.semiboldFont(), scenes_, *profileService_,
        *statisticsRepository_, *achievementService_, selectedStatsProfileId_));
    scenes_.add(SceneId::ProfileAchievements, std::make_unique<ProfileAchievementsScene>(
        assets_.regularFont(), assets_.semiboldFont(), scenes_, *profileService_,
        *achievementService_, selectedStatsProfileId_));
    scenes_.add(SceneId::MatchHistory, std::make_unique<MatchHistoryScene>(
        assets_.regularFont(), assets_.semiboldFont(), scenes_, *profileService_,
        *matchRepository_, selectedStatsProfileId_));
    scenes_.add(SceneId::GameLibrary, std::make_unique<GameLibraryScene>(
        assets_.regularFont(), assets_.semiboldFont(), scenes_));
    scenes_.add(SceneId::TicTacToeSetup, std::make_unique<TicTacToeSetupScene>(
        assets_.regularFont(), assets_.semiboldFont(), scenes_, ticTacToeSession_, *profileService_, *ticTacToeRecorder_));
    scenes_.add(SceneId::TicTacToeGame, std::make_unique<TicTacToeGameScene>(
        assets_.regularFont(), assets_.semiboldFont(), scenes_, ticTacToeSession_, *ticTacToeRecorder_,
        *achievementService_, achievementNotifications_));
    scenes_.add(SceneId::PingPongSetup, std::make_unique<PingPongSetupScene>(
        assets_.regularFont(), assets_.semiboldFont(), scenes_, pingPongSession_, *profileService_, *pingPongRecorder_));
    scenes_.add(SceneId::PingPongGame, std::make_unique<PingPongGameScene>(
        assets_.regularFont(), assets_.semiboldFont(), scenes_, pingPongSession_, *pingPongRecorder_,
        *achievementService_, achievementNotifications_));
    scenes_.add(SceneId::Settings, std::make_unique<SettingsScene>(
        assets_.regularFont(), assets_.semiboldFont(), scenes_));
    scenes_.add(SceneId::About, std::make_unique<AboutScene>(
        assets_.regularFont(), assets_.semiboldFont(), scenes_));
    scenes_.switchTo(SceneId::MainMenu);
    return true;
}

void Application::updateView(sf::Vector2u windowSize)
{
    if (windowSize.x == 0 || windowSize.y == 0) {
        return;
    }

    const float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    const float targetRatio = Theme::logicalSize.x / Theme::logicalSize.y;
    sf::FloatRect viewport({0.f, 0.f}, {1.f, 1.f});

    if (windowRatio > targetRatio) {
        viewport.size.x = targetRatio / windowRatio;
        viewport.position.x = (1.f - viewport.size.x) / 2.f;
    } else {
        viewport.size.y = windowRatio / targetRatio;
        viewport.position.y = (1.f - viewport.size.y) / 2.f;
    }

    sf::View view({Theme::logicalSize.x / 2.f, Theme::logicalSize.y / 2.f}, Theme::logicalSize);
    view.setViewport(viewport);
    window_.setView(view);
}
