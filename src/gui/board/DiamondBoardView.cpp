#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "DiamondGame.hpp"
#include "Theme.hpp"

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

namespace board_view {
namespace {

using diamond::DiamondGame;
using diamond::Mark;
using turn_based::MoveId;
using turn_based::Seat;
using turn_based::TurnBasedGame;

constexpr int size = DiamondGame::size;

// The 25 diamond cells, placed on the 7 x 7 grid they belong to. View cells are numbered row by
// row; moves are grid indices.
class DiamondView final : public CellBoardView {
public:
    DiamondView()
    {
        gridCell_.reserve(DiamondGame::cellCount);
        viewCell_.fill(-1);
        for (int row = 0; row < size; ++row) {
            for (int column = 0; column < size; ++column) {
                if (!DiamondGame::onBoard(row, column)) continue;
                viewCell_[static_cast<std::size_t>(row * size + column)] = static_cast<int>(gridCell_.size());
                gridCell_.push_back(row * size + column);
            }
        }
    }

    void layout(sf::FloatRect area) override
    {
        const auto grid = gridCells(area, size, size, 62.f, 6.f);
        std::vector<sf::FloatRect> cells;
        for (const int index : gridCell_) cells.push_back(grid[static_cast<std::size_t>(index)]);
        setCells(std::move(cells), {});  // no square backdrop behind the diamond
    }

    std::string moveHint(const TurnBasedGame& game) const override
    {
        return std::string("place ") + (game.currentSeat() == Seat::First ? "X" : "O") +
               "  |  a line of three plus a line of four wins";
    }

    std::string controls() const override { return "Arrows choose a cell  |  Enter or click places"; }

protected:
    int initialCursor() const override { return cellOf(DiamondGame::moveAt(3, 3)); }

    std::optional<MoveId> moveForCell(const TurnBasedGame& game, int cell) const override
    {
        const MoveId move = gridCell_.at(static_cast<std::size_t>(cell));
        if (!game.isLegal(move)) return std::nullopt;
        return move;
    }

    int cellOfMove(const TurnBasedGame&, MoveId move) const override { return cellOf(move); }

    bool cellHighlighted(const TurnBasedGame& game, int cell) const override
    {
        const auto& lines = typed(game).winningLines();
        if (!lines) return false;
        const int move = gridCell_.at(static_cast<std::size_t>(cell));
        return std::find(lines->three.begin(), lines->three.end(), move) != lines->three.end() ||
               std::find(lines->four.begin(), lines->four.end(), move) != lines->four.end();
    }

    void drawCellContent(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                         const DrawState& state) const override
    {
        const int move = gridCell_.at(static_cast<std::size_t>(cell));
        const Mark mark = typed(game).cell(move / size, move % size);
        if (mark == Mark::Empty) return;
        drawMark(target, mark == Mark::X, centerOf(rect), rect.size.x, placeScale(cell, state));
    }

    void drawPreview(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                     const DrawState&) const override
    {
        const int move = gridCell_.at(static_cast<std::size_t>(cell));
        if (typed(game).cell(move / size, move % size) != Mark::Empty) return;
        drawMark(target, game.currentSeat() == Seat::First, centerOf(rect), rect.size.x, 1.f, 70);
    }

    void drawOverlay(sf::RenderTarget& target, const TurnBasedGame& game, const DrawState&) const override
    {
        const auto& lines = typed(game).winningLines();
        if (!lines) return;
        for (const auto* line : {&lines->four, &lines->three}) {
            if (line->empty()) continue;
            drawStrike(target, centerOf(cellRect(cellOf(line->front()))), centerOf(cellRect(cellOf(line->back()))), 6.f,
                       withAlpha(Theme::secondary, 210));
        }
    }

private:
    static const DiamondGame& typed(const TurnBasedGame& game) { return dynamic_cast<const DiamondGame&>(game); }
    int cellOf(MoveId move) const { return move >= 0 && move < size * size ? viewCell_[static_cast<std::size_t>(move)] : -1; }

    std::vector<int> gridCell_;
    std::array<int, size * size> viewCell_{};
};

} // namespace

std::unique_ptr<BoardView> makeDiamondView()
{
    return std::make_unique<DiamondView>();
}

} // namespace board_view
