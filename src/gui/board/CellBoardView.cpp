#include "CellBoardView.hpp"

#include "Theme.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace board_view {

sf::Color withAlpha(sf::Color color, std::uint8_t alpha)
{
    color.a = alpha;
    return color;
}

sf::Vector2f centerOf(const sf::FloatRect& rect)
{
    return {rect.position.x + rect.size.x / 2.f, rect.position.y + rect.size.y / 2.f};
}

void drawCross(sf::RenderTarget& target, sf::Vector2f center, float size, const sf::Color& color)
{
    const float thickness = std::max(3.f, size * 0.11f);
    for (const float angle : {45.f, -45.f}) {
        sf::RectangleShape stroke({size, thickness});
        stroke.setOrigin({size / 2.f, thickness / 2.f});
        stroke.setPosition(center);
        stroke.setRotation(sf::degrees(angle));
        stroke.setFillColor(color);
        target.draw(stroke);
    }
}

void drawRing(sf::RenderTarget& target, sf::Vector2f center, float size, const sf::Color& color)
{
    const float thickness = std::max(3.f, size * 0.11f);
    const float radius = std::max(1.f, size / 2.f - thickness / 2.f);
    sf::CircleShape ring(radius);
    ring.setOrigin({radius, radius});
    ring.setPosition(center);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(thickness);
    ring.setOutlineColor(color);
    target.draw(ring);
}

void drawStrike(sf::RenderTarget& target, sf::Vector2f from, sf::Vector2f to, float thickness, const sf::Color& color)
{
    const sf::Vector2f delta = to - from;
    const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (length < 1.f) return;
    sf::RectangleShape strike({length + thickness, thickness});
    strike.setOrigin({thickness / 2.f, thickness / 2.f});
    strike.setPosition(from);
    strike.setRotation(sf::radians(std::atan2(delta.y, delta.x)));
    strike.setFillColor(color);
    target.draw(strike);
}

void drawCenteredText(sf::RenderTarget& target, const sf::Font& font, const std::string& text, sf::Vector2f center,
                      unsigned int size, const sf::Color& color)
{
    sf::Text label(font, sf::String::fromUtf8(text.begin(), text.end()), size);
    const auto bounds = label.getLocalBounds();
    label.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
    label.setPosition(center);
    label.setFillColor(color);
    target.draw(label);
}

void drawMark(sf::RenderTarget& target, bool cross, sf::Vector2f center, float cellSize, float scale, std::uint8_t alpha)
{
    if (cross) drawCross(target, center, cellSize * 0.64f * scale, withAlpha(Theme::markX, alpha));
    else drawRing(target, center, cellSize * 0.6f * scale, withAlpha(Theme::markO, alpha));
}

sf::FloatRect frameAround(const std::vector<sf::FloatRect>& cells, float padding)
{
    if (cells.empty()) return {};
    float left = cells.front().position.x;
    float top = cells.front().position.y;
    float right = left + cells.front().size.x;
    float bottom = top + cells.front().size.y;
    for (const auto& cell : cells) {
        left = std::min(left, cell.position.x);
        top = std::min(top, cell.position.y);
        right = std::max(right, cell.position.x + cell.size.x);
        bottom = std::max(bottom, cell.position.y + cell.size.y);
    }
    return {{left - padding, top - padding}, {right - left + 2.f * padding, bottom - top + 2.f * padding}};
}

std::vector<sf::FloatRect> gridCells(sf::FloatRect area, int columns, int rows, float cellSize, float gap)
{
    const float width = static_cast<float>(columns) * cellSize + static_cast<float>(columns - 1) * gap;
    const float height = static_cast<float>(rows) * cellSize + static_cast<float>(rows - 1) * gap;
    const float left = area.position.x + (area.size.x - width) / 2.f;
    const float top = area.position.y + (area.size.y - height) / 2.f;
    std::vector<sf::FloatRect> cells;
    cells.reserve(static_cast<std::size_t>(columns * rows));
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            cells.push_back({{left + static_cast<float>(column) * (cellSize + gap), top + static_cast<float>(row) * (cellSize + gap)},
                             {cellSize, cellSize}});
        }
    }
    return cells;
}

namespace {
// The nearest cell in the arrow's direction, preferring cells in line with the current one.
// Past the edge the cursor wraps to the far end of the same row or column.
int nextInDirection(const std::vector<sf::FloatRect>& cells, int from, int columns, int rows)
{
    if (cells.empty()) return from;
    const sf::Vector2f origin = centerOf(cells.at(static_cast<std::size_t>(from)));
    const auto stepOf = [&](const sf::FloatRect& cell) {
        const sf::Vector2f delta = centerOf(cell) - origin;
        const float along = delta.x * static_cast<float>(columns) + delta.y * static_cast<float>(rows);
        const float across = std::abs(columns != 0 ? delta.y : delta.x);
        return std::pair<float, float>{along, across};
    };
    int best = -1;
    float bestScore = std::numeric_limits<float>::max();
    for (int index = 0; index < static_cast<int>(cells.size()); ++index) {
        if (index == from) continue;
        const auto [along, across] = stepOf(cells[static_cast<std::size_t>(index)]);
        if (along <= 1.f) continue;
        const float score = along + across * 3.f;
        if (score < bestScore) { bestScore = score; best = index; }
    }
    if (best >= 0) return best;
    bestScore = std::numeric_limits<float>::max();
    for (int index = 0; index < static_cast<int>(cells.size()); ++index) {
        if (index == from) continue;
        const auto [along, across] = stepOf(cells[static_cast<std::size_t>(index)]);
        if (along >= -1.f) continue;
        const float score = across * 3.f + along;  // same line first, then the farthest back
        if (score < bestScore) { bestScore = score; best = index; }
    }
    return best >= 0 ? best : from;
}
} // namespace

void CellBoardView::setCells(std::vector<sf::FloatRect> cells, sf::FloatRect frame)
{
    cells_ = std::move(cells);
    frame_ = frame;
    cursor_ = std::clamp(cursor_, 0, std::max(0, cellCount() - 1));
}

std::optional<int> CellBoardView::cellAt(sf::Vector2f point) const
{
    for (int index = 0; index < cellCount(); ++index) {
        if (cells_[static_cast<std::size_t>(index)].contains(point)) return index;
    }
    return std::nullopt;
}

std::optional<sf::Vector2f> CellBoardView::cellCenter(int cell) const
{
    if (cell < 0 || cell >= cellCount()) return std::nullopt;
    return centerOf(cells_[static_cast<std::size_t>(cell)]);
}

float CellBoardView::placeScale(int cell, const DrawState& state) const noexcept
{
    if (state.reducedMotion || !lastCell_ || *lastCell_ != cell) return 1.f;
    return std::clamp(animation_, 0.05f, 1.f);
}

sf::Color CellBoardView::highlightColor() const
{
    return Theme::winningCell;
}

void CellBoardView::reset(const turn_based::TurnBasedGame&)
{
    cursor_ = std::clamp(initialCursor(), 0, std::max(0, cellCount() - 1));
    lastCell_.reset();
    animation_ = 1.f;
}

void CellBoardView::movePlayed(const turn_based::TurnBasedGame& game, turn_based::MoveId move)
{
    lastCell_ = cellOfMove(game, move);
    animation_ = 0.f;
}

void CellBoardView::update(float seconds, bool reducedMotion)
{
    animation_ = reducedMotion ? 1.f : std::min(1.f, animation_ + seconds * 5.5f);
}

Response CellBoardView::pointerMoved(sf::Vector2f point, const turn_based::TurnBasedGame&, bool)
{
    if (const auto cell = cellAt(point)) cursor_ = *cell;
    return {};
}

Response CellBoardView::pointerReleased(sf::Vector2f point, const turn_based::TurnBasedGame& game, bool interactive)
{
    const auto cell = cellAt(point);
    if (!cell) return {};
    cursor_ = *cell;
    if (!interactive) return {};
    return chooseCell(game, *cell);
}

Response CellBoardView::navigate(int columns, int rows, const turn_based::TurnBasedGame&, bool interactive)
{
    // The cursor is hidden while the player cannot move, so it stays put (and silent).
    if (!interactive) return {};
    const int next = nextInDirection(cells_, cursor_, columns, rows);
    if (next == cursor_) return {};
    cursor_ = next;
    return {std::nullopt, Response::Feedback::Focus};
}

Response CellBoardView::activate(const turn_based::TurnBasedGame& game, bool interactive)
{
    if (!interactive || cells_.empty()) return {};
    return chooseCell(game, cursor_);
}

Response CellBoardView::chooseCell(const turn_based::TurnBasedGame& game, int cell) const
{
    if (const auto move = moveForCell(game, cell)) return {move, Response::Feedback::None, {}};
    return {std::nullopt, Response::Feedback::Invalid, refusal(game, cell)};
}

void CellBoardView::draw(sf::RenderTarget& target, const turn_based::TurnBasedGame& game, const DrawState& state) const
{
    if (frame_.size.x > 0.f && frame_.size.y > 0.f) {
        sf::RectangleShape frame(frame_.size);
        frame.setPosition(frame_.position);
        frame.setFillColor(Theme::playfield);
        frame.setOutlineThickness(2.f);
        frame.setOutlineColor(Theme::border);
        target.draw(frame);
    }

    const float pulse = state.reducedMotion ? 0.5f : 0.5f + 0.5f * std::sin(state.time * 4.f);
    for (int index = 0; index < cellCount(); ++index) {
        const auto& rect = cells_[static_cast<std::size_t>(index)];
        const bool highlighted = cellHighlighted(game, index);
        const bool cursorHere = state.interactive && cursorCovers(index, cursor_);
        sf::RectangleShape tile(rect.size);
        tile.setPosition(rect.position);
        tile.setFillColor(highlighted ? highlightColor() : Theme::backgroundRaised);
        tile.setOutlineThickness(cursorHere ? 2.f : 1.f);
        tile.setOutlineColor(cursorHere ? Theme::primaryBright : Theme::border);
        target.draw(tile);
        if (cursorHere) {
            sf::RectangleShape glow(rect.size);
            glow.setPosition(rect.position);
            glow.setFillColor(withAlpha(Theme::primary, static_cast<std::uint8_t>(22 + 18 * pulse)));
            target.draw(glow);
        }
        drawCellContent(target, game, index, rect, state);
        if (cursorHere) drawPreview(target, game, index, rect, state);
    }
    drawOverlay(target, game, state);
}

} // namespace board_view
