#pragma once

#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

#include <array>

enum class PingPongPauseAction { None, Resume, RestartMatch, NewSetup, ReturnToLibrary };

class PingPongPauseOverlay {
public:
    PingPongPauseOverlay(const sf::Font& regularFont, const sf::Font& semiboldFont);
    PingPongPauseAction handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(sf::Time deltaTime);
    void draw(sf::RenderTarget& target) const;
    void resetSelection();

private:
    void select(std::size_t index);
    PingPongPauseAction actionFor(std::size_t index) const;

    sf::RectangleShape shade_;
    sf::RectangleShape panel_;
    sf::Text eyebrow_;
    sf::Text title_;
    sf::Text message_;
    std::array<UiButton, 4> buttons_;
    std::size_t selected_{};
};
