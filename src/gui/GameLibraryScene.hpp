#pragma once

#include "Scene.hpp"
#include "SceneManager.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

class GameLibraryScene final : public Scene {
public:
    GameLibraryScene(const sf::Font& regularFont, const sf::Font& semiboldFont, SceneManager& sceneManager);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;
    void onActivate() override;

private:
    void goBack();
    void openClassic();
    void openPingPong();
    void refreshSelection();

    SceneManager& sceneManager_;
    sf::Text kicker_;
    sf::Text title_;
    sf::Text subtitle_;
    UiButton classicCard_;
    sf::Text classicDetails_;
    sf::RectangleShape consoleCard_;
    UiButton pingPongCard_;
    sf::Text pingPongDetails_;
    sf::Text consoleTitle_;
    sf::Text consoleBadge_;
    sf::Text consoleDescription_;
    UiButton backButton_;
    std::size_t selectedIndex_{};
};
