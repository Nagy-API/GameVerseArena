#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>

class UiButton {
public:
    UiButton(const sf::Font& font, std::string text, sf::Vector2f size);

    void setPosition(sf::Vector2f position);
    void setHovered(bool hovered) noexcept { hovered_ = hovered; }
    void setSelected(bool selected) noexcept { selected_ = selected; }

    bool contains(sf::Vector2f point) const;
    void update(sf::Time deltaTime);
    void draw(sf::RenderTarget& target) const;

private:
    void centerLabel();

    sf::RectangleShape background_;
    sf::RectangleShape accentBar_;
    sf::Text label_;
    bool hovered_{false};
    bool selected_{false};
    float emphasis_{0.f};
};
