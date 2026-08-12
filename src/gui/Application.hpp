#pragma once

#include "AssetManager.hpp"
#include "SceneManager.hpp"
#include "TicTacToeSession.hpp"
#include "PingPongSession.hpp"
#include "Database.hpp"
#include "ProfileService.hpp"

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
};
