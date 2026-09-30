#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "FiveByFiveGame.hpp"
#include "Theme.hpp"

#include <utility>

namespace board_view {
namespace {

using five_by_five::FiveByFiveGame;
using five_by_five::Mark;
using turn_based::MoveId;
using turn_based::Seat;
using turn_based::TurnBasedGame;

class FiveByFiveView final : public CellBoardView {
public:
    void layout(sf::FloatRect area) override
    {
        auto cells = gridCells(area, 5, 5, 84.f, 6.f);
        const auto frame = frameAround(cells, 12.f);
        setCells(std::move(cells), frame);
    }

    std::string moveHint(const TurnBasedGame& game) const override
    {
        const int left = FiveByFiveGame::finalMoveCount - game.movesPlayed();
        return std::string("place ") + (game.currentSeat() == Seat::First ? "X" : "O") + "  |  " + std::to_string(left) +
               (left == 1 ? " move left" : " moves left");
    }

    std::string controls() const override { return "Arrows choose a cell  |  Enter or click places"; }

protected:
    std::optional<MoveId> moveForCell(const TurnBasedGame& game, int cell) const override
    {
        if (!game.isLegal(cell)) return std::nullopt;
        return cell;
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

    // Every scoring run of three, in its player's colour; overlapping runs each get a line.
    void drawOverlay(sf::RenderTarget& target, const TurnBasedGame& game, const DrawState&) const override
    {
        const auto& board = typed(game);
        for (const Mark mark : {Mark::X, Mark::O}) {
            const auto color = withAlpha(mark == Mark::X ? Theme::markX : Theme::markO, 150);
            for (const auto& triple : board.triplesOf(mark)) {
                drawStrike(target, centerOf(cellRect(triple.front())), centerOf(cellRect(triple.back())), 5.f, color);
            }
        }
    }

private:
    static const FiveByFiveGame& typed(const TurnBasedGame& game)
    {
        return dynamic_cast<const FiveByFiveGame&>(game);
    }
};

} // namespace

std::unique_ptr<BoardView> makeFiveByFiveView()
{
    return std::make_unique<FiveByFiveView>();
}

} // namespace board_view
