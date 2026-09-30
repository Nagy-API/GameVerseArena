#include "BoardGameSetupScene.hpp"

#include "BoardViews.hpp"
#include "MatchTypes.hpp"
#include "TextLayout.hpp"
#include "Theme.hpp"
#include "Utf8Text.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>
#include <random>

using audio::SoundId;
using turn_based::Seat;

namespace {
constexpr float rowWidth = 548.f;
constexpr float valueWidth = rowWidth - 44.f;

std::string uppercase(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) { return static_cast<char>(std::toupper(character)); });
    return text;
}
} // namespace

BoardGameSetupScene::BoardGameSetupScene(AppContext& context, BoardGameHost& host,
                                         persistence::ProfileService& profileService,
                                         persistence::MatchRecorder& matchRecorder)
    : context_(context), host_(host), profileService_(profileService), matchRecorder_(matchRecorder),
      kicker_(context.semiboldFont, "", Theme::labelSize), title_(context.semiboldFont, "Player setup", Theme::pageTitleSize),
      subtitle_(context.regularFont, "", Theme::bodySize),
      help_(context.regularFont, "Up / Down or Tab to navigate  |  Left / Right to change  |  Enter to edit or start",
            Theme::labelSize),
      message_(context.regularFont, "", 16), rulesPanel_({rowWidth, 368.f}),
      rulesHeading_(context.semiboldFont, "HOW TO PLAY", Theme::labelSize), rulesText_(context.regularFont, "", 17),
      computerHeading_(context.semiboldFont, "COMPUTER OPPONENT", Theme::labelSize), computerText_(context.regularFont, "", 16),
      startButton_(context.semiboldFont, "Start Game", {250.f, 56.f}), backButton_(context.semiboldFont, "Back", {170.f, 56.f})
{
    kicker_.setPosition({Theme::pageMargin, 42.f});
    kicker_.setFillColor(Theme::secondary);
    title_.setPosition({Theme::pageMargin, 70.f});
    title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({Theme::pageMargin, 125.f});
    subtitle_.setFillColor(Theme::textSecondary);
    message_.setPosition({Theme::pageMargin, 648.f});
    message_.setFillColor(Theme::warning);
    help_.setPosition({Theme::pageMargin, 680.f});
    help_.setFillColor(Theme::textMuted);

    labels_.reserve(rowCount);
    values_.reserve(rowCount);
    for (std::size_t index = 0; index < rowCount; ++index) {
        const float y = 180.f + static_cast<float>(index) * 96.f;
        rows_[index].setSize({rowWidth, 80.f});
        rows_[index].setPosition({Theme::pageMargin, y});
        rows_[index].setFillColor(Theme::backgroundRaised);
        labels_.emplace_back(context.semiboldFont, "", Theme::labelSize);
        labels_.back().setPosition({Theme::pageMargin + 22.f, y + 12.f});
        labels_.back().setFillColor(Theme::textMuted);
        values_.emplace_back(context.regularFont, "", 21);
        values_.back().setPosition({Theme::pageMargin + 22.f, y + 39.f});
        values_.back().setFillColor(Theme::textPrimary);
    }

    rulesPanel_.setPosition({660.f, 180.f});
    rulesPanel_.setFillColor(Theme::backgroundRaised);
    rulesPanel_.setOutlineThickness(1.f);
    rulesPanel_.setOutlineColor(Theme::border);
    rulesHeading_.setPosition({682.f, 196.f});
    rulesHeading_.setFillColor(Theme::secondary);
    rulesText_.setPosition({682.f, 222.f});
    rulesText_.setFillColor(Theme::textSecondary);
    computerHeading_.setPosition({682.f, 452.f});
    computerHeading_.setFillColor(Theme::secondary);
    computerText_.setPosition({682.f, 476.f});
    computerText_.setFillColor(Theme::textSecondary);

    backButton_.setPosition({Theme::pageMargin, 580.f});
    startButton_.setPosition({958.f, 580.f});
}

void BoardGameSetupScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (!host_.game) return;
    if (const auto* text = event.getIf<sf::Event::TextEntered>(); text && editing_) {
        if (selected_ == 1) editName(playerOne_, text->unicode);
        if (selected_ == 2 && mode_ == 0) editName(playerTwo_, text->unicode);
        refresh();
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        using Key = sf::Keyboard::Key;
        if (key->code == Key::Escape) {
            if (editing_) {
                editing_ = false;
                context_.play(SoundId::UiBack);
            } else {
                goBack();
                return;
            }
        } else if (key->code == Key::Backspace && editing_) {
            utf8_text::eraseLast(selected_ == 1 ? playerOne_ : playerTwo_);
        } else if (key->code == Key::Tab) {
            moveSelection(key->shift ? -1 : 1);
        } else if (key->code == Key::Down) {
            moveSelection(1);
        } else if (key->code == Key::Up) {
            moveSelection(-1);
        } else if (key->code == Key::Left && !editing_) {
            adjustSelected(-1);
        } else if (key->code == Key::Right && !editing_) {
            adjustSelected(1);
        } else if (key->code == Key::Enter || (key->code == Key::Space && !editing_ && !nameEditable(selected_))) {
            // Space never starts editing a name: its text event would type a leading space.
            activateSelected();
            return;
        }
        refresh();
    }

    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        startButton_.setHovered(startButton_.contains(point));
        backButton_.setHovered(backButton_.contains(point));
        if (!editing_) {
            for (std::size_t index = 0; index < rowCount; ++index) {
                if (rows_[index].getGlobalBounds().contains(point)) select(index, true);
            }
            if (startButton_.contains(point)) select(startIndex, true);
            if (backButton_.contains(point)) select(backIndex, true);
        }
        refresh();
    }

    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        if (startButton_.contains(point)) {
            select(startIndex, false);
            startGame();
            return;
        }
        if (backButton_.contains(point)) {
            goBack();
            return;
        }
        for (std::size_t index = 0; index < rowCount; ++index) {
            if (!rows_[index].getGlobalBounds().contains(point)) continue;
            select(index, false);
            if (nameEditable(index)) {
                editing_ = true;
                context_.play(SoundId::UiConfirm);
            } else {
                adjustSelected(1);
            }
            break;
        }
        refresh();
    }
}

void BoardGameSetupScene::update(sf::Time deltaTime)
{
    startButton_.update(deltaTime, context_.reducedMotion());
    backButton_.update(deltaTime, context_.reducedMotion());
}

void BoardGameSetupScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_);
    window.draw(title_);
    window.draw(subtitle_);
    for (std::size_t index = 0; index < rowCount; ++index) {
        window.draw(rows_[index]);
        window.draw(labels_[index]);
        window.draw(values_[index]);
    }
    window.draw(rulesPanel_);
    window.draw(rulesHeading_);
    window.draw(rulesText_);
    window.draw(computerHeading_);
    window.draw(computerText_);
    backButton_.draw(window);
    startButton_.draw(window);
    window.draw(message_);
    window.draw(help_);
}

void BoardGameSetupScene::onResize(sf::Vector2u) {}

void BoardGameSetupScene::onActivate()
{
    if (!host_.game) {
        context_.scenes.switchTo(SceneId::GameLibrary);
        return;
    }
    if (const auto active = profileService_.activeProfile()) playerOne_ = active->displayName;
    if (!game().humanVsHuman) mode_ = 1;
    if (!game().humanVsComputer) mode_ = 0;
    editing_ = false;
    selected_ = 0;
    message_.setString("");
    startButton_.setHovered(false);
    backButton_.setHovered(false);
    refreshRules();
    refresh();
}

bool BoardGameSetupScene::nameEditable(std::size_t row) const
{
    return row == 1 || (row == 2 && mode_ == 0);
}

void BoardGameSetupScene::select(std::size_t index, bool withSound)
{
    if (withSound && index != selected_) context_.play(SoundId::UiFocus);
    selected_ = index;
}

void BoardGameSetupScene::moveSelection(int offset)
{
    editing_ = false;
    constexpr int count = static_cast<int>(backIndex) + 1;
    select(static_cast<std::size_t>((static_cast<int>(selected_) + offset + count) % count), true);
}

void BoardGameSetupScene::adjustSelected(int offset)
{
    editing_ = false;
    bool changed = false;
    if (selected_ == 0 && game().humanVsHuman && game().humanVsComputer) {
        mode_ = 1 - mode_;
        changed = true;
    } else if (selected_ == 3 && mode_ == 1) {
        humanSeat_ = 1 - humanSeat_;
        changed = true;
    }
    (void)offset;  // Every setting here has exactly two values, so both directions toggle.
    if (changed) context_.play(SoundId::UiConfirm);
    else if (selected_ < rowCount) context_.play(SoundId::UiError);
}

void BoardGameSetupScene::activateSelected()
{
    if (nameEditable(selected_)) {
        editing_ = !editing_;
        context_.play(editing_ ? SoundId::UiConfirm : SoundId::UiBack);
        refresh();
    } else if (selected_ < rowCount) {
        adjustSelected(1);
        refresh();
    } else if (selected_ == startIndex) {
        startGame();
    } else {
        goBack();
    }
}

void BoardGameSetupScene::goBack()
{
    editing_ = false;
    context_.play(SoundId::UiBack);
    context_.scenes.switchTo(SceneId::GameLibrary);
}

void BoardGameSetupScene::startGame()
{
    const auto& selectedGame = game();
    auto view = board_view::createBoardView(selectedGame.key);
    if (!view || !selectedGame.createGame) {
        message_.setString(toDisplay(selectedGame.displayName + " could not be started."));
        context_.play(SoundId::UiError);
        return;
    }

    turn_based::SessionSetup setup;
    setup.mode = mode_ == 1 ? turn_based::PlayMode::HumanVsComputer : turn_based::PlayMode::HumanVsHuman;
    setup.firstName = playerOne_;
    setup.secondName = playerTwo_;
    setup.humanSeat = humanSeat_ == 0 ? Seat::First : Seat::Second;
    std::random_device entropy;
    host_.session.start(selectedGame.createGame(entropy()), setup);
    host_.view = std::move(view);

    // The finished game is recorded for the active profile's player: Player 1 in human-vs-human
    // play, or the human against the computer.
    host_.recordingUnavailable = true;
    try {
        const auto active = profileService_.activeProfile();
        if (active) {
            const bool computer = setup.mode == turn_based::PlayMode::HumanVsComputer;
            const Seat profileSeat = computer ? setup.humanSeat : Seat::First;
            matchRecorder_.beginBoardGame(
                *active, persistence::gameKeyFromStorage(selectedGame.key),
                computer ? persistence::MatchMode::HumanVsComputer : persistence::MatchMode::HumanVsHuman,
                profileSeat == Seat::First, host_.session.nameOf(profileSeat),
                host_.session.nameOf(turn_based::otherSeat(profileSeat)));
            host_.recordingUnavailable = false;
        } else {
            matchRecorder_.abandon();
            std::cerr << "GameVerseArenaGUI: no active profile; " << selectedGame.displayName << " will not be recorded\n";
        }
    } catch (const std::exception& error) {
        matchRecorder_.abandon();
        std::cerr << "GameVerseArenaGUI: " << selectedGame.displayName << " will not be recorded: " << error.what() << '\n';
    }

    editing_ = false;
    message_.setString("");
    context_.play(SoundId::UiConfirm);
    context_.scenes.switchTo(SceneId::BoardGame);
}

void BoardGameSetupScene::refresh()
{
    if (!host_.game) return;
    const auto& selectedGame = game();
    const auto fitValue = [this](const std::string& text, bool typing) {
        return toDisplay(typing ? fitTailToWidth(context_.regularFont, text + " |", 21, valueWidth)
                                : fitToWidth(context_.regularFont, text, 21, valueWidth));
    };
    const bool computer = mode_ == 1;
    values_[0].setString(computer ? "Human vs Computer" : "Human vs Human");
    values_[1].setString(fitValue(playerOne_, editing_ && selected_ == 1));
    values_[2].setString(computer ? sf::String("Computer") : fitValue(playerTwo_, editing_ && selected_ == 2));
    if (computer) {
        values_[3].setString(fitValue(humanSeat_ == 0 ? selectedGame.firstSeatLabel + " (moves first)"
                                                      : selectedGame.secondSeatLabel + " (moves second)",
                                      false));
    } else {
        values_[3].setString(fitValue("Player 1: " + selectedGame.firstSeatLabel + "  |  Player 2: " +
                                          selectedGame.secondSeatLabel,
                                      false));
    }
    labels_[0].setString("GAME MODE");
    labels_[1].setString(computer ? "YOUR NAME  |  ACTIVE PROFILE DEFAULT" : "PLAYER 1 NAME  |  ACTIVE PROFILE DEFAULT");
    labels_[2].setString(computer ? "OPPONENT" : "PLAYER 2 NAME");
    labels_[3].setString(computer ? "YOUR SIDE" : "SIDES");
    for (std::size_t index = 0; index < rowCount; ++index) {
        rows_[index].setOutlineColor(index == selected_ ? Theme::primaryBright : Theme::border);
        rows_[index].setOutlineThickness(index == selected_ ? 2.f : 1.f);
    }
    startButton_.setSelected(selected_ == startIndex);
    backButton_.setSelected(selected_ == backIndex);
}

void BoardGameSetupScene::refreshRules()
{
    const auto& selectedGame = game();
    kicker_.setString(toDisplay(uppercase(selectedGame.displayName)));
    subtitle_.setString(toDisplay(fitToWidth(context_.regularFont, selectedGame.shortDescription, Theme::bodySize, 1136.f)));
    rulesText_.setString(toDisplay(wrapToWidth(context_.regularFont, selectedGame.rules, 17, 504.f, 9)));
    computerText_.setString(toDisplay(wrapToWidth(context_.regularFont,
                                                  selectedGame.humanVsComputer ? selectedGame.computerStrategy
                                                                               : "This game is for two players.",
                                                  16, 504.f, 3)));
    // The computer section follows the rules text directly, however long the rules are.
    const auto rulesBounds = rulesText_.getGlobalBounds();
    const float below = rulesBounds.position.y + rulesBounds.size.y + 26.f;
    computerHeading_.setPosition({682.f, below});
    computerText_.setPosition({682.f, below + 24.f});
}

void BoardGameSetupScene::editName(std::string& name, char32_t codepoint)
{
    if (codepoint < 32) return;  // Enter, Backspace, and Tab also arrive as text and are handled as keys.
    if (!utf8_text::appendPrintable(name, codepoint, 24)) context_.play(SoundId::UiError);
}
