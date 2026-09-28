#pragma once

#include "AppContext.hpp"
#include "Scene.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <cstddef>
#include <vector>

class MainMenuScene final : public Scene {
public:
    explicit MainMenuScene(AppContext& context);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;
    void onActivate() override;

private:
    void activate(std::size_t index);
    void moveSelection(int offset);
    void select(std::size_t index, bool withSound);

    AppContext& context_;
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
