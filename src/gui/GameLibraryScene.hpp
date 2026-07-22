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

private:
    void goBack();

    SceneManager& sceneManager_;
    sf::Text kicker_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::RectangleShape boardCard_;
    sf::RectangleShape arcadeCard_;
    sf::Text boardTitle_;
    sf::Text boardCount_;
    sf::Text boardDescription_;
    sf::Text arcadeTitle_;
    sf::Text plannedBadge_;
    sf::Text arcadeDescription_;
    UiButton backButton_;
};
