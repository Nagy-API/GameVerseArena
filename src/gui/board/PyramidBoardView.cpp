#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "PyramidGame.hpp"
#include "Theme.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace board_view {
namespace {

using pyramid::Mark;
using pyramid::PyramidGame;
using turn_based::MoveId;
using turn_based::Seat;
using turn_based::TurnBasedGame;

// Five cells on the bottom row, three centred above them, and one at the top, so the pyramid's
// sloped edges and centre column line up on screen.
class PyramidView final : public CellBoardView {
public:
    void layout(sf::FloatRect area) override
    {
        constexpr float cell = 96.f;
        constexpr float step = cell + 10.f;
        const float left = area.position.x + (area.size.x - (4.f * step + cell)) / 2.f;
        const float top = area.position.y + (area.size.y - (2.f * step + cell)) / 2.f;
        std::vector<sf::FloatRect> cells;
        for (int index = 0; index < PyramidGame::cellCount; ++index) {
            const int row = PyramidGame::rowOf(index);            // 0 = bottom
            const int first = row == 0 ? 0 : row == 1 ? 5 : 8;     // first cell of the row
            const float column = static_cast<float>(index - first + row);  // rows are indented by one step per level
            cells.push_back({{left + column * step, top + static_cast<float>(2 - row) * step}, {cell, cell}});
        }
        setCells(std::move(cells), {});  // no rectangular backdrop behind the pyramid
    }

    std::string moveHint(const TurnBasedGame& game) const override
    {
        return std::string("place ") + (game.currentSeat() == Seat::First ? "X" : "O");
    }

    std::string controls() const override { return "Arrows choose a cell  |  Enter or click places"; }

protected:
    int initialCursor() const override { return 2; }

    std::optional<MoveId> moveForCell(const TurnBasedGame& game, int cell) const override
    {
        if (!game.isLegal(cell)) return std::nullopt;
        return cell;
    }

    bool cellHighlighted(const TurnBasedGame& game, int cell) const override
    {
        const auto& line = typed(game).winningLine();
        return line && std::find(line->begin(), line->end(), cell) != line->end();
    }

    void drawCellContent(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                         const DrawState& state) const override
    {
        const Mark mark = typed(game).cell(cell);
        if (mark == Mark::Empty) return;
        drawMark(target, mark == Mark::X, centerOf(rect), rect.size.x, placeScale(cell, state));
    }

    void drawPreview(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                     const DrawState&) const override
    {
        if (typed(game).cell(cell) != Mark::Empty) return;
        drawMark(target, game.currentSeat() == Seat::First, centerOf(rect), rect.size.x, 1.f, 70);
    }

    void drawOverlay(sf::RenderTarget& target, const TurnBasedGame& game, const DrawState&) const override
    {
        const auto& line = typed(game).winningLine();
        if (!line) return;
        drawStrike(target, centerOf(cellRect(line->front())), centerOf(cellRect(line->back())), 7.f,
                   withAlpha(Theme::secondary, 210));
    }

private:
    static const PyramidGame& typed(const TurnBasedGame& game) { return dynamic_cast<const PyramidGame&>(game); }
};

} // namespace

std::unique_ptr<BoardView> makePyramidView()
{
    return std::make_unique<PyramidView>();
}

} // namespace board_view
