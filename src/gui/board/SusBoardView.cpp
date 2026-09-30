#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "SusGame.hpp"
#include "Theme.hpp"

#include <algorithm>
#include <utility>

namespace board_view {
namespace {

using turn_based::MoveId;
using turn_based::Seat;
using turn_based::TurnBasedGame;

class SusView final : public CellBoardView {
public:
    void layout(sf::FloatRect area) override
    {
        auto cells = gridCells(area, 3, 3, 124.f, 8.f);
        const auto frame = frameAround(cells, 12.f);
        setCells(std::move(cells), frame);
    }

    std::string moveHint(const TurnBasedGame& game) const override
    {
        return std::string("place ") + sus_game::SusGame::letterFor(game.currentSeat()) + " and spell S-U-S";
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
        for (const auto& line : typed(game).susLines()) {
            if (std::find(line.begin(), line.end(), cell) != line.end()) return true;
        }
        return false;
    }

    void drawCellContent(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                         const DrawState& state) const override
    {
        const char letter = typed(game).cell(cell);
        if (letter != 'S' && letter != 'U') return;
        const auto size = static_cast<unsigned int>(std::max(8.f, 76.f * placeScale(cell, state)));
        drawCenteredText(target, state.semiboldFont, std::string(1, letter), centerOf(rect), size,
                         letter == 'S' ? Theme::markX : Theme::markO);
    }

    void drawPreview(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                     const DrawState& state) const override
    {
        if (typed(game).cell(cell) != '.') return;
        const bool first = game.currentSeat() == Seat::First;
        drawCenteredText(target, state.semiboldFont, std::string(1, sus_game::SusGame::letterFor(game.currentSeat())),
                         centerOf(rect), 76, withAlpha(first ? Theme::markX : Theme::markO, 70));
    }

    void drawOverlay(sf::RenderTarget& target, const TurnBasedGame& game, const DrawState&) const override
    {
        for (const auto& line : typed(game).susLines()) {
            drawStrike(target, centerOf(cellRect(line.front())), centerOf(cellRect(line.back())), 6.f,
                       withAlpha(Theme::secondary, 170));
        }
    }

private:
    static const sus_game::SusGame& typed(const TurnBasedGame& game)
    {
        return dynamic_cast<const sus_game::SusGame&>(game);
    }
};

} // namespace

std::unique_ptr<BoardView> makeSusView()
{
    return std::make_unique<SusView>();
}

} // namespace board_view
