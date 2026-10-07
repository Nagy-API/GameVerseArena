#include "MatchHistoryScene.hpp"

#include "TextLayout.hpp"
#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
std::string date(std::int64_t ms)
{
    std::time_t raw = static_cast<std::time_t>(ms / 1000);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &raw);
#else
    gmtime_r(&raw, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%d %H:%M UTC");
    return out.str();
}

// Short game names for the rows and the filter button.
std::string game(persistence::GameKey value)
{
    using persistence::GameKey;
    switch (value) {
    case GameKey::ClassicTicTacToe: return "Tic-Tac-Toe";
    case GameKey::NumericalTicTacToe: return "Numerical";
    case GameKey::Sus: return "SUS";
    case GameKey::FiveByFiveTicTacToe: return "5x5";
    case GameKey::MisereTicTacToe: return "Misere";
    case GameKey::FourInARow: return "Four-in-a-Row";
    case GameKey::FourByFourTicTacToe: return "4x4";
    case GameKey::WordTicTacToe: return "Word";
    case GameKey::PyramidTicTacToe: return "Pyramid";
    case GameKey::Diamond: return "Diamond";
    case GameKey::InfinityXo: return "Infinity XO";
    case GameKey::UltimateXo: return "Ultimate XO";
    case GameKey::MemoryXo: return "Memory XO";
    case GameKey::ObstacleTicTacToe: return "Obstacle";
    case GameKey::PingPong: return "Ping Pong";
    }
    return "Game";
}

std::string result(persistence::MatchResult value)
{
    return value == persistence::MatchResult::Win ? "WIN" : value == persistence::MatchResult::Loss ? "LOSS" : "DRAW";
}

std::string mode(const persistence::CompletedMatch& match)
{
    if (match.mode != persistence::MatchMode::HumanVsComputer) return "vs Human";
    // Board games have one computer strategy ("standard"); only the others have a level to show.
    if (match.difficulty == persistence::DifficultyKey::Standard) return "vs Computer";
    return "vs Computer / " + std::string(persistence::toStorage(match.difficulty));
}

constexpr std::array<const char*, 4> resultLabels{"Result: All", "Result: Win", "Result: Loss", "Result: Draw"};
} // namespace

MatchHistoryScene::MatchHistoryScene(AppContext& context, persistence::ProfileService& profiles,
    persistence::MatchRepository& matches, std::int64_t& selectedProfileId,
    std::optional<persistence::GameKey>& selectedGame)
    : context_(context), scenes_(context.scenes), profiles_(profiles), matches_(matches),
      profileId_(selectedProfileId), selectedGame_(selectedGame),
      title_(context.semiboldFont, "Match History", Theme::pageTitleSize),
      subtitle_(context.regularFont, "", Theme::bodySize), status_(context.regularFont, "", 14),
      empty_(context.regularFont, "", 20),
      help_(context.regularFont, "Up / Down or click: change the focused filter", 14),
      buttons_{UiButton(context.semiboldFont, "Game: All", {290.f, 48.f}),
               UiButton(context.semiboldFont, "Result: All", {210.f, 48.f}),
               UiButton(context.semiboldFont, "Previous", {160.f, 48.f}),
               UiButton(context.semiboldFont, "Next", {160.f, 48.f}),
               UiButton(context.semiboldFont, "Back", {140.f, 48.f})}
{
    title_.setPosition({72.f, 45.f});
    title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({72.f, 100.f});
    subtitle_.setFillColor(Theme::textSecondary);
    status_.setPosition({72.f, 655.f});
    status_.setFillColor(Theme::textMuted);
    empty_.setPosition({72.f, 280.f});
    empty_.setFillColor(Theme::textSecondary);
    help_.setPosition({606.f, 150.f});
    help_.setFillColor(Theme::textMuted);
    buttons_[0].setPosition({72.f, 135.f});
    buttons_[1].setPosition({378.f, 135.f});
    buttons_[2].setPosition({680.f, 640.f});
    buttons_[3].setPosition({855.f, 640.f});
    buttons_[4].setPosition({1068.f, 640.f});
    for (std::size_t i = 0; i < pageSize; ++i) {
        rows_.emplace_back(context.regularFont, "", 16);
        rows_.back().setPosition({72.f, 205.f + static_cast<float>(i) * 52.f});
        rows_.back().setFillColor(Theme::textPrimary);
    }
    select(0);
}

void MatchHistoryScene::onActivate()
{
    // Another profile's history starts unfiltered by result, with the focus on the game filter.
    if (shownProfileId_ != profileId_) {
        shownProfileId_ = profileId_;
        resultFilter_ = 0;
        selected_ = 0;
    }
    for (auto& button : buttons_) button.setHovered(false);
    page_ = 0;
    filter_.game = selectedGame_;
    reload();
}

void MatchHistoryScene::reload()
{
    filter_.result = resultFilter_ == 1 ? std::optional{persistence::MatchResult::Win}
        : resultFilter_ == 2 ? std::optional{persistence::MatchResult::Loss}
        : resultFilter_ == 3 ? std::optional{persistence::MatchResult::Draw} : std::nullopt;
    std::vector<persistence::CompletedMatch> data;
    try {
        const auto list = profiles_.listProfiles();
        const auto found = std::find_if(list.begin(), list.end(), [this](const auto& p) { return p.id == profileId_; });
        subtitle_.setString(found == list.end() ? sf::String("Selected profile")
                                                : toDisplay(fitToWidth(context_.regularFont, found->displayName,
                                                                       Theme::bodySize, 1000.f)));
        total_ = matches_.count(profileId_, filter_);
        if (page_ * pageSize >= static_cast<std::size_t>(total_) && page_ > 0) --page_;
        data = matches_.recent(profileId_, filter_, pageSize, page_ * pageSize);
    } catch (const std::exception& error) {
        // A damaged or unreadable history must not close the application.
        std::cerr << "GameVerseArenaGUI: match history could not be loaded: " << error.what() << '\n';
        total_ = 0;
        for (auto& row : rows_) row.setString("");
        empty_.setString("Match history could not be loaded.\nOther profile data is unaffected; details were logged.");
        refresh();
        return;
    }
    // Long names are shortened keeping their beginning, so each row stays on the screen.
    const auto shortName = [this](const std::string& value) { return fitToWidth(context_.regularFont, value, 16, 130.f); };
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (i >= data.size()) {
            rows_[i].setString("");
            continue;
        }
        const auto& match = data[i];
        const std::string score = match.profileScore && match.opponentScore
            ? std::to_string(*match.profileScore) + "-" + std::to_string(*match.opponentScore) : "-";
        rows_[i].setString(toDisplay(result(match.result) + "  |  " + game(match.game) + "  |  " +
            shortName(match.profileDisplayName) + " vs " + shortName(match.opponentName) + "  |  " + score +
            "  |  " + mode(match) + "  |  " + std::to_string(match.durationMs / 1000) + "s  |  " +
            date(match.completedAt)));
    }
    empty_.setString(total_ == 0 ? "No matches match these filters.\nPlay any game from the library to build your history."
                                 : "");
    refresh();
}

void MatchHistoryScene::refresh()
{
    buttons_[0].setText(filter_.game ? "Game: " + game(*filter_.game) : "Game: All");
    buttons_[1].setText(resultLabels[resultFilter_]);
    const std::size_t pages = total_ == 0 ? 1 : (static_cast<std::size_t>(total_) + pageSize - 1) / pageSize;
    status_.setString("Page " + std::to_string(page_ + 1) + " of " + std::to_string(pages) + "  |  " +
                      std::to_string(total_) + (total_ == 1 ? " match" : " matches"));
    for (std::size_t i = 0; i < buttons_.size(); ++i) buttons_[i].setSelected(i == selected_);
}

void MatchHistoryScene::select(std::size_t index, bool withSound)
{
    if (withSound && index != selected_) context_.play(audio::SoundId::UiFocus);
    selected_ = index;
    for (std::size_t i = 0; i < buttons_.size(); ++i) buttons_[i].setSelected(i == selected_);
}

void MatchHistoryScene::stepFilter(std::size_t index, int direction)
{
    if (index == 0) {
        // All games, then every game in catalogue order.
        const auto& keys = persistence::allGameKeys();
        const std::size_t choices = keys.size() + 1;
        std::size_t position = 0;
        if (filter_.game) {
            position = static_cast<std::size_t>(std::find(keys.begin(), keys.end(), *filter_.game) - keys.begin()) + 1;
        }
        position = (position + choices + (direction < 0 ? choices - 1 : 1)) % choices;
        filter_.game = position == 0 ? std::nullopt : std::optional{keys[position - 1]};
        selectedGame_ = filter_.game;  // Statistics shows the same game when the player goes back
    } else {
        resultFilter_ = (resultFilter_ + resultLabels.size() + (direction < 0 ? resultLabels.size() - 1 : 1)) %
                        resultLabels.size();
    }
    page_ = 0;
    reload();
    context_.play(audio::SoundId::UiConfirm);
}

void MatchHistoryScene::activate(std::size_t index)
{
    if (index <= 1) {
        stepFilter(index, 1);
    } else if (index == 2 && page_ > 0) {
        --page_;
        reload();
        context_.play(audio::SoundId::UiConfirm);
    } else if (index == 3 && (page_ + 1) * pageSize < static_cast<std::size_t>(total_)) {
        ++page_;
        reload();
        context_.play(audio::SoundId::UiConfirm);
    } else if (index == 4) {
        context_.play(audio::SoundId::UiBack);
        scenes_.switchTo(SceneId::ProfileStats);
    } else {
        context_.play(audio::SoundId::UiError);
    }
}

void MatchHistoryScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        using Key = sf::Keyboard::Key;
        if (key->code == Key::Escape) activate(4);
        else if ((key->code == Key::Tab && !key->shift) || key->code == Key::Right)
            select((selected_ + 1) % buttons_.size(), true);
        else if (key->code == Key::Left || key->code == Key::Tab)
            select((selected_ + buttons_.size() - 1) % buttons_.size(), true);
        else if ((key->code == Key::Up || key->code == Key::Down) && selected_ <= 1)
            stepFilter(selected_, key->code == Key::Up ? -1 : 1);
        else if (key->code == Key::Enter || key->code == Key::Space) activate(selected_);
    }
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        for (std::size_t i = 0; i < buttons_.size(); ++i) {
            buttons_[i].setHovered(buttons_[i].contains(point));
            if (buttons_[i].contains(point)) select(i, true);
        }
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>()) {
        const auto point = window.mapPixelToCoords(click->position);
        for (std::size_t i = 0; i < buttons_.size(); ++i) {
            if (!buttons_[i].contains(point)) continue;
            if (click->button == sf::Mouse::Button::Left) activate(i);
            else if (click->button == sf::Mouse::Button::Right && i <= 1) stepFilter(i, -1);  // filters step back
        }
    }
}

void MatchHistoryScene::update(sf::Time deltaTime)
{
    for (auto& button : buttons_) button.update(deltaTime, context_.reducedMotion());
}

void MatchHistoryScene::render(sf::RenderWindow& window) const
{
    window.draw(title_);
    window.draw(subtitle_);
    window.draw(help_);
    for (const auto& row : rows_) window.draw(row);
    window.draw(empty_);
    window.draw(status_);
    for (const auto& button : buttons_) button.draw(window);
}
