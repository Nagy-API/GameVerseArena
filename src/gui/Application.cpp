#include "Application.hpp"

#include "AboutScene.hpp"
#include "BoardGameScene.hpp"
#include "BoardGameSetupScene.hpp"
#include "BoardViews.hpp"
#include "DatabasePaths.hpp"
#include "GameLibraryScene.hpp"
#include "MainMenuScene.hpp"
#include "MatchHistoryScene.hpp"
#include "PingPongGameScene.hpp"
#include "PingPongSetupScene.hpp"
#include "ProfileAchievementsScene.hpp"
#include "ProfileStatsScene.hpp"
#include "ProfilesScene.hpp"
#include "SettingsScene.hpp"
#include "SmokeTestDriver.hpp"
#include "Theme.hpp"
#include "TicTacToeGameScene.hpp"
#include "TicTacToeSetupScene.hpp"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Clock.hpp>

#include "ErrorDialog.hpp"

#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

Application::Application(std::filesystem::path executableDirectory, ApplicationOptions options)
    : executableDirectory_(std::move(executableDirectory)),
      options_(std::move(options)),
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
    try {
        return runLoop();
    } catch (const std::exception& error) {
        // Last resort: a failure that escaped a scene is reported instead of terminating
        // silently. Completed matches were already saved when they finished.
        if (audio_) audio_->stopAll();
        window_.close();
        reportFatal("Unexpected error", std::string("GameVerseArena stopped because of an unexpected error. "
                                                    "Completed matches and settings saved earlier are kept.\n\n") +
                                            error.what());
        return 1;
    }
}

int Application::runLoop()
{
    if (options_.smokeTestDirectory) {
        SmokeTestDriver driver(*this, *options_.smokeTestDirectory);
        const int result = driver.run();
        if (audio_) audio_->stopAll();
        window_.close();
        return result;
    }

    sf::Clock clock;
    while (window_.isOpen()) {
        while (const auto event = window_.pollEvent()) {
            dispatch(*event);
        }
        advanceFrame(sf::seconds(std::min(clock.restart().asSeconds(), 0.05f)));
    }

    if (audio_) audio_->stopAll();
    return 0;
}

void Application::dispatch(const sf::Event& event)
{
    if (event.is<sf::Event::Closed>()) {
        window_.close();
        return;
    }
    if (const auto* resized = event.getIf<sf::Event::Resized>()) {
        updateView(resized->size);
        scenes_.active().onResize(resized->size);
    }
    if (achievementToast_ && achievementToast_->handleEvent(event, window_)) {
        return;
    }
    if (window_.isOpen()) {
        scenes_.active().handleEvent(event, window_);
    }
}

void Application::advanceFrame(sf::Time deltaTime, const std::filesystem::path* capturePath)
{
    scenes_.active().update(deltaTime);
    achievementToast_->update(deltaTime);

    window_.clear(Theme::background);
    scenes_.active().render(window_);
    achievementToast_->render(window_);
    if (capturePath != nullptr) {
        sf::Texture capture;
        if (capture.resize(window_.getSize())) {
            capture.update(window_);
            if (!capture.copyToImage().saveToFile(*capturePath)) {
                std::cerr << "GameVerseArenaGUI: could not save capture " << capturePath->string() << '\n';
            }
        }
    }
    window_.display();
}

void Application::reportFatal(const std::string& title, const std::string& message) const
{
    std::cerr << "GameVerseArenaGUI: " << message << '\n';
    // Unattended smoke-test runs report through stderr and the exit code only.
    if (!options_.smokeTestDirectory) showErrorDialog("GameVerseArenaGUI - " + title, message);
}

bool Application::initialize()
{
    std::string error;
    if (!assets_.load(executableDirectory_ / "assets", error)) {
        reportFatal("Missing asset", error + "\n\nReinstall or re-extract GameVerseArena so the assets folder "
                                             "sits beside the executable.");
        return false;
    }

    try {
        audio_ = std::make_unique<AudioEngine>(options_.silentAudio ? AudioEngine::Output::NullDevice
                                                                    : AudioEngine::Output::DefaultDevice);
    } catch (const std::exception& exception) {
        reportFatal("Audio error", std::string("The audio system could not be created.\n\n") + exception.what());
        return false;
    }

    bool opened = false;
    try {
        const auto path = options_.databasePath.has_value()
            ? *options_.databasePath
            : persistence::DatabasePaths::productionDatabasePath();
        database_ = std::make_unique<persistence::Database>(path);
        opened = true;
        profileService_ = std::make_unique<persistence::ProfileService>(*database_);
        profileService_->bootstrap();
        settingsService_ = std::make_unique<persistence::SettingsService>(*database_);
        settings_ = std::make_unique<SettingsController>(*settingsService_, *audio_);
        settings_->load();
        matchService_ = std::make_unique<persistence::MatchService>(*database_);
        ticTacToeRecorder_ = std::make_unique<persistence::MatchRecorder>(*matchService_);
        pingPongRecorder_ = std::make_unique<persistence::MatchRecorder>(*matchService_);
        boardGameRecorder_ = std::make_unique<persistence::MatchRecorder>(*matchService_);
        matchRepository_ = std::make_unique<persistence::MatchRepository>(*database_);
        statisticsRepository_ = std::make_unique<persistence::StatisticsRepository>(*database_);
        achievementService_ = std::make_unique<persistence::AchievementService>(*database_);
        achievementService_->backfillAll();
    } catch (const std::exception& exception) {
        const std::string explanation = opened
            ? "The local profile database opened, but loading profiles, settings, or achievements failed, so "
              "GameVerseArena has stopped. No match history was deleted.\n\n"
            : "The local profile database could not be opened or upgraded, so GameVerseArena has stopped. "
              "An upgrade step that could not finish was rolled back and no data was deleted.\n\n";
        reportFatal("Profile database error", explanation + exception.what());
        return false;
    }

    context_ = std::make_unique<AppContext>(AppContext{
        assets_.regularFont(), assets_.semiboldFont(), scenes_, window_, *audio_, *settings_});
    achievementToast_ = std::make_unique<AchievementToast>(*context_, achievementNotifications_);

    auto& context = *context_;
    scenes_.add(SceneId::MainMenu, std::make_unique<MainMenuScene>(context));
    scenes_.add(SceneId::Profiles, std::make_unique<ProfilesScene>(context, *profileService_, selectedStatsProfileId_));
    scenes_.add(SceneId::ProfileStats, std::make_unique<ProfileStatsScene>(
        context, *profileService_, *statisticsRepository_, *achievementService_, selectedStatsProfileId_,
        selectedStatsGame_));
    scenes_.add(SceneId::ProfileAchievements, std::make_unique<ProfileAchievementsScene>(
        context, *profileService_, *achievementService_, selectedStatsProfileId_));
    scenes_.add(SceneId::MatchHistory, std::make_unique<MatchHistoryScene>(
        context, *profileService_, *matchRepository_, selectedStatsProfileId_, selectedStatsGame_));
    launcher_ = std::make_unique<GameLauncher>(scenes_);
    launcher_->bindDedicatedScene("classic_tic_tac_toe", SceneId::TicTacToeSetup);
    launcher_->bindDedicatedScene("ping_pong", SceneId::PingPongSetup);
    launcher_->setBoardGameLauncher([this](const catalogue::GameDescriptor& game) {
        boardGameHost_.game = &game;
        scenes_.switchTo(SceneId::BoardGameSetup);
    });
    // The catalogue's "playable" flag and the launcher must agree for every game, and every shared
    // board game needs its board view, or the library would advertise a game it cannot start (or
    // hide one it can).
    for (const auto& game : catalogue::all()) {
        const bool viewAvailable =
            game.launch != catalogue::LaunchKind::BoardGame || board_view::createBoardView(game.key) != nullptr;
        if (game.playableInGui() != launcher_->canLaunch(game) || !viewAvailable) {
            reportFatal("Internal error", "The game library entry '" + game.key +
                                              "' does not match the available game scenes.");
            return false;
        }
    }
    scenes_.add(SceneId::GameLibrary, std::make_unique<GameLibraryScene>(context, *launcher_));
    scenes_.add(SceneId::TicTacToeSetup, std::make_unique<TicTacToeSetupScene>(
        context, ticTacToeSession_, *profileService_, *ticTacToeRecorder_));
    scenes_.add(SceneId::TicTacToeGame, std::make_unique<TicTacToeGameScene>(
        context, ticTacToeSession_, *ticTacToeRecorder_, *achievementService_, achievementNotifications_));
    scenes_.add(SceneId::PingPongSetup, std::make_unique<PingPongSetupScene>(
        context, pingPongSession_, *profileService_, *pingPongRecorder_));
    scenes_.add(SceneId::PingPongGame, std::make_unique<PingPongGameScene>(
        context, pingPongSession_, *pingPongRecorder_, *achievementService_, achievementNotifications_));
    scenes_.add(SceneId::BoardGameSetup, std::make_unique<BoardGameSetupScene>(
        context, boardGameHost_, *profileService_, *boardGameRecorder_));
    scenes_.add(SceneId::BoardGame, std::make_unique<BoardGameScene>(
        context, boardGameHost_, *boardGameRecorder_, *achievementService_, achievementNotifications_));
    scenes_.add(SceneId::Settings, std::make_unique<SettingsScene>(context));
    const auto playable = catalogue::playableCount();
    const auto total = catalogue::all().size();
    std::string aboutText = std::to_string(playable) + " of the " + std::to_string(total) +
                            " games in the library are playable here, including real-time Ping Pong.\n"
                            "All 14 original board games remain available in the separate console application";
    aboutText += playable == total ? ".\n" : ";\nthe remaining board games are still awaiting graphical migration.\n";
    aboutText += "Profiles, match history, statistics, achievements, and settings are stored only on this computer.";
    scenes_.add(SceneId::About, std::make_unique<AboutScene>(context, aboutText));
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
