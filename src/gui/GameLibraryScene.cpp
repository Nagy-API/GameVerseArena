#include "GameLibraryScene.hpp"

#include "TextLayout.hpp"
#include "Theme.hpp"
#include "Utf8Text.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <cmath>

using audio::SoundId;

namespace {
constexpr float gridLeft = 72.f;
constexpr float gridTop = 196.f;
constexpr float cardWidth = 368.f;
constexpr float cardHeight = 128.f;
constexpr float columnSpacing = 384.f;
constexpr float rowSpacing = 142.f;
constexpr std::size_t maximumQueryLength = 24;
constexpr unsigned int smallTextSize = 14;
constexpr unsigned int badgeTextSize = 13;

const std::array<sf::FloatRect, 3> categoryBounds{
    sf::FloatRect{{512.f, 128.f}, {110.f, 48.f}},
    sf::FloatRect{{632.f, 128.f}, {130.f, 48.f}},
    sf::FloatRect{{772.f, 128.f}, {140.f, 48.f}}};
const sf::FloatRect backBounds{{72.f, 626.f}, {170.f, 52.f}};

const char* categoryName(std::size_t category)
{
    if (category == 1) return "Board";
    if (category == 2) return "Arcade";
    return "All";
}

sf::String display(const std::string& utf8)
{
    return sf::String::fromUtf8(utf8.begin(), utf8.end());
}
} // namespace

GameLibraryScene::GameLibraryScene(AppContext& context, GameLauncher& launcher)
    : context_(context), launcher_(launcher),
      kicker_(context.semiboldFont, "GAME LIBRARY", Theme::labelSize),
      title_(context.semiboldFont, "Choose your game", Theme::pageTitleSize),
      summary_(context.regularFont, "", 16),
      searchLabel_(context.regularFont, "Type to search games", 18),
      searchText_(context.regularFont, "", 18),
      emptyState_(context.regularFont, "", 20),
      scrollStatus_(context.regularFont, "", Theme::labelSize),
      help_(context.regularFont,
            "Type to search  |  Tab moves between search, filters, games, and Back  |  Arrows choose  |  Enter plays",
            Theme::labelSize),
      message_(context.regularFont, "", Theme::labelSize),
      searchField_({420.f, 48.f}),
      categoryButtons_{UiButton(context.semiboldFont, "All", categoryBounds[0].size),
                       UiButton(context.semiboldFont, "Board", categoryBounds[1].size),
                       UiButton(context.semiboldFont, "Arcade", categoryBounds[2].size)},
      backButton_(context.semiboldFont, "Back", backBounds.size)
{
    kicker_.setPosition({72.f, 36.f});
    kicker_.setFillColor(Theme::secondary);
    title_.setPosition({72.f, 60.f});
    title_.setFillColor(Theme::textPrimary);
    summary_.setFillColor(Theme::textSecondary);
    searchField_.setPosition({72.f, 128.f});
    searchField_.setFillColor(Theme::backgroundRaised);
    searchLabel_.setPosition({92.f, 140.f});
    searchLabel_.setFillColor(Theme::textMuted);
    searchText_.setPosition({92.f, 140.f});
    searchText_.setFillColor(Theme::textPrimary);
    for (std::size_t index = 0; index < categoryButtons_.size(); ++index) {
        categoryButtons_[index].setPosition(categoryBounds[index].position);
    }
    emptyState_.setPosition({72.f, 260.f});
    emptyState_.setFillColor(Theme::textSecondary);
    emptyState_.setLineSpacing(1.3f);
    backButton_.setPosition(backBounds.position);
    message_.setPosition({268.f, 624.f});
    message_.setFillColor(Theme::warning);
    scrollStatus_.setPosition({268.f, 652.f});
    scrollStatus_.setFillColor(Theme::textMuted);
    help_.setPosition({72.f, 692.f});
    help_.setFillColor(Theme::textMuted);
    applyFilter();
}

std::size_t GameLibraryScene::rowCount() const
{
    return (results_.size() + columns - 1) / columns;
}

std::size_t GameLibraryScene::maximumFirstRow() const
{
    return rowCount() > visibleRows ? rowCount() - visibleRows : 0;
}

sf::FloatRect GameLibraryScene::cardBounds(std::size_t index) const
{
    const auto row = static_cast<float>(index / columns) - static_cast<float>(firstRow_);
    const auto column = static_cast<float>(index % columns);
    return {{gridLeft + column * columnSpacing, gridTop + row * rowSpacing}, {cardWidth, cardHeight}};
}

std::optional<std::size_t> GameLibraryScene::cardAt(sf::Vector2f point) const
{
    for (std::size_t row = firstRow_; row < firstRow_ + visibleRows; ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            const std::size_t index = row * columns + column;
            if (index < results_.size() && cardBounds(index).contains(point)) return index;
        }
    }
    return std::nullopt;
}

void GameLibraryScene::applyFilter()
{
    std::optional<catalogue::GameCategory> category;
    if (category_ == 1) category = catalogue::GameCategory::Board;
    else if (category_ == 2) category = catalogue::GameCategory::Arcade;
    results_ = catalogue::filter(query_, category);
    hovered_.reset();
    if (selected_ >= results_.size()) selected_ = results_.empty() ? 0 : results_.size() - 1;
    firstRow_ = std::min(firstRow_, maximumFirstRow());
    ensureSelectionVisible();

    summary_.setString(std::to_string(results_.size()) + " of " + std::to_string(catalogue::all().size()) +
                       " games shown  |  " + std::to_string(catalogue::playableCount()) + " playable here");
    const auto bounds = summary_.getLocalBounds();
    summary_.setPosition({1208.f - bounds.size.x - bounds.position.x, 82.f});
    emptyState_.setString(results_.empty()
        ? display("No games match \"" + query_ + "\" in " + categoryName(category_) +
                  ".\nPress Escape to clear the search, or choose All.")
        : sf::String());
    for (std::size_t index = 0; index < categoryButtons_.size(); ++index) {
        categoryButtons_[index].setSelected(index == category_);
    }
    backButton_.setSelected(zone_ == Zone::Back);
}

void GameLibraryScene::setZone(Zone zone, bool withSound)
{
    if (withSound && zone != zone_) context_.play(SoundId::UiFocus);
    zone_ = zone;
    backButton_.setSelected(zone_ == Zone::Back);
}

void GameLibraryScene::selectCard(std::size_t index, bool withSound)
{
    if (results_.empty()) return;
    index = std::min(index, results_.size() - 1);
    if (withSound && (index != selected_ || zone_ != Zone::Grid)) context_.play(SoundId::UiFocus);
    selected_ = index;
    zone_ = Zone::Grid;
    backButton_.setSelected(false);
    ensureSelectionVisible();
}

void GameLibraryScene::ensureSelectionVisible()
{
    const std::size_t previousFirstRow = firstRow_;
    if (results_.empty()) {
        firstRow_ = 0;
    } else {
        const std::size_t row = selected_ / columns;
        if (row < firstRow_) firstRow_ = row;
        if (row >= firstRow_ + visibleRows) firstRow_ = row - visibleRows + 1;
        firstRow_ = std::min(firstRow_, maximumFirstRow());
    }
    // Cards moved under the pointer; the hover outline returns with the next mouse move.
    if (firstRow_ != previousFirstRow) hovered_.reset();
    const std::size_t firstShown = results_.empty() ? 0 : firstRow_ * columns + 1;
    const std::size_t lastShown = std::min(results_.size(), (firstRow_ + visibleRows) * columns);
    scrollStatus_.setString(rowCount() > visibleRows
        ? "Showing " + std::to_string(firstShown) + "-" + std::to_string(lastShown) + " of " +
              std::to_string(results_.size()) + "  |  Mouse wheel or Page Up / Page Down scrolls"
        : std::to_string(results_.size()) + (results_.size() == 1 ? " game" : " games"));
}

void GameLibraryScene::moveInGrid(int dx, int dy)
{
    if (results_.empty()) {
        setZone(dy < 0 ? Zone::Categories : Zone::Back, true);
        return;
    }
    const int row = static_cast<int>(selected_ / columns);
    const int column = static_cast<int>(selected_ % columns);
    const int lastIndex = static_cast<int>(results_.size()) - 1;
    if (dy < 0 && row == 0) { setZone(Zone::Categories, true); return; }
    if (dy > 0 && row == static_cast<int>(rowCount()) - 1) { setZone(Zone::Back, true); return; }
    int target = static_cast<int>(selected_);
    if (dx != 0) target = std::clamp(target + dx, 0, lastIndex);
    if (dy != 0) target = std::clamp((row + dy) * static_cast<int>(columns) + column, 0, lastIndex);
    selectCard(static_cast<std::size_t>(target), true);
}

void GameLibraryScene::moveByPage(int pages)
{
    if (results_.empty()) return;
    // Page keys move the focus a screenful at a time, so repeated presses reach both ends.
    const int step = pages * static_cast<int>(visibleRows * columns);
    const int target = std::clamp(static_cast<int>(selected_) + step, 0, static_cast<int>(results_.size()) - 1);
    selectCard(static_cast<std::size_t>(target), true);
}

void GameLibraryScene::scrollRows(int rows)
{
    const int next = std::clamp(static_cast<int>(firstRow_) + rows, 0, static_cast<int>(maximumFirstRow()));
    firstRow_ = static_cast<std::size_t>(next);
    hovered_.reset();
    if (!results_.empty()) {
        // Keep the keyboard selection on screen after a mouse-wheel scroll.
        const std::size_t row = selected_ / columns;
        if (row < firstRow_ || row >= firstRow_ + visibleRows) {
            const std::size_t targetRow = row < firstRow_ ? firstRow_ : firstRow_ + visibleRows - 1;
            selected_ = std::min(results_.size() - 1, targetRow * columns + selected_ % columns);
        }
    }
    ensureSelectionVisible();
}

void GameLibraryScene::setCategory(std::size_t category)
{
    if (category == category_) return;
    category_ = category;
    selected_ = 0;
    firstRow_ = 0;
    message_.setString("");
    applyFilter();
    context_.play(SoundId::UiConfirm);
}

void GameLibraryScene::activate()
{
    switch (zone_) {
    case Zone::Search:
    case Zone::Categories:
        if (!results_.empty()) selectCard(selected_, true);
        else context_.play(SoundId::UiError);
        break;
    case Zone::Grid:
        if (!results_.empty()) launch(selected_);
        break;
    case Zone::Back:
        goBack();
        break;
    }
}

void GameLibraryScene::launch(std::size_t index)
{
    if (index >= results_.size()) return;
    const auto& game = *results_[index];
    selected_ = index;
    if (launcher_.canLaunch(game)) {
        message_.setString("");
        context_.play(SoundId::UiConfirm);
        launcher_.launch(game);
        return;
    }
    context_.play(SoundId::UiError);
    if (game.launch == catalogue::LaunchKind::ConsoleOnly && game.consoleMenuNumber != 0) {
        message_.setString(display(game.displayName + " is playable in the console app; its graphical version is "
                                                       "not available yet."));
    } else {
        message_.setString(display(game.displayName + " could not be started."));
    }
}

void GameLibraryScene::goBack()
{
    context_.play(SoundId::UiBack);
    context_.scenes.switchTo(SceneId::MainMenu);
}

void GameLibraryScene::editQuery(char32_t codepoint)
{
    if (codepoint < 32 || codepoint == 127) return;
    if (zone_ != Zone::Search) {
        if (codepoint == U' ') return;  // Space activates the focused control outside the search box.
        setZone(Zone::Search, false);
    }
    if (!utf8_text::appendPrintable(query_, codepoint, maximumQueryLength)) {
        context_.play(SoundId::UiError);
        return;
    }
    selected_ = 0;
    firstRow_ = 0;
    message_.setString("");
    applyFilter();
}

void GameLibraryScene::eraseQuery()
{
    if (query_.empty()) return;
    setZone(Zone::Search, false);
    utf8_text::eraseLast(query_);
    selected_ = 0;
    firstRow_ = 0;
    message_.setString("");
    applyFilter();
}

void GameLibraryScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* text = event.getIf<sf::Event::TextEntered>()) {
        editQuery(text->unicode);
        return;
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        using Key = sf::Keyboard::Key;
        if (key->code == Key::Escape) {
            if (!query_.empty()) {
                query_.clear();
                selected_ = 0;
                firstRow_ = 0;
                message_.setString("");
                applyFilter();
                context_.play(SoundId::UiBack);
            } else {
                goBack();
            }
            return;
        }
        if (key->code == Key::Backspace) {
            eraseQuery();
            return;
        }
        if (key->code == Key::Tab) {
            static constexpr std::array<Zone, 4> order{Zone::Search, Zone::Categories, Zone::Grid, Zone::Back};
            const int step = key->shift ? 3 : 1;
            int next = static_cast<int>(std::find(order.begin(), order.end(), zone_) - order.begin());
            do {
                next = (next + step) % 4;
            } while (order[static_cast<std::size_t>(next)] == Zone::Grid && results_.empty());
            setZone(order[static_cast<std::size_t>(next)], true);
            if (zone_ == Zone::Grid) ensureSelectionVisible();
            return;
        }
        switch (zone_) {
        case Zone::Search:
            if (key->code == Key::Enter || key->code == Key::Down) activate();
            break;
        case Zone::Categories:
            if (key->code == Key::Left) setCategory((category_ + 2) % 3);
            else if (key->code == Key::Right) setCategory((category_ + 1) % 3);
            else if (key->code == Key::Up) setZone(Zone::Search, true);
            else if (key->code == Key::Down || key->code == Key::Enter || key->code == Key::Space) activate();
            break;
        case Zone::Grid:
            if (key->code == Key::Left) moveInGrid(-1, 0);
            else if (key->code == Key::Right) moveInGrid(1, 0);
            else if (key->code == Key::Up) moveInGrid(0, -1);
            else if (key->code == Key::Down) moveInGrid(0, 1);
            else if (key->code == Key::PageUp) moveByPage(-1);
            else if (key->code == Key::PageDown) moveByPage(1);
            else if (key->code == Key::Home) selectCard(0, true);
            else if (key->code == Key::End && !results_.empty()) selectCard(results_.size() - 1, true);
            else if (key->code == Key::Enter || key->code == Key::Space) activate();
            break;
        case Zone::Back:
            if (key->code == Key::Up) setZone(results_.empty() ? Zone::Categories : Zone::Grid, true);
            else if (key->code == Key::Enter || key->code == Key::Space) activate();
            break;
        }
        return;
    }

    if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (wheel->wheel != sf::Mouse::Wheel::Vertical || wheel->delta == 0.f) return;
        // Precision touchpads send fractional steps; accumulate them into whole rows.
        if ((wheelRemainder_ > 0.f) != (wheel->delta > 0.f)) wheelRemainder_ = 0.f;
        wheelRemainder_ += wheel->delta;
        const int rows = static_cast<int>(wheelRemainder_);
        wheelRemainder_ -= static_cast<float>(rows);
        if (rows != 0) scrollRows(-rows);
        return;
    }

    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        hovered_ = cardAt(point);
        // Hovering highlights a card but never pulls focus out of the search box being typed in.
        if (hovered_ && zone_ != Zone::Search) selectCard(*hovered_, true);
        hoveredCategory_.reset();
        for (std::size_t index = 0; index < categoryBounds.size(); ++index) {
            if (categoryBounds[index].contains(point)) hoveredCategory_ = index;
        }
        backButton_.setHovered(backBounds.contains(point));
        return;
    }

    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        if (searchField_.getGlobalBounds().contains(point)) {
            setZone(Zone::Search, true);
            return;
        }
        for (std::size_t index = 0; index < categoryBounds.size(); ++index) {
            if (categoryBounds[index].contains(point)) {
                setZone(Zone::Categories, false);
                setCategory(index);
                return;
            }
        }
        if (backBounds.contains(point)) {
            goBack();
            return;
        }
        if (const auto index = cardAt(point)) launch(*index);
    }
}

void GameLibraryScene::update(sf::Time deltaTime)
{
    const bool reduced = context_.reducedMotion();
    for (auto& button : categoryButtons_) button.update(deltaTime, reduced);
    backButton_.update(deltaTime, reduced);
}

const std::string& GameLibraryScene::cachedText(const std::string& cacheKey, const std::string& source, bool semibold,
                                                unsigned int size, float width, std::size_t lines) const
{
    auto found = textCache_.find(cacheKey);
    if (found == textCache_.end()) {
        const sf::Font& font = semibold ? context_.semiboldFont : context_.regularFont;
        const std::string fitted = lines > 1 ? wrapToWidth(font, source, size, width, lines)
                                             : fitToWidth(font, source, size, width);
        found = textCache_.emplace(cacheKey, fitted).first;
    }
    return found->second;
}

void GameLibraryScene::drawRing(sf::RenderTarget& target, sf::FloatRect bounds, const sf::Color& color) const
{
    sf::RectangleShape ring(bounds.size + sf::Vector2f{8.f, 8.f});
    ring.setPosition(bounds.position - sf::Vector2f{4.f, 4.f});
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(3.f);
    ring.setOutlineColor(color);
    target.draw(ring);
}

void GameLibraryScene::drawCard(sf::RenderTarget& target, std::size_t index) const
{
    const auto& game = *results_[index];
    const auto bounds = cardBounds(index);
    const bool focused = zone_ == Zone::Grid && index == selected_;
    const bool hovered = hovered_ && *hovered_ == index;
    const bool playable = game.playableInGui();

    sf::RectangleShape card(bounds.size);
    card.setPosition(bounds.position);
    card.setFillColor(focused ? Theme::panelHover : Theme::backgroundRaised);
    card.setOutlineThickness(focused ? 3.f : hovered ? 2.f : 1.f);
    card.setOutlineColor(focused ? Theme::primaryBright : hovered ? Theme::textMuted : Theme::border);
    target.draw(card);
    if (focused) {
        sf::RectangleShape accent({6.f, bounds.size.y});
        accent.setPosition(bounds.position);
        accent.setFillColor(Theme::secondary);
        target.draw(accent);
    }

    const float x = bounds.position.x + 20.f;
    const float y = bounds.position.y;
    sf::Text name(context_.semiboldFont, display(cachedText(game.key + ":name", game.displayName, true, 22, 250.f, 1)), 22);
    name.setPosition({x, y + 12.f});
    name.setFillColor(playable ? Theme::textPrimary : Theme::textSecondary);
    target.draw(name);

    const bool arcade = game.category == catalogue::GameCategory::Arcade;
    sf::Text badge(context_.semiboldFont, arcade ? "ARCADE" : "BOARD", badgeTextSize);
    const auto badgeBounds = badge.getLocalBounds();
    badge.setPosition({bounds.position.x + bounds.size.x - 20.f - badgeBounds.size.x, y + 19.f});
    badge.setFillColor(arcade ? Theme::arcadeRight : Theme::secondary);
    target.draw(badge);

    sf::Text description(context_.regularFont,
                         display(cachedText(game.key + ":description", game.shortDescription, false, 15, 328.f, 2)), 15);
    description.setPosition({x, y + 46.f});
    description.setFillColor(Theme::textSecondary);
    description.setLineSpacing(1.1f);
    target.draw(description);

    sf::Text availability(context_.semiboldFont, playable ? "PLAYABLE" : "CONSOLE ONLY", badgeTextSize);
    const auto availabilityBounds = availability.getLocalBounds();
    availability.setPosition({bounds.position.x + bounds.size.x - 20.f - availabilityBounds.size.x, y + 101.f});
    availability.setFillColor(playable ? Theme::secondary : Theme::warning);
    target.draw(availability);

    std::string modes;
    if (game.humanVsHuman && game.humanVsComputer) modes = "2 players / vs AI";
    else if (game.humanVsComputer) modes = "vs AI";
    else modes = "2 players";
    // The footer never runs under the availability badge, whatever the board summary length.
    const float footerWidth = bounds.size.x - 40.f - availabilityBounds.size.x - 14.f;
    sf::Text footer(context_.regularFont,
                    display(cachedText(game.key + (playable ? ":footer-playable" : ":footer"),
                                       game.boardSummary + " | " + modes, false, smallTextSize, footerWidth, 1)),
                    smallTextSize);
    footer.setPosition({x, y + 100.f});
    footer.setFillColor(Theme::textMuted);
    target.draw(footer);
}

void GameLibraryScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_);
    window.draw(title_);
    window.draw(summary_);

    sf::RectangleShape field = searchField_;
    field.setOutlineThickness(zone_ == Zone::Search ? 3.f : 1.f);
    field.setOutlineColor(zone_ == Zone::Search ? Theme::primaryBright : Theme::border);
    window.draw(field);
    if (query_.empty() && zone_ != Zone::Search) {
        window.draw(searchLabel_);
    } else {
        sf::Text text = searchText_;
        const std::string caret = zone_ == Zone::Search ? " |" : "";
        text.setString(display(fitTailToWidth(context_.regularFont, query_, 18, 360.f) + caret));
        window.draw(text);
    }

    for (const auto& button : categoryButtons_) button.draw(window);
    if (hoveredCategory_ && *hoveredCategory_ != category_) drawRing(window, categoryBounds[*hoveredCategory_], Theme::textMuted);
    if (zone_ == Zone::Categories) drawRing(window, categoryBounds[category_], Theme::primaryBright);

    const std::size_t end = std::min(results_.size(), (firstRow_ + visibleRows) * columns);
    for (std::size_t index = firstRow_ * columns; index < end; ++index) drawCard(window, index);

    if (rowCount() > visibleRows) {
        const float trackHeight = static_cast<float>(visibleRows) * rowSpacing - 14.f;
        sf::RectangleShape track({4.f, trackHeight});
        track.setPosition({1216.f, gridTop});
        track.setFillColor(Theme::panel);
        window.draw(track);
        const float fraction = static_cast<float>(visibleRows) / static_cast<float>(rowCount());
        const float offset = static_cast<float>(firstRow_) / static_cast<float>(rowCount());
        sf::RectangleShape thumb({4.f, trackHeight * fraction});
        thumb.setPosition({1216.f, gridTop + trackHeight * offset});
        thumb.setFillColor(Theme::primaryBright);
        window.draw(thumb);
    }

    window.draw(emptyState_);
    backButton_.draw(window);
    if (zone_ == Zone::Back) drawRing(window, backBounds, Theme::primaryBright);
    window.draw(message_);
    window.draw(scrollStatus_);
    window.draw(help_);
}

void GameLibraryScene::onResize(sf::Vector2u) {}

void GameLibraryScene::onActivate()
{
    hovered_.reset();
    hoveredCategory_.reset();
    wheelRemainder_ = 0.f;
    message_.setString("");
    backButton_.setHovered(false);
    zone_ = Zone::Grid;
    applyFilter();
    // A kept search with no results leaves nothing to focus in the grid; focus the search box.
    if (results_.empty()) setZone(Zone::Search, false);
}
