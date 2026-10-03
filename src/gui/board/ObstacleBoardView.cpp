#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "ObstacleGame.hpp"
#include "Theme.hpp"

#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace board_view {
namespace {

using obstacle::Cell;
using obstacle::ObstacleGame;
using turn_based::MoveId;
using turn_based::Seat;
using turn_based::TurnBasedGame;

constexpr float dropSeconds = 0.3f;

// The 6x6 grid. Obstacles are dark, crossed-out cells; new ones settle in after the move that
// created them.
class ObstacleView final : public CellBoardView {
public:
    void layout(sf::FloatRect area) override
    {
        auto cells = gridCells(area, ObstacleGame::size, ObstacleGame::size, 70.f, 6.f);
        const auto frame = frameAround(cells, 12.f);
        setCells(std::move(cells), frame);
    }

    void reset(const TurnBasedGame& game) override
    {
        CellBoardView::reset(game);
        fresh_.clear();
        dropLeft_ = 0.f;
    }

    void movePlayed(const TurnBasedGame& game, MoveId move) override
    {
        CellBoardView::movePlayed(game, move);
        fresh_ = typed(game).lastObstacles();
        dropLeft_ = fresh_.empty() ? 0.f : dropSeconds;
    }

    void update(float seconds, bool reducedMotion) override
    {
        CellBoardView::update(seconds, reducedMotion);
        dropLeft_ = reducedMotion ? 0.f : std::max(0.f, dropLeft_ - seconds);
    }

    std::string moveHint(const TurnBasedGame& game) const override
    {
        std::string hint = std::string("place ") + (game.currentSeat() == Seat::First ? "X" : "O");
        if (game.movesPlayed() % 2 == 1) hint += "  |  two obstacles appear after this move";
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
        const Cell value = typed(game).cell(cell);
        if (value == Cell::Empty) return;
        if (value == Cell::Blocked) {
            const bool settling = dropLeft_ > 0.f && std::find(fresh_.begin(), fresh_.end(), cell) != fresh_.end();
            const float scale = settling ? 1.f - 0.5f * dropLeft_ / dropSeconds : 1.f;
            const sf::Vector2f size{(rect.size.x - 10.f) * scale, (rect.size.y - 10.f) * scale};
            sf::RectangleShape block(size);
            block.setOrigin({size.x / 2.f, size.y / 2.f});
            block.setPosition(centerOf(rect));
            block.setFillColor(Theme::background);
            block.setOutlineThickness(1.f);
            block.setOutlineColor(Theme::border);
            target.draw(block);
            // Parallel hatching (not a cross, so an obstacle never looks like an X mark).
            const auto centre = centerOf(rect);
            const float left = centre.x - size.x / 2.f + 6.f;
            const float top = centre.y - size.y / 2.f + 6.f;
            const float right = centre.x + size.x / 2.f - 6.f;
            const float bottom = centre.y + size.y / 2.f - 6.f;
            const float midX = (left + right) / 2.f;
            const float midY = (top + bottom) / 2.f;
            for (const auto& [from, to] : {std::pair<sf::Vector2f, sf::Vector2f>{{left, midY}, {midX, top}},
                                           std::pair<sf::Vector2f, sf::Vector2f>{{left, bottom}, {right, top}},
                                           std::pair<sf::Vector2f, sf::Vector2f>{{midX, bottom}, {right, midY}}}) {
                drawStrike(target, from, to, 3.f, Theme::textMuted);
            }
            return;
        }
        drawMark(target, value == Cell::X, centerOf(rect), rect.size.x, placeScale(cell, state));
    }

    void drawPreview(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                     const DrawState&) const override
    {
        if (typed(game).cell(cell) != Cell::Empty) return;
        drawMark(target, game.currentSeat() == Seat::First, centerOf(rect), rect.size.x, 1.f, 70);
    }

    void drawOverlay(sf::RenderTarget& target, const TurnBasedGame& game, const DrawState&) const override
    {
        const auto& line = typed(game).winningLine();
        if (!line || line->empty()) return;
        drawStrike(target, centerOf(cellRect(line->front())), centerOf(cellRect(line->back())), 6.f,
                   withAlpha(Theme::secondary, 210));
    }

private:
    static const ObstacleGame& typed(const TurnBasedGame& game) { return dynamic_cast<const ObstacleGame&>(game); }

    std::vector<int> fresh_;
    float dropLeft_{0.f};
};

} // namespace

std::unique_ptr<BoardView> makeObstacleView()
{
    return std::make_unique<ObstacleView>();
}

} // namespace board_view
