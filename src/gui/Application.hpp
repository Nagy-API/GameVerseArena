#pragma once

#include "AssetManager.hpp"
#include "SceneManager.hpp"
#include "TicTacToeSession.hpp"
#include "PingPongSession.hpp"
#include "Database.hpp"
#include "ProfileService.hpp"
#include "MatchService.hpp"
#include "MatchRecorder.hpp"
#include "MatchRepository.hpp"
#include "StatisticsRepository.hpp"
#include "AchievementService.hpp"
#include "AchievementNotificationQueue.hpp"
#include "AchievementToast.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

#include <filesystem>
#include <memory>
#include <optional>

class Application {
public:
    explicit Application(std::filesystem::path executableDirectory,
                         std::optional<std::filesystem::path> databasePath = std::nullopt);
    int run();

private:
    bool initialize();
    void updateView(sf::Vector2u windowSize);

    std::filesystem::path executableDirectory_;
    std::optional<std::filesystem::path> databasePath_;
    sf::RenderWindow window_;
    AssetManager assets_;
    SceneManager scenes_;
    classic_ttt::TicTacToeSession ticTacToeSession_;
    ping_pong::PingPongSession pingPongSession_;
    std::unique_ptr<persistence::Database> database_;
    std::unique_ptr<persistence::ProfileService> profileService_;
    std::unique_ptr<persistence::MatchService> matchService_;
    std::unique_ptr<persistence::MatchRecorder> ticTacToeRecorder_;
    std::unique_ptr<persistence::MatchRecorder> pingPongRecorder_;
    std::unique_ptr<persistence::MatchRepository> matchRepository_;
    std::unique_ptr<persistence::StatisticsRepository> statisticsRepository_;
    std::unique_ptr<persistence::AchievementService> achievementService_;
    achievements::AchievementNotificationQueue achievementNotifications_;
    std::unique_ptr<AchievementToast> achievementToast_;
    std::int64_t selectedStatsProfileId_{};
};
