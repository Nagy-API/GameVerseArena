#include "PingPongSetupScene.hpp"

#include "Theme.hpp"
#include "Utf8Text.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <array>

using namespace ping_pong;
using audio::SoundId;

namespace {
std::string visibleName(const std::string& value)
{
    constexpr std::size_t maximumVisible = 18;
    return utf8_text::length(value) <= maximumVisible
        ? value
        : "..." + utf8_text::tail(value, maximumVisible - 3);
}
} // namespace

PingPongSetupScene::PingPongSetupScene(AppContext& context, PingPongSession& session,
                                       persistence::ProfileService& profileService, persistence::MatchRecorder& matchRecorder)
    : context_(context), session_(session), profileService_(profileService), matchRecorder_(matchRecorder),
      kicker_(context.semiboldFont, "ARCADE GAMES  /  PING PONG", Theme::labelSize),
      title_(context.semiboldFont, "Match setup", Theme::pageTitleSize),
      subtitle_(context.regularFont, "Choose your players and enter a first-to-five real-time match.", Theme::bodySize),
      rules_(context.regularFont, "LEFT PADDLE\nW / S\n\nRIGHT PADDLE\nArrow Up / Arrow Down\n\nHUMAN VS COMPUTER\nBoth key pairs control the left paddle.", 17),
      help_(context.regularFont, "Up / Down or Tab navigates  |  Left / Right changes  |  Enter edits or starts", Theme::labelSize),
      labels_{sf::Text(context.semiboldFont, "GAME MODE", Theme::labelSize),
              sf::Text(context.semiboldFont, "PLAYER 1 NAME", Theme::labelSize),
              sf::Text(context.semiboldFont, "PLAYER 2 NAME", Theme::labelSize),
              sf::Text(context.semiboldFont, "AI DIFFICULTY", Theme::labelSize)},
      values_{sf::Text(context.regularFont, "", 22), sf::Text(context.regularFont, "", 22),
              sf::Text(context.regularFont, "", 22), sf::Text(context.regularFont, "", 22)},
      startButton_(context.semiboldFont, "Start Match", {250.f, 56.f}), backButton_(context.semiboldFont, "Back", {170.f, 56.f})
{
    kicker_.setPosition({Theme::pageMargin, 42.f}); kicker_.setFillColor(Theme::arcadeRight);
    title_.setPosition({Theme::pageMargin, 70.f}); title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({Theme::pageMargin, 125.f}); subtitle_.setFillColor(Theme::textSecondary);
    rules_.setPosition({790.f, 215.f}); rules_.setFillColor(Theme::textSecondary); rules_.setLineSpacing(1.25f);
    help_.setPosition({Theme::pageMargin, 680.f}); help_.setFillColor(Theme::textMuted);

    for (std::size_t index = 0; index < rowCount; ++index) {
        const float y = 190.f + static_cast<float>(index) * 92.f;
        rows_[index].setSize({620.f, 72.f}); rows_[index].setPosition({72.f, y});
        rows_[index].setFillColor(Theme::backgroundRaised); rows_[index].setOutlineThickness(1.f);
        labels_[index].setPosition({94.f, y + 10.f}); labels_[index].setFillColor(Theme::textMuted);
        values_[index].setPosition({94.f, y + 36.f}); values_[index].setFillColor(Theme::textPrimary);
    }
    backButton_.setPosition({72.f, 590.f}); startButton_.setPosition({958.f, 590.f});
    refresh();
}

void PingPongSetupScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* text = event.getIf<sf::Event::TextEntered>(); text && editing_) {
        if (selected_ == 1) editName(playerOne_, text->unicode);
        else if (selected_ == 2 && mode_ == 0) editName(playerTwo_, text->unicode);
        refresh();
    }
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) {
            if (editing_) { editing_ = false; context_.play(SoundId::UiBack); }
            else { goBack(); return; }
        } else if (key->code == sf::Keyboard::Key::Backspace && editing_) {
            auto& name = selected_ == 1 ? playerOne_ : playerTwo_;
            utf8_text::eraseLast(name);
        } else if (key->code == sf::Keyboard::Key::Tab) moveSelection(key->shift ? -1 : 1);
        else if (key->code == sf::Keyboard::Key::Down) moveSelection(1);
        else if (key->code == sf::Keyboard::Key::Up) moveSelection(-1);
        else if (key->code == sf::Keyboard::Key::Left) adjustSelected(-1);
        else if (key->code == sf::Keyboard::Key::Right) adjustSelected(1);
        else if (key->code == sf::Keyboard::Key::Enter || (key->code == sf::Keyboard::Key::Space && !editing_)) {
            activateSelected();
            return;
        }
        refresh();
    }
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        startButton_.setHovered(startButton_.contains(point)); backButton_.setHovered(backButton_.contains(point));
        if (!editing_) {
            for (std::size_t index = 0; index < rowCount; ++index)
                if (rows_[index].getGlobalBounds().contains(point)) select(index, true);
            if (startButton_.contains(point)) select(startIndex, true);
            if (backButton_.contains(point)) select(backIndex, true);
        }
        refresh();
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        if (startButton_.contains(point)) { select(startIndex, false); startMatch(); return; }
        if (backButton_.contains(point)) { goBack(); return; }
        for (std::size_t index = 0; index < rowCount; ++index) {
            if (!rows_[index].getGlobalBounds().contains(point)) continue;
            select(index, false);
            if (index == 1 || (index == 2 && mode_ == 0)) { editing_ = true; context_.play(SoundId::UiConfirm); }
            else adjustSelected(1);
            break;
        }
        refresh();
    }
}

void PingPongSetupScene::update(sf::Time deltaTime)
{
    startButton_.update(deltaTime, context_.reducedMotion());
    backButton_.update(deltaTime, context_.reducedMotion());
}

void PingPongSetupScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_); window.draw(title_); window.draw(subtitle_); window.draw(rules_);
    for (std::size_t index = 0; index < rowCount; ++index) {
        window.draw(rows_[index]); window.draw(labels_[index]); window.draw(values_[index]);
    }
    backButton_.draw(window); startButton_.draw(window); window.draw(help_);
}

void PingPongSetupScene::onResize(sf::Vector2u) {}
void PingPongSetupScene::onActivate()
{
    if (const auto active = profileService_.activeProfile(); active.has_value()) {
        playerOne_ = active->displayName;
    }
    editing_ = false; selected_ = 0;
    startButton_.setHovered(false); backButton_.setHovered(false);
    refresh();
}

void PingPongSetupScene::select(std::size_t index, bool withSound)
{
    if (withSound && index != selected_) context_.play(SoundId::UiFocus);
    selected_ = index;
}

void PingPongSetupScene::moveSelection(int offset)
{
    editing_ = false;
    constexpr int count = 6;
    select(static_cast<std::size_t>((static_cast<int>(selected_) + offset + count) % count), true);
}

void PingPongSetupScene::adjustSelected(int offset)
{
    editing_ = false;
    if (selected_ == 0) { mode_ = (mode_ + offset + 2) % 2; context_.play(SoundId::UiConfirm); }
    else if (selected_ == 3 && mode_ == 1) { difficulty_ = (difficulty_ + offset + 3) % 3; context_.play(SoundId::UiConfirm); }
    else if (selected_ < rowCount) context_.play(SoundId::UiError);
}

void PingPongSetupScene::activateSelected()
{
    if (selected_ == 1 || (selected_ == 2 && mode_ == 0)) {
        editing_ = !editing_;
        context_.play(editing_ ? SoundId::UiConfirm : SoundId::UiBack);
        refresh();
    } else if (selected_ < rowCount) {
        adjustSelected(1);
        refresh();
    } else if (selected_ == startIndex) {
        startMatch();
    } else {
        goBack();
    }
}

void PingPongSetupScene::goBack()
{
    editing_ = false;
    context_.play(SoundId::UiBack);
    context_.scenes.switchTo(SceneId::GameLibrary);
}

void PingPongSetupScene::editName(std::string& name, char32_t codepoint)
{
    if (codepoint < 32) return;  // Control characters (Enter, Backspace, Tab) are handled as keys.
    if (!utf8_text::appendPrintable(name, codepoint, 24)) context_.play(SoundId::UiError);
}

void PingPongSetupScene::startMatch()
{
    SessionConfig config;
    config.leftPlayerName = playerOne_;
    config.rightPlayerName = mode_ == 0 ? playerTwo_ : "Computer";
    config.mode = mode_ == 0 ? GameMode::HumanVsHuman : GameMode::HumanVsComputer;
    config.difficulty = static_cast<AIDifficulty>(difficulty_);
    session_.startMatch(config);
    if (const auto active = profileService_.activeProfile()) matchRecorder_.beginPingPong(*active, session_.config());
    else matchRecorder_.abandon();
    editing_ = false;
    context_.play(SoundId::UiConfirm);
    context_.scenes.switchTo(SceneId::PingPongGame);
}

void PingPongSetupScene::refresh()
{
    static constexpr std::array<const char*, 3> difficulties{"Easy", "Medium", "Hard"};
    values_[0].setString(mode_ == 0 ? "Human vs Human" : "Human vs Computer");
    values_[1].setString(visibleName(playerOne_) + (editing_ && selected_ == 1 ? " |" : ""));
    values_[2].setString(mode_ == 0 ? visibleName(playerTwo_) + (editing_ && selected_ == 2 ? " |" : "") : "Computer");
    values_[3].setString(mode_ == 0 ? "Not used" : difficulties[static_cast<std::size_t>(difficulty_)]);
    labels_[1].setString("PLAYER 1 NAME  |  ACTIVE PROFILE DEFAULT");
    for (std::size_t index = 0; index < rowCount; ++index) {
        rows_[index].setOutlineColor(index == selected_ ? Theme::arcadeLeft : Theme::border);
        rows_[index].setOutlineThickness(index == selected_ ? 2.f : 1.f);
    }
    startButton_.setSelected(selected_ == startIndex); backButton_.setSelected(selected_ == backIndex);
}
