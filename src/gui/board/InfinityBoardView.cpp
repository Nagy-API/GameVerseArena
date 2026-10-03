#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "InfinityGame.hpp"
#include "Theme.hpp"

#include <algorithm>
#include <utility>

namespace board_view {
namespace {

using infinity_xo::InfinityGame;
using infinity_xo::Mark;
using turn_based::MoveId;
using turn_based::Seat;
using turn_based::TurnBasedGame;

constexpr float vanishSeconds = 0.6f;

// The 3x3 grid. The mark that the next move will remove is dimmed, and a removed mark fades out.
class InfinityView final : public CellBoardView {
public:
    void layout(sf::FloatRect area) override
    {
        auto cells = gridCells(area, 3, 3, 124.f, 8.f);
        const auto frame = frameAround(cells, 12.f);
        setCells(std::move(cells), frame);
    }

    void reset(const TurnBasedGame& game) override
    {
        CellBoardView::reset(game);
        vanished_.reset();
        vanishLeft_ = 0.f;
    }

    void movePlayed(const TurnBasedGame& game, MoveId move) override
    {
        CellBoardView::movePlayed(game, move);
        const auto& board = typed(game);
        vanished_.reset();
        if (const auto removed = board.lastRemoved()) {
            vanished_ = std::make_pair(*removed, board.lastRemovedMark());
            vanishLeft_ = vanishSeconds;
        }
    }

    void update(float seconds, bool reducedMotion) override
    {
        CellBoardView::update(seconds, reducedMotion);
        vanishLeft_ = reducedMotion ? 0.f : std::max(0.f, vanishLeft_ - seconds);
    }

    std::string moveHint(const TurnBasedGame& game) const override
    {
        std::string hint = std::string("place ") + (game.currentSeat() == Seat::First ? "X" : "O");
        if (typed(game).nextToVanish()) hint += "  |  the dimmed mark vanishes after this move";
        return hint;
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
        const auto& line = typed(game).winningLine();
        return line && std::find(line->begin(), line->end(), cell) != line->end();
    }

    void drawCellContent(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                         const DrawState& state) const override
    {
        const auto& board = typed(game);
        const Mark mark = board.cell(cell);
        if (mark == Mark::Empty) {
            // A mark that just vanished fades out where it stood.
            if (vanished_ && vanished_->first == cell && vanishLeft_ > 0.f) {
                const auto alpha = static_cast<std::uint8_t>(200.f * vanishLeft_ / vanishSeconds);
                drawMark(target, vanished_->second == Mark::X, centerOf(rect), rect.size.x, 1.f, alpha);
            }
            return;
        }
        const bool fading = board.nextToVanish() == cell;
        drawMark(target, mark == Mark::X, centerOf(rect), rect.size.x, placeScale(cell, state), fading ? 105 : 255);
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
        drawStrike(target, centerOf(cellRect(line->front())), centerOf(cellRect(line->back())), 8.f,
                   withAlpha(Theme::secondary, 210));
    }

private:
    static const InfinityGame& typed(const TurnBasedGame& game) { return dynamic_cast<const InfinityGame&>(game); }

    std::optional<std::pair<int, Mark>> vanished_;
    float vanishLeft_{0.f};
};

} // namespace

std::unique_ptr<BoardView> makeInfinityView()
{
    return std::make_unique<InfinityView>();
}

} // namespace board_view
