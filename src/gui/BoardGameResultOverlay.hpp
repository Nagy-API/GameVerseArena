#pragma once

#include "AppContext.hpp"
#include "BoardGameSession.hpp"
#include "GameCatalogue.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

#include <array>
#include <string>

enum class BoardGameResultAction { None, Rematch, ViewBoard, NewSetup, ReturnToLibrary };

// The finished-game panel of the shared board-game scene: who won and why, the final points for
// games that score them, and what to do next. Escape (or "View Final Board") hides it so the
// final position can be studied.
class BoardGameResultOverlay {
public:
    explicit BoardGameResultOverlay(AppContext& context);

    void show(const turn_based::BoardGameSession& session, const turn_based::Outcome& outcome,
              const catalogue::GameDescriptor& game);
    // Shows the last result again after the player looked at the final board.
    void reveal();
    void hide() noexcept { visible_ = false; }
    void setWarning(const std::string& warning);
    bool visible() const noexcept { return visible_; }
    BoardGameResultAction handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(sf::Time deltaTime);
    void draw(sf::RenderTarget& target) const;

private:
    static constexpr std::size_t buttonCount = 4;
    void select(std::size_t index, bool withSound = false);
    static BoardGameResultAction actionFor(std::size_t index);

    AppContext& context_;
    sf::RectangleShape shade_;
    sf::RectangleShape panel_;
    sf::Text eyebrow_;
    sf::Text title_;
    sf::Text reason_;
    sf::Text score_;
    sf::Text warning_;
    std::array<UiButton, buttonCount> buttons_;
    std::size_t selected_{};
    bool visible_{false};
};
