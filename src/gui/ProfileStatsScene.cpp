#include "ProfileStatsScene.hpp"

#include "GameCatalogue.hpp"
#include "TextLayout.hpp"
#include "Theme.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

namespace {
// Layout on the 1280 x 720 design canvas.
constexpr float tableLeft = 72.f;
constexpr float tableWidth = 716.f;
constexpr float headerY = 154.f;
constexpr float firstRowY = 180.f;
constexpr float rowStep = 38.f;
constexpr float rowHeight = 34.f;
constexpr float nameX = 92.f;
constexpr float nameWidth = 236.f;
constexpr float playedRight = 400.f;
constexpr float recordX = 432.f;
constexpr float rateRight = 616.f;
constexpr float lastPlayedX = 648.f;
constexpr float detailLeft = 820.f;
constexpr float detailTop = 150.f;
constexpr float detailWidth = 388.f;
constexpr float detailHeight = 406.f;
constexpr float detailTextWidth = 340.f;
constexpr unsigned int rowTextSize = 16;
constexpr unsigned int detailTitleSize = 22;
constexpr unsigned int detailTextSize = 17;

std::string rate(double value)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value * 100.0 << '%';
    return out.str();
}

std::string duration(std::int64_t ms)
{
    if (ms < 60000) return std::to_string(ms / 1000) + "s";
    const auto minutes = ms / 60000;
    const auto hours = minutes / 60;
    return hours > 0 ? std::to_string(hours) + "h " + std::to_string(minutes % 60) + "m"
                     : std::to_string(minutes) + "m";
}

std::string utc(const std::optional<std::int64_t>& value, const char* format)
{
    if (!value) return "Never";
    const std::time_t raw = static_cast<std::time_t>(*value / 1000);
    std::tm time{};
#ifdef _WIN32
    gmtime_s(&time, &raw);
#else
    gmtime_r(&raw, &time);
#endif
    std::ostringstream out;
    out << std::put_time(&time, format);
    return out.str();
}

std::string displayName(persistence::GameKey game)
{
    const auto* entry = catalogue::find(persistence::toStorage(game));
    return entry != nullptr ? entry->displayName : std::string(persistence::toStorage(game));
}

std::string pair(std::int64_t first, std::int64_t second)
{
    return std::to_string(first) + " / " + std::to_string(second);
}

// The lines every row's detail starts with.
std::string totals(const persistence::OverallStatistics& stats)
{
    return "Matches  " + std::to_string(stats.matches) +
           "\nWins / Losses / Draws  " + std::to_string(stats.wins) + " / " + std::to_string(stats.losses) + " / " +
           std::to_string(stats.draws) +
           "\nWin rate  " + rate(stats.winRate) +
           "\nPlay time  " + duration(stats.totalDurationMs) +
           "\nCurrent / best win streak  " + pair(stats.currentWinStreak, stats.bestWinStreak) +
           "\nLast played  " + utc(stats.lastPlayedAt, "%Y-%m-%d %H:%M UTC");
}

// Game-specific lines: the marks and formats of Classic Tic-Tac-Toe, Ping Pong's points, and the
// sides (and SUS / 5x5 points) of the shared board games.
std::string extras(persistence::GameKey game, const persistence::GameStatistics& stats)
{
    using persistence::GameKey;
    if (game == GameKey::ClassicTicTacToe) {
        return "\n\nAs X / O  " + pair(stats.ticTacToeAsX, stats.ticTacToeAsO) + "\nSingle / BO3 / BO5  " +
               std::to_string(stats.singleMatches) + " / " + std::to_string(stats.bestOfThreeMatches) + " / " +
               std::to_string(stats.bestOfFiveMatches);
    }
    std::string text;
    if (game == GameKey::PingPong || persistence::boardGameRecordsPoints(game)) {
        text += "\n\nPoints for / against  " + pair(stats.pointsScored, stats.pointsConceded) +
                "\nBest final margin  " + std::to_string(stats.bestFinalMargin);
    }
    if (const auto sides = persistence::boardGameSides(game)) {
        const std::string first = sides->first;
        const std::string label = first == "First" ? "Moved first / second  "
                                                   : "As " + first + " / " + sides->second + "  ";
        text += (text.empty() ? "\n\n" : "\n") + label + pair(stats.firstSideMatches, stats.secondSideMatches);
    }
    return text;
}

// Fits every line of a multi-line text to `width` (very large counts could otherwise leave the card).
std::string fitLines(const sf::Font& font, const std::string& text, unsigned int size, float width)
{
    std::string result;
    std::size_t start = 0;
    while (true) {
        const auto end = text.find('\n', start);
        result += fitToWidth(font, text.substr(start, end == std::string::npos ? std::string::npos : end - start), size, width);
        if (end == std::string::npos) break;
        result += '\n';
        start = end + 1;
    }
    return result;
}

void rightAlign(sf::Text& text, float right, float y)
{
    const auto bounds = text.getLocalBounds();
    text.setPosition({right - bounds.position.x - bounds.size.x, y});
}
} // namespace

ProfileStatsScene::ProfileStatsScene(AppContext& context,
    persistence::ProfileService& profiles, persistence::StatisticsRepository& statistics,
    persistence::AchievementService& achievements,
    std::int64_t& selectedProfileId, std::optional<persistence::GameKey>& selectedGame)
    : context_(context), scenes_(context.scenes), profiles_(profiles), statistics_(statistics),
      achievements_(achievements), selectedProfileId_(selectedProfileId), selectedGame_(selectedGame),
      regular_(context.regularFont), semibold_(context.semiboldFont),
      kicker_(context.semiboldFont, "PLAYER PROFILE", Theme::labelSize),
      title_(context.semiboldFont, "Statistics", Theme::pageTitleSize),
      subtitle_(context.regularFont, "", Theme::bodySize),
      achievementSummary_(context.semiboldFont, "", 18),
      error_(context.regularFont, "", 20),
      headers_{sf::Text(context.semiboldFont, "GAME", 13), sf::Text(context.semiboldFont, "PLAYED", 13),
               sf::Text(context.semiboldFont, "W-L-D", 13), sf::Text(context.semiboldFont, "WIN RATE", 13),
               sf::Text(context.semiboldFont, "LAST PLAYED", 13)},
      scrollStatus_(context.regularFont, "", 14),
      detailTitle_(context.semiboldFont, "", detailTitleSize),
      detailBody_(context.regularFont, "", detailTextSize),
      buttons_{UiButton(context.semiboldFont, "Recent Matches", {220.f, 54.f}),
               UiButton(context.semiboldFont, "Achievements", {220.f, 54.f}),
               UiButton(context.semiboldFont, "Back", {170.f, 54.f})}
{
    kicker_.setPosition({72.f, 32.f});
    kicker_.setFillColor(Theme::secondary);
    title_.setPosition({72.f, 57.f});
    title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({72.f, 110.f});
    subtitle_.setFillColor(Theme::textSecondary);
    achievementSummary_.setPosition({detailLeft, 110.f});
    achievementSummary_.setFillColor(Theme::warning);
    error_.setPosition({72.f, 260.f});
    error_.setFillColor(Theme::textSecondary);
    for (auto& header : headers_) header.setFillColor(Theme::textMuted);
    headers_[0].setPosition({nameX, headerY});
    rightAlign(headers_[1], playedRight, headerY);
    headers_[2].setPosition({recordX, headerY});
    rightAlign(headers_[3], rateRight, headerY);
    headers_[4].setPosition({lastPlayedX, headerY});
    scrollStatus_.setPosition({tableLeft, firstRowY + static_cast<float>(visibleRows) * rowStep + 6.f});
    scrollStatus_.setFillColor(Theme::textMuted);
    detailTitle_.setPosition({detailLeft + 24.f, detailTop + 24.f});
    detailTitle_.setFillColor(Theme::textPrimary);
    detailBody_.setPosition({detailLeft + 24.f, detailTop + 72.f});
    detailBody_.setFillColor(Theme::textPrimary);
    detailBody_.setLineSpacing(1.3f);
    buttons_[0].setPosition({746.f, 635.f});
    buttons_[1].setPosition({976.f, 635.f});
    buttons_[2].setPosition({72.f, 635.f});
    select(0);
}

void ProfileStatsScene::onActivate()
{
    for (auto& button : buttons_) button.setHovered(false);
    wheelRemainder_ = 0.f;
    std::vector<persistence::Profile> list;
    std::optional<persistence::Profile> active;
    try {
        list = profiles_.listProfiles();
        active = profiles_.activeProfile();
    } catch (const std::exception& error) {
        std::cerr << "GameVerseArenaGUI: profiles could not be loaded for statistics: " << error.what() << '\n';
        rows_.clear();
        loaded_ = false;
        achievementSummary_.setString("");
        error_.setString("Statistics could not be loaded.\nOther profile data is unaffected; details were logged.");
        select(0);
        return;
    }
    auto found = std::find_if(list.begin(), list.end(), [this](const auto& p) { return p.id == selectedProfileId_; });
    if (found == list.end()) {
        if (!active) {
            scenes_.switchTo(SceneId::Profiles);
            return;
        }
        selectedProfileId_ = active->id;
        found = std::find_if(list.begin(), list.end(), [this](const auto& p) { return p.id == selectedProfileId_; });
        if (found == list.end()) {
            scenes_.switchTo(SceneId::Profiles);
            return;
        }
    }
    // Another profile starts from its overall row; returning from its history or achievements
    // keeps the game that was selected.
    if (shownProfileId_ != selectedProfileId_) {
        selectedGame_.reset();
        shownProfileId_ = selectedProfileId_;
        scroll_ = 0;
    }
    subtitle_.setString(toDisplay(fitToWidth(regular_,
        found->displayName + (active && active->id == found->id ? "  |  ACTIVE" : ""), Theme::bodySize, 700.f)));

    rows_.clear();
    hoveredRow_.reset();
    loaded_ = false;
    std::vector<persistence::GameSummary> games;
    try {
        overall_ = statistics_.overall(selectedProfileId_);
        games = statistics_.perGame(selectedProfileId_);
        achievementSummary_.setString(
            "Achievements: " + std::to_string(achievements_.unlockedCount(selectedProfileId_)) + " / 12");
    } catch (const std::exception& error) {
        std::cerr << "GameVerseArenaGUI: statistics could not be loaded: " << error.what() << '\n';
        achievementSummary_.setString("");
        error_.setString("Statistics could not be loaded.\nOther profile data is unaffected; details were logged.");
        scrollStatus_.setString("");
        selectedRow_ = 0;
        scroll_ = 0;
        select(0);
        return;
    }
    error_.setString("");
    loaded_ = true;

    const auto addRow = [this](std::optional<persistence::GameKey> game, std::string name,
                               const persistence::GameSummary& summary, const sf::Font& nameFont) {
        Row row{game, std::move(name), summary,
                {sf::Text(nameFont, "", rowTextSize), sf::Text(regular_, "", rowTextSize),
                 sf::Text(regular_, "", rowTextSize), sf::Text(regular_, "", rowTextSize),
                 sf::Text(regular_, "", rowTextSize)}};
        const bool played = summary.matches > 0;
        row.cells[0].setString(toDisplay(fitToWidth(nameFont, row.name, rowTextSize, nameWidth)));
        row.cells[1].setString(std::to_string(summary.matches));
        // Fitted to the room left of the right-aligned win rate, so very large counts cannot overlap it.
        row.cells[2].setString(fitToWidth(regular_, std::to_string(summary.wins) + "-" + std::to_string(summary.losses) +
                                                        "-" + std::to_string(summary.draws),
                                          rowTextSize, rateRight - 64.f - recordX));
        row.cells[3].setString(played ? rate(summary.winRate) : "-");
        row.cells[4].setString(utc(summary.lastPlayedAt, "%Y-%m-%d"));
        row.cells[0].setFillColor(played ? Theme::textPrimary : Theme::textSecondary);
        for (std::size_t cell = 1; cell < row.cells.size(); ++cell) {
            row.cells[cell].setFillColor(played ? Theme::textSecondary : Theme::textMuted);
        }
        rows_.push_back(std::move(row));
    };
    persistence::GameSummary everything;
    everything.matches = overall_.matches;
    everything.wins = overall_.wins;
    everything.losses = overall_.losses;
    everything.draws = overall_.draws;
    everything.winRate = overall_.winRate;
    everything.totalDurationMs = overall_.totalDurationMs;
    everything.lastPlayedAt = overall_.lastPlayedAt;
    addRow(std::nullopt, "All games", everything, semibold_);
    for (const auto& game : games) addRow(game.game, displayName(game.game), game, regular_);

    const auto chosen = std::find_if(rows_.begin(), rows_.end(), [this](const Row& row) { return row.game == selectedGame_; });
    selectedRow_ = chosen == rows_.end() ? 0 : static_cast<std::size_t>(chosen - rows_.begin());
    selectedGame_ = rows_[selectedRow_].game;
    const std::size_t maximum = rows_.size() > visibleRows ? rows_.size() - visibleRows : 0;
    scroll_ = std::min(scroll_, maximum);
    if (selectedRow_ < scroll_) scroll_ = selectedRow_;
    if (selectedRow_ >= scroll_ + visibleRows) scroll_ = selectedRow_ + 1 - visibleRows;
    layoutRows();
    showDetail();
    select(0);
}

void ProfileStatsScene::layoutRows()
{
    for (std::size_t index = 0; index < rows_.size(); ++index) {
        if (index < scroll_ || index >= scroll_ + visibleRows) continue;
        const float y = firstRowY + static_cast<float>(index - scroll_) * rowStep + 7.f;
        auto& cells = rows_[index].cells;
        cells[0].setPosition({nameX, y});
        rightAlign(cells[1], playedRight, y);
        cells[2].setPosition({recordX, y});
        rightAlign(cells[3], rateRight, y);
        cells[4].setPosition({lastPlayedX, y});
    }
    const std::size_t first = rows_.empty() ? 0 : scroll_ + 1;
    const std::size_t last = std::min(rows_.size(), scroll_ + visibleRows);
    scrollStatus_.setString("Showing " + std::to_string(first) + "-" + std::to_string(last) + " of " +
                            std::to_string(rows_.size()) + "  |  Up / Down chooses a game  |  Mouse wheel scrolls");
}

void ProfileStatsScene::showDetail()
{
    if (rows_.empty()) return;
    const auto& row = rows_[selectedRow_];
    detailTitle_.setString(toDisplay(fitToWidth(semibold_, row.name, detailTitleSize, detailTextWidth)));
    const auto message = [this](const std::string& text) {
        return wrapToWidth(regular_, text, detailTextSize, detailTextWidth, 6);
    };
    std::string body;
    if (!row.game) {
        if (overall_.matches == 0) {
            body = message("No matches yet. Play any game from the library to build your history.");
        } else {
            const auto played = std::count_if(rows_.begin() + 1, rows_.end(),
                                              [](const Row& game) { return game.summary.matches > 0; });
            body = totals(overall_) + "\n\nGames played  " + std::to_string(played) + " of " +
                   std::to_string(rows_.size() - 1);
        }
    } else {
        persistence::GameStatistics stats;
        try {
            stats = statistics_.forGame(selectedProfileId_, *row.game);
        } catch (const std::exception& error) {
            std::cerr << "GameVerseArenaGUI: game statistics could not be loaded: " << error.what() << '\n';
            detailText_ = message("These details could not be loaded; the problem was logged.");
            detailBody_.setString(toDisplay(detailText_));
            return;
        }
        body = stats.matches == 0
            ? message("No matches yet. Play " + row.name + " from the library to start its history.")
            : totals(stats) + extras(*row.game, stats);
    }
    detailText_ = fitLines(regular_, body, detailTextSize, detailTextWidth);
    detailBody_.setString(toDisplay(detailText_));
}

std::optional<std::size_t> ProfileStatsScene::rowAt(sf::Vector2f point) const
{
    if (!loaded_ || point.x < tableLeft || point.x > tableLeft + tableWidth || point.y < firstRowY) return std::nullopt;
    const float offset = point.y - firstRowY;
    const auto slot = static_cast<std::size_t>(offset / rowStep);
    if (slot >= visibleRows || offset - static_cast<float>(slot) * rowStep > rowHeight) return std::nullopt;
    const std::size_t row = scroll_ + slot;
    if (row >= rows_.size()) return std::nullopt;
    return row;
}

void ProfileStatsScene::selectRow(std::size_t row, bool withSound)
{
    if (rows_.empty()) return;
    row = std::min(row, rows_.size() - 1);
    const bool changed = row != selectedRow_;
    if (withSound && changed) context_.play(audio::SoundId::UiFocus);
    selectedRow_ = row;
    selectedGame_ = rows_[row].game;
    const auto before = scroll_;
    if (selectedRow_ < scroll_) scroll_ = selectedRow_;
    if (selectedRow_ >= scroll_ + visibleRows) scroll_ = selectedRow_ + 1 - visibleRows;
    if (scroll_ != before) hoveredRow_.reset();
    layoutRows();
    if (changed) showDetail();
}

void ProfileStatsScene::scrollBy(int rows)
{
    const std::size_t maximum = rows_.size() > visibleRows ? rows_.size() - visibleRows : 0;
    const auto before = scroll_;
    if (rows < 0) scroll_ = scroll_ > static_cast<std::size_t>(-rows) ? scroll_ - static_cast<std::size_t>(-rows) : 0;
    else scroll_ = std::min(maximum, scroll_ + static_cast<std::size_t>(rows));
    if (scroll_ != before) {
        hoveredRow_.reset();  // the pointer is over a different row now; the next move finds it
        layoutRows();
    }
}

void ProfileStatsScene::select(std::size_t index, bool withSound)
{
    if (withSound && index != selected_) context_.play(audio::SoundId::UiFocus);
    selected_ = index;
    for (std::size_t i = 0; i < buttons_.size(); ++i) buttons_[i].setSelected(i == selected_);
}

void ProfileStatsScene::activate(std::size_t index)
{
    context_.play(index == 2 ? audio::SoundId::UiBack : audio::SoundId::UiConfirm);
    if (index == 0) scenes_.switchTo(SceneId::MatchHistory);  // filtered to the selected game
    else if (index == 1) scenes_.switchTo(SceneId::ProfileAchievements);
    else scenes_.switchTo(SceneId::Profiles);
}

void ProfileStatsScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        using Key = sf::Keyboard::Key;
        const std::size_t last = rows_.empty() ? 0 : rows_.size() - 1;
        switch (key->code) {
        case Key::Escape: activate(2); break;
        case Key::Right: select((selected_ + 1) % buttons_.size(), true); break;
        case Key::Left: select((selected_ + buttons_.size() - 1) % buttons_.size(), true); break;
        case Key::Tab:
            select(key->shift ? (selected_ + buttons_.size() - 1) % buttons_.size() : (selected_ + 1) % buttons_.size(),
                   true);
            break;
        // Always through selectRow, so even at the first or last row the selection scrolls back into view.
        case Key::Up: selectRow(selectedRow_ > 0 ? selectedRow_ - 1 : 0, true); break;
        case Key::Down: selectRow(std::min(last, selectedRow_ + 1), true); break;
        case Key::PageUp: selectRow(selectedRow_ > visibleRows ? selectedRow_ - visibleRows : 0, true); break;
        case Key::PageDown: selectRow(std::min(last, selectedRow_ + visibleRows), true); break;
        case Key::Home: selectRow(0, true); break;
        case Key::End: selectRow(last, true); break;
        case Key::Enter:
        case Key::Space: activate(selected_); break;
        default: break;
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
        if (rows != 0) scrollBy(-rows);
        return;
    }
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        hoveredRow_ = rowAt(point);
        for (std::size_t i = 0; i < buttons_.size(); ++i) {
            buttons_[i].setHovered(buttons_[i].contains(point));
            if (buttons_[i].contains(point)) select(i, true);
        }
        return;
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        if (const auto row = rowAt(point)) {
            selectRow(*row, true);
            return;
        }
        for (std::size_t i = 0; i < buttons_.size(); ++i) {
            if (buttons_[i].contains(point)) {
                activate(i);
                return;
            }
        }
    }
}

void ProfileStatsScene::update(sf::Time deltaTime)
{
    for (auto& button : buttons_) button.update(deltaTime, context_.reducedMotion());
}

void ProfileStatsScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_);
    window.draw(title_);
    window.draw(subtitle_);
    window.draw(achievementSummary_);
    if (!loaded_) {
        window.draw(error_);
        for (const auto& button : buttons_) button.draw(window);
        return;
    }
    for (const auto& header : headers_) window.draw(header);
    const auto end = std::min(rows_.size(), scroll_ + visibleRows);
    for (std::size_t index = scroll_; index < end; ++index) {
        const float y = firstRowY + static_cast<float>(index - scroll_) * rowStep;
        const bool chosen = index == selectedRow_;
        sf::RectangleShape background({tableWidth, rowHeight});
        background.setPosition({tableLeft, y});
        background.setFillColor(chosen ? Theme::panelHover : hoveredRow_ == index ? Theme::panel : Theme::backgroundRaised);
        background.setOutlineThickness(chosen ? 2.f : 1.f);
        background.setOutlineColor(chosen ? Theme::primaryBright : Theme::border);
        window.draw(background);
        if (chosen) {
            sf::RectangleShape accent({5.f, rowHeight});
            accent.setPosition({tableLeft, y});
            accent.setFillColor(Theme::secondary);
            window.draw(accent);
        }
        for (const auto& cell : rows_[index].cells) window.draw(cell);
    }
    if (rows_.size() > visibleRows) {
        // Scroll bar: the thumb shows which part of the table is visible.
        const float trackHeight = static_cast<float>(visibleRows) * rowStep - (rowStep - rowHeight);
        sf::RectangleShape track({4.f, trackHeight});
        track.setPosition({tableLeft + tableWidth + 8.f, firstRowY});
        track.setFillColor(Theme::border);
        window.draw(track);
        const float share = static_cast<float>(visibleRows) / static_cast<float>(rows_.size());
        sf::RectangleShape thumb({4.f, trackHeight * share});
        thumb.setPosition({tableLeft + tableWidth + 8.f,
                           firstRowY + trackHeight * static_cast<float>(scroll_) / static_cast<float>(rows_.size())});
        thumb.setFillColor(Theme::primaryBright);
        window.draw(thumb);
    }
    window.draw(scrollStatus_);

    sf::RectangleShape card({detailWidth, detailHeight});
    card.setPosition({detailLeft, detailTop});
    card.setFillColor(Theme::backgroundRaised);
    card.setOutlineThickness(1.f);
    card.setOutlineColor(Theme::border);
    window.draw(card);
    window.draw(detailTitle_);
    window.draw(detailBody_);
    for (const auto& button : buttons_) button.draw(window);
}
