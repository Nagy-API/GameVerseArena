#pragma once

#include "AssetManager.hpp"
#include "SceneManager.hpp"
#include "TicTacToeSession.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

#include <filesystem>

class Application {
public:
    explicit Application(std::filesystem::path executableDirectory);
    int run();

private:
    bool initialize();
    void updateView(sf::Vector2u windowSize);

    std::filesystem::path executableDirectory_;
    sf::RenderWindow window_;
    AssetManager assets_;
    SceneManager scenes_;
    classic_ttt::TicTacToeSession ticTacToeSession_;
};
