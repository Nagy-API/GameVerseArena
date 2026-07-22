#pragma once

#include "Scene.hpp"
#include "SceneManager.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

class AboutScene final : public Scene {
public:
    AboutScene(const sf::Font& regularFont, const sf::Font& semiboldFont, SceneManager& sceneManager);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;

private:
    void goBack();

    SceneManager& sceneManager_;
    sf::Text kicker_;
    sf::Text title_;
    sf::RectangleShape detailsCard_;
    sf::Text technology_;
    sf::Text milestone_;
    sf::Text description_;
    UiButton backButton_;
};
