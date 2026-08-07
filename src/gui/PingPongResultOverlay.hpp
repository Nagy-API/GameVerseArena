#pragma once

#include "PingPongSession.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

#include <array>

enum class PingPongResultAction { None, Rematch, NewSetup, ReturnToLibrary };

class PingPongResultOverlay {
public:
    PingPongResultOverlay(const sf::Font& regularFont, const sf::Font& semiboldFont);
    void show(const ping_pong::PingPongSession& session);
    void hide() noexcept { visible_ = false; }
    bool visible() const noexcept { return visible_; }
    PingPongResultAction handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(sf::Time deltaTime);
    void draw(sf::RenderTarget& target) const;

private:
    void select(std::size_t index);
    sf::RectangleShape shade_;
    sf::RectangleShape panel_;
    sf::Text eyebrow_;
    sf::Text title_;
    sf::Text score_;
    sf::Text message_;
    std::array<UiButton, 3> buttons_;
    std::size_t selected_{};
    bool visible_{false};
};
