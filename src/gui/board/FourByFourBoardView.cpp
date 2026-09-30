#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "FourByFourGame.hpp"
#include "Theme.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>
#include <utility>

namespace board_view {
namespace {

using four_by_four::FourByFourGame;
using four_by_four::Token;
using turn_based::MoveId;
using turn_based::Seat;
using turn_based::TurnBasedGame;

// The 4 x 4 grid with two-step moves: pick one of your tokens, then the empty neighbouring cell to
// slide it to. Picking another of your tokens switches the pick; Escape drops it.
class FourByFourView final : public CellBoardView {
public:
    void layout(sf::FloatRect area) override
    {
        auto cells = gridCells(area, 4, 4, 104.f, 8.f);
        const auto frame = frameAround(cells, 12.f);
        setCells(std::move(cells), frame);
    }

    void reset(const TurnBasedGame& game) override
    {
        CellBoardView::reset(game);
        picked_.reset();
    }

    void movePlayed(const TurnBasedGame& game, MoveId move) override
    {
        CellBoardView::movePlayed(game, move);
        picked_.reset();
    }

    Response pointerReleased(sf::Vector2f point, const TurnBasedGame& game, bool interactive) override
    {
        const auto cell = cellAt(point);
        if (!cell) return {};
        setCursor(*cell);
        if (!interactive) return {};
        return choose(game, *cell);
    }

    Response activate(const TurnBasedGame& game, bool interactive) override
    {
        if (!interactive) return {};
        return choose(game, cursor());
    }

    bool cancelSelection() override
    {
        if (!picked_) return false;
        picked_.reset();
        return true;
    }

    std::string moveHint(const TurnBasedGame& game) const override
    {
        const std::string token = game.currentSeat() == Seat::First ? "X" : "O";
        if (picked_) return "slide the " + token + " token to a marked cell";
        return "choose an " + token + " token to slide";
    }

    std::string controls() const override
    {
        return "Arrows choose a cell  |  Enter or click picks a token, then its new cell  |  Esc drops the pick";
    }

protected:
    int initialCursor() const override { return 5; }

    // Moves come from choose(); a single cell never makes a move on its own.
    std::optional<MoveId> moveForCell(const TurnBasedGame&, int) const override { return std::nullopt; }

    int cellOfMove(const TurnBasedGame&, MoveId move) const override { return FourByFourGame::toOf(move); }

    bool cellHighlighted(const TurnBasedGame& game, int cell) const override
    {
        const auto& line = typed(game).winningLine();
        return line && std::find(line->begin(), line->end(), cell) != line->end();
    }

    void drawCellContent(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                         const DrawState& state) const override
    {
        const Token token = typed(game).cell(cell);
        if (token == Token::Empty) return;
        drawMark(target, token == Token::X, centerOf(rect), rect.size.x, placeScale(cell, state));
    }

    void drawOverlay(sf::RenderTarget& target, const TurnBasedGame& game, const DrawState& state) const override
    {
        const auto& board = typed(game);
        if (state.interactive && picked_) {
            // The picked token and the cells it can slide to.
            const auto& rect = cellRect(*picked_);
            sf::RectangleShape outline({rect.size.x - 6.f, rect.size.y - 6.f});
            outline.setPosition({rect.position.x + 3.f, rect.position.y + 3.f});
            outline.setFillColor(sf::Color::Transparent);
            outline.setOutlineThickness(3.f);
            outline.setOutlineColor(Theme::secondary);
            target.draw(outline);
            for (const int destination : board.destinationsFrom(*picked_)) {
                sf::CircleShape dot(9.f);
                dot.setOrigin({9.f, 9.f});
                dot.setPosition(centerOf(cellRect(destination)));
                dot.setFillColor(withAlpha(Theme::secondary, 200));
                target.draw(dot);
            }
        }
        if (const auto& line = board.winningLine()) {
            drawStrike(target, centerOf(cellRect(line->front())), centerOf(cellRect(line->back())), 7.f,
                       withAlpha(Theme::secondary, 210));
        }
    }

private:
    static const FourByFourGame& typed(const TurnBasedGame& game) { return dynamic_cast<const FourByFourGame&>(game); }

    Response choose(const TurnBasedGame& game, int cell)
    {
        const auto& board = typed(game);
        if (board.cell(cell) == FourByFourGame::tokenFor(game.currentSeat())) {
            if (board.destinationsFrom(cell).empty()) return {std::nullopt, Response::Feedback::Invalid};
            if (picked_ == cell) {
                picked_.reset();
            } else {
                picked_ = cell;
            }
            return {std::nullopt, Response::Feedback::Select};
        }
        if (picked_) {
            const auto destinations = board.destinationsFrom(*picked_);
            if (std::find(destinations.begin(), destinations.end(), cell) != destinations.end()) {
                return {FourByFourGame::encode(*picked_, cell), Response::Feedback::None};
            }
        }
        return {std::nullopt, Response::Feedback::Invalid};
    }

    std::optional<int> picked_;
};

} // namespace

std::unique_ptr<BoardView> makeFourByFourView()
{
    return std::make_unique<FourByFourView>();
}

} // namespace board_view
