#include "TicTacToeSetupScene.hpp"

#include "Theme.hpp"
#include "Utf8Text.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <array>

using namespace classic_ttt;

namespace {
constexpr std::array<const char*, 6> labels{
    "GAME MODE", "PLAYER 1 NAME", "PLAYER 2 NAME", "YOUR MARK", "AI DIFFICULTY", "MATCH LENGTH"
};

std::string visibleName(const std::string& value)
{
    constexpr std::size_t displayed = 18;
    if (utf8_text::length(value) <= displayed) return value;
    return "..." + utf8_text::tail(value, displayed - 3);
}
} // namespace

TicTacToeSetupScene::TicTacToeSetupScene(const sf::Font& regularFont, const sf::Font& semiboldFont,
                                         SceneManager& sceneManager, TicTacToeSession& session,
                                         persistence::ProfileService& profileService, persistence::MatchRecorder& matchRecorder)
    : sceneManager_(sceneManager), session_(session), profileService_(profileService), matchRecorder_(matchRecorder),
      regularFont_(regularFont), semiboldFont_(semiboldFont),
      kicker_(semiboldFont, "CLASSIC TIC-TAC-TOE", Theme::labelSize),
      title_(semiboldFont, "Player setup", Theme::pageTitleSize),
      subtitle_(regularFont, "Choose players, marks, and match length before entering the board.", Theme::bodySize),
      help_(regularFont, "Up / Down or Tab to navigate  |  Left / Right to change  |  Enter to edit or start", Theme::labelSize),
      startButton_(semiboldFont, "Start Match", {250.f, 56.f}), backButton_(semiboldFont, "Back", {170.f, 56.f})
{
    kicker_.setPosition({Theme::pageMargin, 42.f});
    kicker_.setFillColor(Theme::secondary);
    title_.setPosition({Theme::pageMargin, 70.f});
    title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({Theme::pageMargin, 125.f});
    subtitle_.setFillColor(Theme::textSecondary);
    help_.setPosition({Theme::pageMargin, 680.f});
    help_.setFillColor(Theme::textMuted);

    labels_.reserve(rowCount);
    values_.reserve(rowCount);
    for (std::size_t index = 0; index < rowCount; ++index) {
        const float x = index < 3 ? 72.f : 660.f;
        const float y = 190.f + static_cast<float>(index % 3) * 112.f;
        rows_[index].setSize({548.f, 82.f});
        rows_[index].setPosition({x, y});
        rows_[index].setFillColor(Theme::backgroundRaised);
        rows_[index].setOutlineThickness(1.f);
        labels_.emplace_back(semiboldFont_, labels[index], Theme::labelSize);
        labels_.back().setPosition({x + 22.f, y + 13.f});
        labels_.back().setFillColor(Theme::textMuted);
        values_.emplace_back(regularFont_, "", 21);
        values_.back().setPosition({x + 22.f, y + 42.f});
        values_.back().setFillColor(Theme::textPrimary);
    }

    backButton_.setPosition({72.f, 580.f});
    startButton_.setPosition({958.f, 580.f});
    refresh();
}

void TicTacToeSetupScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* text = event.getIf<sf::Event::TextEntered>(); text && editing_) {
        if (selected_ == 1) editName(playerOne_, text->unicode);
        if (selected_ == 2 && mode_ == 0) editName(playerTwo_, text->unicode);
        refresh();
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) {
            if (editing_) editing_ = false;
            else sceneManager_.switchTo(SceneId::GameLibrary);
        } else if (key->code == sf::Keyboard::Key::Backspace && editing_) {
            auto& name = selected_ == 1 ? playerOne_ : playerTwo_;
            utf8_text::eraseLast(name);
        } else if (key->code == sf::Keyboard::Key::Tab || key->code == sf::Keyboard::Key::Down) {
            moveSelection(1);
        } else if (key->code == sf::Keyboard::Key::Up) {
            moveSelection(-1);
        } else if (key->code == sf::Keyboard::Key::Left) {
            adjustSelected(-1);
        } else if (key->code == sf::Keyboard::Key::Right) {
            adjustSelected(1);
        } else if (key->code == sf::Keyboard::Key::Enter) {
            activateSelected();
        }
        refresh();
    }

    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        startButton_.setHovered(startButton_.contains(point));
        backButton_.setHovered(backButton_.contains(point));
        for (std::size_t index = 0; index < rowCount; ++index) {
            if (rows_[index].getGlobalBounds().contains(point)) selected_ = index;
        }
        refresh();
    }

    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        if (startButton_.contains(point)) { selected_ = startIndex; startMatch(); }
        else if (backButton_.contains(point)) sceneManager_.switchTo(SceneId::GameLibrary);
        else {
            for (std::size_t index = 0; index < rowCount; ++index) {
                if (rows_[index].getGlobalBounds().contains(point)) {
                    selected_ = index;
                    if (index == 1 || (index == 2 && mode_ == 0)) editing_ = true;
                    else adjustSelected(1);
                    break;
                }
            }
        }
        refresh();
    }
}

void TicTacToeSetupScene::update(sf::Time deltaTime)
{
    startButton_.update(deltaTime);
    backButton_.update(deltaTime);
}

void TicTacToeSetupScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_); window.draw(title_); window.draw(subtitle_);
    for (std::size_t index = 0; index < rowCount; ++index) {
        window.draw(rows_[index]); window.draw(labels_[index]); window.draw(values_[index]);
    }
    backButton_.draw(window); startButton_.draw(window); window.draw(help_);
}

void TicTacToeSetupScene::onResize(sf::Vector2u) {}

void TicTacToeSetupScene::onActivate()
{
    if (const auto active = profileService_.activeProfile(); active.has_value()) {
        playerOne_ = active->displayName;
    }
    editing_ = false;
    selected_ = 0;
    refresh();
}

void TicTacToeSetupScene::moveSelection(int offset)
{
    editing_ = false;
    constexpr int count = 8;
    selected_ = static_cast<std::size_t>((static_cast<int>(selected_) + offset + count) % count);
}

void TicTacToeSetupScene::adjustSelected(int offset)
{
    editing_ = false;
    const auto cycle = [offset](int value, int count) { return (value + offset + count) % count; };
    if (selected_ == 0) mode_ = cycle(mode_, 2);
    else if (selected_ == 3 && mode_ == 1) humanMark_ = cycle(humanMark_, 2);
    else if (selected_ == 4 && mode_ == 1) difficulty_ = cycle(difficulty_, 3);
    else if (selected_ == 5) bestOf_ = cycle(bestOf_, 3);
}

void TicTacToeSetupScene::activateSelected()
{
    if (selected_ == 1 || (selected_ == 2 && mode_ == 0)) editing_ = !editing_;
    else if (selected_ < rowCount) adjustSelected(1);
    else if (selected_ == startIndex) startMatch();
    else sceneManager_.switchTo(SceneId::GameLibrary);
}

void TicTacToeSetupScene::startMatch()
{
    SessionConfig config;
    config.mode = mode_ == 0 ? GameMode::HumanVsHuman : GameMode::HumanVsComputer;
    config.playerOneName = playerOne_;
    config.playerTwoName = mode_ == 0 ? playerTwo_ : "Computer";
    config.humanMark = humanMark_ == 0 ? Cell::X : Cell::O;
    config.difficulty = static_cast<AIDifficulty>(difficulty_);
    config.bestOf = bestOf_ == 0 ? BestOf::Single : bestOf_ == 1 ? BestOf::Three : BestOf::Five;
    session_.startNewMatch(config);
    if (const auto active = profileService_.activeProfile()) matchRecorder_.beginTicTacToe(*active, session_.config());
    editing_ = false;
    sceneManager_.switchTo(SceneId::TicTacToeGame);
}

void TicTacToeSetupScene::refresh()
{
    static constexpr std::array<const char*, 3> difficulties{"Easy", "Medium", "Hard"};
    static constexpr std::array<const char*, 3> lengths{"Single Game", "Best of 3", "Best of 5"};
    values_[0].setString(mode_ == 0 ? "Human vs Human" : "Human vs Computer");
    values_[1].setString(visibleName(playerOne_) + (editing_ && selected_ == 1 ? " |" : ""));
    values_[2].setString(mode_ == 0 ? visibleName(playerTwo_) + (editing_ && selected_ == 2 ? " |" : "") : "Computer");
    values_[3].setString(mode_ == 0 ? "X: Player 1  |  O: Player 2" : humanMark_ == 0 ? "X" : "O");
    values_[4].setString(mode_ == 0 ? "Not used" : difficulties[static_cast<std::size_t>(difficulty_)]);
    values_[5].setString(lengths[static_cast<std::size_t>(bestOf_)]);
    labels_[1].setString("PLAYER 1 NAME  |  ACTIVE PROFILE DEFAULT");
    for (std::size_t index = 0; index < rowCount; ++index) {
        rows_[index].setOutlineColor(index == selected_ ? Theme::primaryBright : Theme::border);
        rows_[index].setOutlineThickness(index == selected_ ? 2.f : 1.f);
    }
    startButton_.setSelected(selected_ == startIndex);
    backButton_.setSelected(selected_ == backIndex);
}

void TicTacToeSetupScene::editName(std::string& name, char32_t codepoint)
{
    utf8_text::appendPrintable(name, codepoint, 24);
}
