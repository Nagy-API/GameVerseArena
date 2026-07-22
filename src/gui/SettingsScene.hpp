#pragma once

#include "Scene.hpp"
#include "SceneManager.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <vector>

class SettingsScene final : public Scene {
public:
    SettingsScene(const sf::Font& regularFont, const sf::Font& semiboldFont, SceneManager& sceneManager);

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
    std::vector<sf::RectangleShape> cards_;
    std::vector<sf::Text> cardTitles_;
    std::vector<sf::Text> cardStatuses_;
    UiButton backButton_;
};
