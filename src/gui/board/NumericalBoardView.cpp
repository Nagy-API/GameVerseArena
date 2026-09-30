#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "NumericalGame.hpp"
#include "Theme.hpp"

#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace board_view {
namespace {

using numerical_ttt::NumericalGame;
using turn_based::MoveId;
using turn_based::Seat;
using turn_based::TurnBasedGame;

constexpr float slotSize = 58.f;
constexpr float slotGap = 12.f;

// The board with a tray of the current player's numbers below it. A number is chosen with the
// number keys or by clicking the tray (the lowest unused number is preselected each turn), then
// placed on a cell.
class NumericalView final : public CellBoardView {
public:
    void layout(sf::FloatRect area) override
    {
        constexpr float cell = 112.f;
        constexpr float gap = 8.f;
        constexpr float board = 3.f * cell + 2.f * gap;
        constexpr float total = board + 24.f + 20.f + 24.f + slotSize;  // board, frame, spacing, label, tray
        const float top = area.position.y + (area.size.y - total) / 2.f + 12.f;
        auto cells = gridCells({{area.position.x, top}, {area.size.x, board}}, 3, 3, cell, gap);
        const auto frame = frameAround(cells, 12.f);
        setCells(std::move(cells), frame);
        labelY_ = frame.position.y + frame.size.y + 20.f;
        trayTop_ = labelY_ + 24.f;
        centerX_ = area.position.x + area.size.x / 2.f;
    }

    void reset(const TurnBasedGame& game) override
    {
        CellBoardView::reset(game);
        hovered_.reset();
        selectDefault(game);
    }

    void movePlayed(const TurnBasedGame& game, MoveId move) override
    {
        CellBoardView::movePlayed(game, move);
        selectDefault(game);
    }

    Response pointerMoved(sf::Vector2f point, const TurnBasedGame& game, bool interactive) override
    {
        hovered_ = numberAt(point, game);
        return CellBoardView::pointerMoved(point, game, interactive);
    }

    Response pointerReleased(sf::Vector2f point, const TurnBasedGame& game, bool interactive) override
    {
        if (const auto number = numberAt(point, game)) {
            if (!interactive) return {};
            return choose(game, *number);
        }
        return CellBoardView::pointerReleased(point, game, interactive);
    }

    Response character(char32_t character, const TurnBasedGame& game, bool interactive) override
    {
        if (!interactive || character < U'1' || character > U'9') return {};
        return choose(game, static_cast<int>(character - U'0'));
    }

    std::string moveHint(const TurnBasedGame&) const override
    {
        if (!selected_) return "choose a number";
        return "place " + std::to_string(*selected_) + " (number keys change it)";
    }

    std::string controls() const override
    {
        return "Number keys or the tray choose a number  |  Arrows choose a cell  |  Enter or click places";
    }

    std::string seatDetail(const TurnBasedGame& game, Seat seat) const override
    {
        const auto numbers = typed(game).availableNumbers(seat);
        if (numbers.empty()) return "No numbers left";
        std::string text = "Numbers left:";
        for (const int number : numbers) text += " " + std::to_string(number);
        return text;
    }

    int selectedNumber() const noexcept { return selected_.value_or(0); }

protected:
    std::optional<MoveId> moveForCell(const TurnBasedGame& game, int cell) const override
    {
        if (!selected_) return std::nullopt;
        const MoveId move = NumericalGame::encode(cell, *selected_);
        if (!game.isLegal(move)) return std::nullopt;
        return move;
    }

    int cellOfMove(const TurnBasedGame&, MoveId move) const override { return NumericalGame::cellOf(move); }

    bool cellHighlighted(const TurnBasedGame& game, int cell) const override
    {
        const auto& line = typed(game).winningLine();
        return line && std::find(line->begin(), line->end(), cell) != line->end();
    }

    void drawCellContent(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                         const DrawState& state) const override
    {
        const int number = typed(game).cell(cell);
        if (number == 0) return;
        const auto size = static_cast<unsigned int>(std::max(8.f, 58.f * placeScale(cell, state)));
        drawCenteredText(target, state.semiboldFont, std::to_string(number), centerOf(rect), size,
                         number % 2 == 1 ? Theme::markX : Theme::markO);
    }

    void drawPreview(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                     const DrawState& state) const override
    {
        if (!selected_ || typed(game).cell(cell) != 0) return;
        drawCenteredText(target, state.semiboldFont, std::to_string(*selected_), centerOf(rect), 58,
                         withAlpha(*selected_ % 2 == 1 ? Theme::markX : Theme::markO, 80));
    }

    void drawOverlay(sf::RenderTarget& target, const TurnBasedGame& game, const DrawState& state) const override
    {
        const auto& board = typed(game);
        if (const auto& line = board.winningLine()) {
            drawStrike(target, centerOf(cellRect(line->front())), centerOf(cellRect(line->back())), 7.f,
                       withAlpha(Theme::secondary, 210));
        }
        if (game.outcome().finished()) return;

        const Seat seat = game.currentSeat();
        const sf::Color seatColor = seat == Seat::First ? Theme::markX : Theme::markO;
        drawCenteredText(target, state.semiboldFont, seat == Seat::First ? "ODD NUMBERS" : "EVEN NUMBERS",
                         {centerX_, labelY_ + 9.f}, Theme::labelSize, seatColor);
        for (const auto& [number, rect] : slots(seat)) {
            const bool used = board.numberUsed(number);
            const bool chosen = state.interactive && selected_ == number;
            const bool hovered = state.interactive && hovered_ == number && !used;
            sf::RectangleShape slot(rect.size);
            slot.setPosition(rect.position);
            slot.setFillColor(used ? Theme::background : chosen ? Theme::panelHover : Theme::panel);
            slot.setOutlineThickness(chosen ? 2.f : 1.f);
            slot.setOutlineColor(chosen ? Theme::primaryBright : hovered ? Theme::primary : Theme::border);
            target.draw(slot);
            drawCenteredText(target, state.semiboldFont, std::to_string(number), centerOf(rect), 30,
                             used ? Theme::textMuted : seatColor);
            if (used) {
                drawStrike(target, {rect.position.x + 12.f, rect.position.y + rect.size.y - 12.f},
                           {rect.position.x + rect.size.x - 12.f, rect.position.y + 12.f}, 3.f, Theme::textMuted);
            }
        }
    }

private:
    static const NumericalGame& typed(const TurnBasedGame& game) { return dynamic_cast<const NumericalGame&>(game); }

    // The seat's whole pool (odd or even numbers), used ones included, with their tray slots.
    std::vector<std::pair<int, sf::FloatRect>> slots(Seat seat) const
    {
        std::vector<int> pool;
        for (int number = 1; number <= 9; ++number) {
            if (NumericalGame::numberBelongsTo(number, seat)) pool.push_back(number);
        }
        const float width = static_cast<float>(pool.size()) * slotSize + static_cast<float>(pool.size() - 1) * slotGap;
        float x = centerX_ - width / 2.f;
        std::vector<std::pair<int, sf::FloatRect>> result;
        for (const int number : pool) {
            result.push_back({number, {{x, trayTop_}, {slotSize, slotSize}}});
            x += slotSize + slotGap;
        }
        return result;
    }

    std::optional<int> numberAt(sf::Vector2f point, const TurnBasedGame& game) const
    {
        if (game.outcome().finished()) return std::nullopt;
        for (const auto& [number, rect] : slots(game.currentSeat())) {
            if (rect.contains(point)) return number;
        }
        return std::nullopt;
    }

    Response choose(const TurnBasedGame& game, int number)
    {
        const auto& board = typed(game);
        if (!NumericalGame::numberBelongsTo(number, game.currentSeat()) || board.numberUsed(number)) {
            return {std::nullopt, Response::Feedback::Invalid};
        }
        const bool changed = selected_ != number;
        selected_ = number;
        return {std::nullopt, changed ? Response::Feedback::Select : Response::Feedback::None};
    }

    void selectDefault(const TurnBasedGame& game)
    {
        selected_.reset();
        if (game.outcome().finished()) return;
        const auto numbers = typed(game).availableNumbers(game.currentSeat());
        if (!numbers.empty()) selected_ = numbers.front();
    }

    std::optional<int> selected_;
    std::optional<int> hovered_;
    float labelY_{};
    float trayTop_{};
    float centerX_{};
};

} // namespace

std::unique_ptr<BoardView> makeNumericalView()
{
    return std::make_unique<NumericalView>();
}

} // namespace board_view
