#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "Theme.hpp"
#include "UltimateGame.hpp"

#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

namespace board_view {
namespace {

using turn_based::MoveId;
using turn_based::Seat;
using turn_based::TurnBasedGame;
using ultimate_xo::Mark;
using ultimate_xo::Square;
using ultimate_xo::UltimateGame;

constexpr float cellSize = 46.f;
constexpr float cellGap = 3.f;
constexpr float boardGap = 16.f;
constexpr float boardSize = 3.f * cellSize + 2.f * cellGap;

// Nine small boards in a 3x3 arrangement (81 cells, numbered board * 9 + cell). The board the
// player must use is outlined; won boards carry a large mark, tied boards are dimmed.
class UltimateView final : public CellBoardView {
public:
    void layout(sf::FloatRect area) override
    {
        const float total = 3.f * boardSize + 2.f * boardGap;
        const float left = area.position.x + (area.size.x - total) / 2.f;
        const float top = area.position.y + (area.size.y - total) / 2.f;
        std::vector<sf::FloatRect> cells;
        cells.reserve(81);
        for (int board = 0; board < 9; ++board) {
            const sf::Vector2f origin{left + static_cast<float>(board % 3) * (boardSize + boardGap),
                                      top + static_cast<float>(board / 3) * (boardSize + boardGap)};
            boards_[static_cast<std::size_t>(board)] = {origin, {boardSize, boardSize}};
            for (int cell = 0; cell < 9; ++cell) {
                cells.push_back({{origin.x + static_cast<float>(cell % 3) * (cellSize + cellGap),
                                  origin.y + static_cast<float>(cell / 3) * (cellSize + cellGap)},
                                 {cellSize, cellSize}});
            }
        }
        const auto frame = frameAround(cells, 12.f);
        setCells(std::move(cells), frame);
    }

    std::string moveHint(const TurnBasedGame& game) const override
    {
        const std::string mark = game.currentSeat() == Seat::First ? "X" : "O";
        if (typed(game).forcedBoard()) return "place " + mark + " in the outlined board";
        return "place " + mark + " in any open board";
    }

    std::string controls() const override { return "Arrows choose a cell  |  Enter or click places"; }

    void reset(const TurnBasedGame& game) override
    {
        CellBoardView::reset(game);
        keepCursorPlayable(game);
    }

    void movePlayed(const TurnBasedGame& game, MoveId move) override
    {
        CellBoardView::movePlayed(game, move);
        keepCursorPlayable(game);
    }

protected:
    int initialCursor() const override { return UltimateGame::encode(4, 4); }

    std::optional<MoveId> moveForCell(const TurnBasedGame& game, int cell) const override
    {
        if (!game.isLegal(cell)) return std::nullopt;
        return cell;
    }

    std::string refusal(const TurnBasedGame& game, int cell) const override
    {
        const auto& board = typed(game);
        const int smallBoard = UltimateGame::boardOf(cell);
        if (board.square(smallBoard) != Square::Open) return "That small board is already decided.";
        if (const auto forced = board.forcedBoard(); forced && *forced != smallBoard) {
            return "You must play in the outlined board.";
        }
        return {};
    }

    bool cellHighlighted(const TurnBasedGame& game, int cell) const override
    {
        const auto& board = typed(game);
        const int smallBoard = UltimateGame::boardOf(cell);
        if (const auto& line = board.winningLine()) {
            if (std::find(line->begin(), line->end(), smallBoard) != line->end()) return true;
        }
        if (const auto line = board.smallBoardLine(smallBoard)) {
            const int inBoard = UltimateGame::cellOf(cell);
            return std::find(line->begin(), line->end(), inBoard) != line->end();
        }
        return false;
    }

    void drawCellContent(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                         const DrawState& state) const override
    {
        const Mark mark = typed(game).cell(UltimateGame::boardOf(cell), UltimateGame::cellOf(cell));
        if (mark == Mark::Empty) return;
        const bool closed = typed(game).square(UltimateGame::boardOf(cell)) != Square::Open;
        drawMark(target, mark == Mark::X, centerOf(rect), rect.size.x, placeScale(cell, state), closed ? 120 : 255);
    }

    void drawPreview(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                     const DrawState&) const override
    {
        if (!game.isLegal(cell)) return;
        drawMark(target, game.currentSeat() == Seat::First, centerOf(rect), rect.size.x, 1.f, 70);
    }

    void drawOverlay(sf::RenderTarget& target, const TurnBasedGame& game, const DrawState& state) const override
    {
        const auto& board = typed(game);
        const auto forced = board.forcedBoard();
        for (int index = 0; index < 9; ++index) {
            const auto& rect = boards_[static_cast<std::size_t>(index)];
            const Square square = board.square(index);
            const bool playable = state.interactive && square == Square::Open && (!forced || *forced == index);
            sf::RectangleShape outline({rect.size.x + 8.f, rect.size.y + 8.f});
            outline.setPosition({rect.position.x - 4.f, rect.position.y - 4.f});
            outline.setFillColor(square == Square::Tie ? withAlpha(Theme::background, 150) : sf::Color::Transparent);
            outline.setOutlineThickness(playable && forced ? 3.f : 1.f);
            outline.setOutlineColor(playable ? (forced ? Theme::primaryBright : Theme::primary) : Theme::border);
            target.draw(outline);
            if (square == Square::X || square == Square::O) {
                drawMark(target, square == Square::X, centerOf(rect), rect.size.x, 1.f, 200);
            }
        }
        if (const auto& line = board.winningLine()) {
            drawStrike(target, centerOf(boards_[static_cast<std::size_t>(line->front())]),
                       centerOf(boards_[static_cast<std::size_t>(line->back())]), 10.f, withAlpha(Theme::secondary, 220));
        }
    }

private:
    static const UltimateGame& typed(const TurnBasedGame& game) { return dynamic_cast<const UltimateGame&>(game); }

    // After a move the keyboard cursor may rest on a cell that cannot be played (a taken cell, a
    // decided board, or a board other than the one in play). It then moves to the same cell of the
    // board in play, or to the first cell that can be played. The pointer moves it back on hover.
    void keepCursorPlayable(const TurnBasedGame& game)
    {
        const auto& board = typed(game);
        if (board.outcome().finished() || board.isLegal(cursor())) return;
        const auto legal = board.legalMoves();
        if (legal.empty()) return;
        MoveId target = legal.front();
        if (const auto forced = board.forcedBoard()) {
            const MoveId same = UltimateGame::encode(*forced, UltimateGame::cellOf(cursor()));
            if (board.isLegal(same)) target = same;
        }
        setCursor(target);
    }

    std::array<sf::FloatRect, 9> boards_{};
};

} // namespace

std::unique_ptr<BoardView> makeUltimateView()
{
    return std::make_unique<UltimateView>();
}

} // namespace board_view
