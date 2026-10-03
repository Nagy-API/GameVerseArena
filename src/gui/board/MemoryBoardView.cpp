#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "MemoryGame.hpp"
#include "Theme.hpp"

#include <algorithm>
#include <utility>

namespace board_view {
namespace {

using memory_xo::Mark;
using memory_xo::MemoryGame;
using turn_based::MoveId;
using turn_based::Seat;
using turn_based::TurnBasedGame;

constexpr float showSeconds = 1.2f;  // how long a new mark stays visible
constexpr float fadeSeconds = 0.4f;  // the last part of that time it fades out

// The 3x3 grid with every mark hidden. Each new mark shows briefly where it was placed (the console
// announced every move once) and then disappears; the whole board is revealed when the game ends.
// Hints never reveal whether a cell is taken: choosing a taken cell is simply refused.
class MemoryView final : public CellBoardView {
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
        newest_.reset();
        showLeft_ = 0.f;
    }

    void movePlayed(const TurnBasedGame& game, MoveId move) override
    {
        CellBoardView::movePlayed(game, move);
        newest_ = move;
        showLeft_ = showSeconds;
    }

    void update(float seconds, bool reducedMotion) override
    {
        CellBoardView::update(seconds, reducedMotion);
        showLeft_ = std::max(0.f, showLeft_ - seconds);
    }

    std::string moveHint(const TurnBasedGame& game) const override
    {
        return std::string("place ") + (game.currentSeat() == Seat::First ? "X" : "O") +
               "  |  every mark is hidden, so remember where they are";
    }

    std::string controls() const override { return "Arrows choose a cell  |  Enter or click places"; }

protected:
    std::optional<MoveId> moveForCell(const TurnBasedGame& game, int cell) const override
    {
        if (!game.isLegal(cell)) return std::nullopt;
        return cell;
    }

    std::string refusal(const TurnBasedGame& game, int) const override
    {
        if (game.outcome().finished()) return {};
        return "That cell is already taken. Choose another.";
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
        if (game.outcome().finished()) {
            drawMark(target, mark == Mark::X, centerOf(rect), rect.size.x, 1.f);
            return;
        }
        if (newest_ != cell || showLeft_ <= 0.f) return;
        // With Reduced Motion the mark shows at full strength and then simply disappears.
        const float strength = state.reducedMotion ? 1.f : std::min(1.f, showLeft_ / fadeSeconds);
        drawMark(target, mark == Mark::X, centerOf(rect), rect.size.x, placeScale(cell, state),
                 static_cast<std::uint8_t>(255.f * strength));
    }

    // Shown on every cell under the cursor, taken or not, so the preview gives nothing away.
    void drawPreview(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                     const DrawState&) const override
    {
        if (newest_ == cell && showLeft_ > 0.f) return;
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
    static const MemoryGame& typed(const TurnBasedGame& game) { return dynamic_cast<const MemoryGame&>(game); }

    std::optional<int> newest_;
    float showLeft_{0.f};
};

} // namespace

std::unique_ptr<BoardView> makeMemoryView()
{
    return std::make_unique<MemoryView>();
}

} // namespace board_view
