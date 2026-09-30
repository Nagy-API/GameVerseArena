#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "MisereGame.hpp"
#include "Theme.hpp"

#include <algorithm>
#include <utility>

namespace board_view {
namespace {

using turn_based::MoveId;
using turn_based::TurnBasedGame;

class MisereView final : public CellBoardView {
public:
    void layout(sf::FloatRect area) override
    {
        auto cells = gridCells(area, 3, 3, 124.f, 8.f);
        const auto frame = frameAround(cells, 12.f);
        setCells(std::move(cells), frame);
    }

    std::string moveHint(const TurnBasedGame& game) const override
    {
        return std::string("place ") + (game.currentSeat() == turn_based::Seat::First ? "X" : "O") +
               " without completing a line";
    }

    std::string controls() const override { return "Arrows choose a cell  |  Enter or click places"; }

protected:
    std::optional<MoveId> moveForCell(const TurnBasedGame& game, int cell) const override
    {
        if (!game.isLegal(cell)) return std::nullopt;
        return cell;
    }

    bool cellHighlighted(const TurnBasedGame& game, int cell) const override
    {
        const auto& line = typed(game).losingLine();
        return line && std::find(line->begin(), line->end(), cell) != line->end();
    }

    // The completed line lost the game, so it is marked in red rather than the winning teal.
    sf::Color highlightColor() const override { return {86, 38, 50}; }

    void drawCellContent(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                         const DrawState& state) const override
    {
        const auto mark = typed(game).cell(cell);
        if (mark == misere::Mark::Empty) return;
        drawMark(target, mark == misere::Mark::X, centerOf(rect), rect.size.x, placeScale(cell, state));
    }

    void drawPreview(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                     const DrawState&) const override
    {
        if (typed(game).cell(cell) != misere::Mark::Empty) return;
        drawMark(target, game.currentSeat() == turn_based::Seat::First, centerOf(rect), rect.size.x, 1.f, 70);
    }

    void drawOverlay(sf::RenderTarget& target, const TurnBasedGame& game, const DrawState&) const override
    {
        const auto& line = typed(game).losingLine();
        if (!line) return;
        drawStrike(target, centerOf(cellRect(line->front())), centerOf(cellRect(line->back())), 8.f,
                   withAlpha(Theme::danger, 220));
    }

private:
    static const misere::MisereGame& typed(const TurnBasedGame& game)
    {
        return dynamic_cast<const misere::MisereGame&>(game);
    }
};

} // namespace

std::unique_ptr<BoardView> makeMisereView()
{
    return std::make_unique<MisereView>();
}

} // namespace board_view
