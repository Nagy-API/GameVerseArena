#pragma once

#include "TurnBasedGame.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

#include <optional>
#include <string>

// Presentation and input for one graphical board game. A view draws the game's board inside the
// area the shared board-game scene gives it and turns pointer and keyboard input into complete
// moves. It never changes the game: the scene applies the moves it returns, so every rule stays
// in the SFML-independent game classes.
namespace board_view {

struct Response {
    enum class Feedback { None, Focus, Select, Invalid };

    std::optional<turn_based::MoveId> move;  // a complete move the player chose
    Feedback feedback{Feedback::None};
};

struct DrawState {
    const sf::Font& regularFont;
    const sf::Font& semiboldFont;
    bool interactive{};    // a human may move now: show the cursor and move previews
    bool reducedMotion{};  // no pulsing or placement animation
    float time{};          // seconds since the scene opened, for gentle pulses
};

class BoardView {
public:
    virtual ~BoardView() = default;

    // Places the board inside `area` (logical 1280 x 720 coordinates).
    virtual void layout(sf::FloatRect area) = 0;
    // A new game or a restart: clears the cursor, selections, and animations.
    virtual void reset(const turn_based::TurnBasedGame& game) = 0;
    // A move (by either player) was applied to `game`.
    virtual void movePlayed(const turn_based::TurnBasedGame& game, turn_based::MoveId move) = 0;
    virtual void update(float seconds, bool reducedMotion) = 0;

    virtual Response pointerMoved(sf::Vector2f point, const turn_based::TurnBasedGame& game, bool interactive) = 0;
    virtual Response pointerReleased(sf::Vector2f point, const turn_based::TurnBasedGame& game, bool interactive) = 0;
    // Arrow keys: one step left/right (`columns`) or up/down (`rows`).
    virtual Response navigate(int columns, int rows, const turn_based::TurnBasedGame& game, bool interactive) = 0;
    // Enter or Space on the board.
    virtual Response activate(const turn_based::TurnBasedGame& game, bool interactive) = 0;
    // Typed text, for games that choose a number or letter.
    virtual Response character(char32_t, const turn_based::TurnBasedGame&, bool) { return {}; }
    // Escape: drops a pending selection. Returns false when there was nothing to drop.
    virtual bool cancelSelection() { return false; }

    virtual void draw(sf::RenderTarget& target, const turn_based::TurnBasedGame& game, const DrawState& state) const = 0;

    // What the player to move does next, e.g. "place an S" (shown after the player's name).
    virtual std::string moveHint(const turn_based::TurnBasedGame& game) const = 0;
    // Short keyboard help for this board.
    virtual std::string controls() const = 0;
    // Optional extra line for a seat's panel, e.g. the numbers a player has left.
    virtual std::string seatDetail(const turn_based::TurnBasedGame&, turn_based::Seat) const { return {}; }
};

} // namespace board_view
