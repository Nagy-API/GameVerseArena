#pragma once

#include "BoardView.hpp"

#include <SFML/Graphics/Color.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace board_view {

// Drawing helpers shared by the board views.
sf::Color withAlpha(sf::Color color, std::uint8_t alpha);
sf::Vector2f centerOf(const sf::FloatRect& rect);
void drawCross(sf::RenderTarget& target, sf::Vector2f center, float size, const sf::Color& color);
void drawRing(sf::RenderTarget& target, sf::Vector2f center, float size, const sf::Color& color);
void drawStrike(sf::RenderTarget& target, sf::Vector2f from, sf::Vector2f to, float thickness, const sf::Color& color);
void drawCenteredText(sf::RenderTarget& target, const sf::Font& font, const std::string& text, sf::Vector2f center,
                      unsigned int size, const sf::Color& color);
// An X (`cross`) or O mark sized for a cell, grown by `scale` and faded by `alpha`.
void drawMark(sf::RenderTarget& target, bool cross, sf::Vector2f center, float cellSize, float scale,
              std::uint8_t alpha = 255);
// Square cells in rows, row-major, `gap` apart, centred in `area`.
std::vector<sf::FloatRect> gridCells(sf::FloatRect area, int columns, int rows, float cellSize, float gap);
// The rectangle around all `cells`, grown by `padding` on every side.
sf::FloatRect frameAround(const std::vector<sf::FloatRect>& cells, float padding);

// A board made of individually selectable cells: a grid or any other arrangement of
// rectangles, indexed like the game's cell-based moves. The cursor follows the mouse and the
// arrow keys (moving to the nearest cell in that direction and wrapping at the edges); choosing
// a cell asks the game-specific moveForCell() for the move to play.
class CellBoardView : public BoardView {
public:
    void reset(const turn_based::TurnBasedGame& game) override;
    void movePlayed(const turn_based::TurnBasedGame& game, turn_based::MoveId move) override;
    void update(float seconds, bool reducedMotion) override;
    Response pointerMoved(sf::Vector2f point, const turn_based::TurnBasedGame& game, bool interactive) override;
    Response pointerReleased(sf::Vector2f point, const turn_based::TurnBasedGame& game, bool interactive) override;
    Response navigate(int columns, int rows, const turn_based::TurnBasedGame& game, bool interactive) override;
    Response activate(const turn_based::TurnBasedGame& game, bool interactive) override;
    void draw(sf::RenderTarget& target, const turn_based::TurnBasedGame& game, const DrawState& state) const override;

    int cellCount() const noexcept { return static_cast<int>(cells_.size()); }
    int cursor() const noexcept { return cursor_; }
    // Centre of a cell in logical coordinates (used by the smoke test to click cells).
    std::optional<sf::Vector2f> cellCenter(int cell) const;

protected:
    // `frame` is the board's outer rectangle, drawn behind the cells; an empty frame draws none
    // (for boards that are not rectangular).
    void setCells(std::vector<sf::FloatRect> cells, sf::FloatRect frame);
    std::optional<int> cellAt(sf::Vector2f point) const;
    const sf::FloatRect& cellRect(int cell) const { return cells_.at(static_cast<std::size_t>(cell)); }
    void setCursor(int cell) noexcept { if (cell >= 0 && cell < cellCount()) cursor_ = cell; }
    // Growth of the newest mark, 0.05 to 1 (always 1 with Reduced Motion).
    float placeScale(int cell, const DrawState& state) const noexcept;
    Response chooseCell(const turn_based::TurnBasedGame& game, int cell) const;

    // The move that places the current player's piece in `cell`, or nullopt if it cannot be played now.
    virtual std::optional<turn_based::MoveId> moveForCell(const turn_based::TurnBasedGame& game, int cell) const = 0;
    // The cell a move was played in (to animate it); `game` already includes the move.
    virtual int cellOfMove(const turn_based::TurnBasedGame&, turn_based::MoveId move) const { return move; }
    // Cells lit by the cursor: normally just the cursor cell (a whole column in Four-in-a-Row).
    virtual bool cursorCovers(int cell, int cursorCell) const { return cell == cursorCell; }
    // A short explanation when choosing `cell` cannot make a move (empty: just the error sound).
    virtual std::string refusal(const turn_based::TurnBasedGame&, int) const { return {}; }
    // Cells to emphasise (winning, losing, or scoring lines).
    virtual bool cellHighlighted(const turn_based::TurnBasedGame&, int) const { return false; }
    virtual sf::Color highlightColor() const;
    virtual void drawCellContent(sf::RenderTarget& target, const turn_based::TurnBasedGame& game, int cell,
                                 const sf::FloatRect& rect, const DrawState& state) const = 0;
    // A faded preview of the pending move, called for each cell the cursor covers.
    virtual void drawPreview(sf::RenderTarget&, const turn_based::TurnBasedGame&, int, const sf::FloatRect&,
                             const DrawState&) const {}
    // Drawn above the cells, e.g. lines through completed rows.
    virtual void drawOverlay(sf::RenderTarget&, const turn_based::TurnBasedGame&, const DrawState&) const {}
    virtual int initialCursor() const { return cellCount() / 2; }

private:
    std::vector<sf::FloatRect> cells_;
    sf::FloatRect frame_;
    int cursor_{0};
    std::optional<int> lastCell_;
    float animation_{1.f};
};

} // namespace board_view
