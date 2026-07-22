#pragma once

#include "Scene.hpp"
#include "SceneManager.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <cstddef>
#include <vector>

class MainMenuScene final : public Scene {
public:
    MainMenuScene(const sf::Font& regularFont,
                  const sf::Font& semiboldFont,
                  SceneManager& sceneManager,
                  sf::RenderWindow& window);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;

private:
    void activate(std::size_t index);
    void moveSelection(int offset);
    void refreshSelection();

    SceneManager& sceneManager_;
    sf::RenderWindow& window_;
    sf::Text eyebrow_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text footer_;
    sf::RectangleShape featurePanel_;
    sf::CircleShape glowPrimary_;
    sf::CircleShape glowSecondary_;
    std::vector<UiButton> buttons_;
    std::size_t selectedIndex_{0};
};
