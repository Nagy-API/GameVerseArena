#pragma once

#include "TicTacToeSession.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

#include <array>
#include <string>

enum class TicTacToeResultAction { None, NextRound, RestartRound, Rematch, NewSetup, ReturnToLibrary };

class TicTacToeResultOverlay {
public:
    TicTacToeResultOverlay(const sf::Font& regularFont, const sf::Font& semiboldFont);

    void show(const classic_ttt::TicTacToeSession& session);
    void hide() noexcept { visible_ = false; }
    void setWarning(const std::string& warning) { warning_.setString(warning); }
    bool visible() const noexcept { return visible_; }
    TicTacToeResultAction handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(sf::Time deltaTime);
    void draw(sf::RenderTarget& target) const;

private:
    void select(std::size_t index);
    TicTacToeResultAction actionFor(std::size_t index) const;

    sf::RectangleShape shade_;
    sf::RectangleShape panel_;
    sf::Text eyebrow_;
    sf::Text title_;
    sf::Text roundResult_;
    sf::Text score_;
    sf::Text warning_;
    std::array<UiButton, 3> buttons_;
    std::size_t selected_{};
    bool visible_{false};
    bool matchFinished_{false};
};
