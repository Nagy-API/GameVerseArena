#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "FourInRowGame.hpp"
#include "Theme.hpp"

#include <algorithm>
#include <utility>

namespace board_view {
namespace {

using four_in_a_row::Disc;
using four_in_a_row::FourInRowGame;
using turn_based::MoveId;
using turn_based::Seat;
using turn_based::TurnBasedGame;

constexpr int columns = FourInRowGame::columns;

// The 6 x 7 grid. The cursor lights a whole column; choosing any cell of a column drops a disc
// into it, and the preview shows where the disc will land.
class FourInRowView final : public CellBoardView {
public:
    void layout(sf::FloatRect area) override
    {
        auto cells = gridCells(area, columns, FourInRowGame::rows, 66.f, 5.f);
        const auto frame = frameAround(cells, 12.f);
        setCells(std::move(cells), frame);
    }

    Response navigate(int columnStep, int rowStep, const TurnBasedGame& game, bool interactive) override
    {
        (void)rowStep;  // Up and Down would only change the row inside the lit column.
        if (columnStep == 0) return {};
        return CellBoardView::navigate(columnStep, 0, game, interactive);
    }

    std::string moveHint(const TurnBasedGame& game) const override
    {
        return std::string("drop ") + (game.currentSeat() == Seat::First ? "X" : "O") + " into a column";
    }

    std::string controls() const override { return "Left / Right choose a column  |  Enter or click drops"; }

protected:
    int initialCursor() const override { return FourInRowGame::indexOf(FourInRowGame::rows - 1, 3); }

    std::optional<MoveId> moveForCell(const TurnBasedGame& game, int cell) const override
    {
        const int column = cell % columns;
        if (!game.isLegal(column)) return std::nullopt;
        return column;
    }

    // The disc landed just above the column's next free row.
    int cellOfMove(const TurnBasedGame& game, MoveId move) const override
    {
        return FourInRowGame::indexOf(typed(game).landingRow(move) + 1, move);
    }

    bool cursorCovers(int cell, int cursorCell) const override { return cell % columns == cursorCell % columns; }

    bool cellHighlighted(const TurnBasedGame& game, int cell) const override
    {
        const auto& line = typed(game).winningLine();
        return line && std::find(line->begin(), line->end(), cell) != line->end();
    }

    void drawCellContent(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                         const DrawState& state) const override
    {
        const Disc disc = typed(game).cell(cell / columns, cell % columns);
        if (disc == Disc::Empty) return;
        drawMark(target, disc == Disc::X, centerOf(rect), rect.size.x, placeScale(cell, state));
    }

    void drawPreview(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                     const DrawState&) const override
    {
        const auto& board = typed(game);
        const int row = board.landingRow(cell % columns);
        if (row != cell / columns) return;
        drawMark(target, game.currentSeat() == Seat::First, centerOf(rect), rect.size.x, 1.f, 70);
    }

    void drawOverlay(sf::RenderTarget& target, const TurnBasedGame& game, const DrawState&) const override
    {
        const auto& line = typed(game).winningLine();
        if (!line || line->empty()) return;
        drawStrike(target, centerOf(cellRect(line->front())), centerOf(cellRect(line->back())), 7.f,
                   withAlpha(Theme::secondary, 210));
    }

private:
    static const FourInRowGame& typed(const TurnBasedGame& game) { return dynamic_cast<const FourInRowGame&>(game); }
};

} // namespace

std::unique_ptr<BoardView> makeFourInRowView()
{
    return std::make_unique<FourInRowView>();
}

} // namespace board_view
