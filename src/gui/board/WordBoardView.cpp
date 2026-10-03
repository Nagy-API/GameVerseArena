#include "BoardViews.hpp"
#include "CellBoardView.hpp"
#include "Theme.hpp"
#include "WordGame.hpp"

#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>
#include <cctype>
#include <utility>
#include <vector>

namespace board_view {
namespace {

using turn_based::MoveId;
using turn_based::Seat;
using turn_based::TurnBasedGame;
using word_ttt::WordGame;

constexpr float slotWidth = 36.f;
constexpr float slotHeight = 40.f;
constexpr float slotGap = 4.f;

// The 3x3 grid with an A-Z tray below it. A letter is chosen with the letter keys or the tray,
// then written into a cell.
class WordView final : public CellBoardView {
public:
    void layout(sf::FloatRect area) override
    {
        constexpr float cell = 104.f;
        constexpr float gap = 8.f;
        constexpr float board = 3.f * cell + 2.f * gap;
        constexpr float tray = 2.f * slotHeight + 6.f;
        constexpr float total = board + 24.f + 18.f + 22.f + tray;
        const float top = area.position.y + (area.size.y - total) / 2.f + 12.f;
        auto cells = gridCells({{area.position.x, top}, {area.size.x, board}}, 3, 3, cell, gap);
        const auto frame = frameAround(cells, 12.f);
        setCells(std::move(cells), frame);
        labelY_ = frame.position.y + frame.size.y + 18.f;
        trayTop_ = labelY_ + 22.f;
        centerX_ = area.position.x + area.size.x / 2.f;
    }

    void reset(const TurnBasedGame& game) override
    {
        CellBoardView::reset(game);
        selected_.reset();
        hovered_.reset();
    }

    void movePlayed(const TurnBasedGame& game, MoveId move) override
    {
        CellBoardView::movePlayed(game, move);
        selected_.reset();  // each player chooses their own letter
    }

    Response pointerMoved(sf::Vector2f point, const TurnBasedGame& game, bool interactive) override
    {
        hovered_ = letterAt(point, game);
        return CellBoardView::pointerMoved(point, game, interactive);
    }

    Response pointerReleased(sf::Vector2f point, const TurnBasedGame& game, bool interactive) override
    {
        if (const auto letter = letterAt(point, game)) {
            if (!interactive) return {};
            return choose(*letter);
        }
        return CellBoardView::pointerReleased(point, game, interactive);
    }

    Response character(char32_t character, const TurnBasedGame&, bool interactive) override
    {
        if (!interactive || character > 0x7F || !std::isalpha(static_cast<unsigned char>(character))) return {};
        return choose(static_cast<char>(std::toupper(static_cast<unsigned char>(character))));
    }

    bool cancelSelection() override
    {
        if (!selected_) return false;
        selected_.reset();
        return true;
    }

    std::string moveHint(const TurnBasedGame&) const override
    {
        if (!selected_) return "choose a letter (type A-Z or pick one below), then a cell";
        return std::string("write ") + *selected_ + " in a cell (type another letter to change it)";
    }

    std::string controls() const override
    {
        return "Letter keys or the tray choose a letter  |  Arrows choose a cell  |  Enter or click writes it";
    }

protected:
    std::optional<MoveId> moveForCell(const TurnBasedGame& game, int cell) const override
    {
        if (!selected_) return std::nullopt;
        const MoveId move = WordGame::encode(cell, *selected_);
        if (!game.isLegal(move)) return std::nullopt;
        return move;
    }

    std::string refusal(const TurnBasedGame& game, int cell) const override
    {
        if (!selected_ && typed(game).cell(cell) == '.') return "Choose a letter first: type A-Z or pick one below.";
        return {};
    }

    int cellOfMove(const TurnBasedGame&, MoveId move) const override { return WordGame::cellOf(move); }

    bool cellHighlighted(const TurnBasedGame& game, int cell) const override
    {
        const auto& line = typed(game).winningLine();
        return line && std::find(line->begin(), line->end(), cell) != line->end();
    }

    void drawCellContent(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                         const DrawState& state) const override
    {
        const char letter = typed(game).cell(cell);
        if (letter == '.') return;
        const auto size = static_cast<unsigned int>(std::max(8.f, 64.f * placeScale(cell, state)));
        drawCenteredText(target, state.semiboldFont, std::string(1, letter), centerOf(rect), size, Theme::textPrimary);
    }

    void drawPreview(sf::RenderTarget& target, const TurnBasedGame& game, int cell, const sf::FloatRect& rect,
                     const DrawState& state) const override
    {
        if (!selected_ || typed(game).cell(cell) != '.') return;
        const sf::Color color = game.currentSeat() == Seat::First ? Theme::markX : Theme::markO;
        drawCenteredText(target, state.semiboldFont, std::string(1, *selected_), centerOf(rect), 64, withAlpha(color, 90));
    }

    void drawOverlay(sf::RenderTarget& target, const TurnBasedGame& game, const DrawState& state) const override
    {
        const auto& board = typed(game);
        if (const auto& line = board.winningLine()) {
            drawStrike(target, centerOf(cellRect(line->front())), centerOf(cellRect(line->back())), 7.f,
                       withAlpha(Theme::secondary, 210));
        }
        if (game.outcome().finished()) return;
        const sf::Color seatColor = game.currentSeat() == Seat::First ? Theme::markX : Theme::markO;
        drawCenteredText(target, state.semiboldFont, "LETTERS", {centerX_, labelY_ + 8.f}, Theme::labelSize, seatColor);
        for (const auto& [letter, rect] : slots()) {
            const bool chosen = state.interactive && selected_ == letter;
            const bool hovered = state.interactive && hovered_ == letter;
            sf::RectangleShape slot(rect.size);
            slot.setPosition(rect.position);
            slot.setFillColor(chosen ? Theme::panelHover : Theme::panel);
            slot.setOutlineThickness(chosen ? 2.f : 1.f);
            slot.setOutlineColor(chosen ? Theme::primaryBright : hovered ? Theme::primary : Theme::border);
            target.draw(slot);
            drawCenteredText(target, state.semiboldFont, std::string(1, letter), centerOf(rect), 20,
                             chosen ? seatColor : Theme::textPrimary);
        }
    }

private:
    static const WordGame& typed(const TurnBasedGame& game) { return dynamic_cast<const WordGame&>(game); }

    std::vector<std::pair<char, sf::FloatRect>> slots() const
    {
        std::vector<std::pair<char, sf::FloatRect>> result;
        const float width = 13.f * slotWidth + 12.f * slotGap;
        for (int index = 0; index < 26; ++index) {
            const float x = centerX_ - width / 2.f + static_cast<float>(index % 13) * (slotWidth + slotGap);
            const float y = trayTop_ + static_cast<float>(index / 13) * (slotHeight + 6.f);
            result.push_back({static_cast<char>('A' + index), {{x, y}, {slotWidth, slotHeight}}});
        }
        return result;
    }

    std::optional<char> letterAt(sf::Vector2f point, const TurnBasedGame& game) const
    {
        if (game.outcome().finished()) return std::nullopt;
        for (const auto& [letter, rect] : slots()) {
            if (rect.contains(point)) return letter;
        }
        return std::nullopt;
    }

    Response choose(char letter)
    {
        const bool changed = selected_ != letter;
        selected_ = letter;
        return {std::nullopt, changed ? Response::Feedback::Select : Response::Feedback::None};
    }

    std::optional<char> selected_;
    std::optional<char> hovered_;
    float labelY_{};
    float trayTop_{};
    float centerX_{};
};

} // namespace

std::unique_ptr<BoardView> makeWordView()
{
    return std::make_unique<WordView>();
}

} // namespace board_view
