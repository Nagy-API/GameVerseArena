#include "PingPongResultOverlay.hpp"

#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

using namespace ping_pong;

PingPongResultOverlay::PingPongResultOverlay(const sf::Font& regularFont, const sf::Font& semiboldFont)
    : shade_(Theme::logicalSize), panel_({700.f, 450.f}), eyebrow_(semiboldFont, "MATCH COMPLETE", Theme::labelSize),
      title_(semiboldFont, "", 42), score_(semiboldFont, "", 26),
      message_(regularFont, "First to five claims the arena.", Theme::bodySize),
      buttons_{UiButton(semiboldFont, "Rematch", {410.f, 54.f}),
               UiButton(semiboldFont, "New Setup", {410.f, 54.f}),
               UiButton(semiboldFont, "Return to Library", {410.f, 54.f})}
{
    shade_.setFillColor(Theme::overlay); panel_.setPosition({290.f, 135.f});
    panel_.setFillColor(Theme::backgroundRaised); panel_.setOutlineThickness(1.f); panel_.setOutlineColor(Theme::arcadeRight);
    eyebrow_.setPosition({340.f, 175.f}); eyebrow_.setFillColor(Theme::arcadeRight);
    title_.setPosition({340.f, 205.f}); title_.setFillColor(Theme::textPrimary);
    score_.setPosition({340.f, 270.f}); score_.setFillColor(Theme::textPrimary);
    message_.setPosition({340.f, 310.f}); message_.setFillColor(Theme::textSecondary);
    for (std::size_t index = 0; index < buttons_.size(); ++index)
        buttons_[index].setPosition({435.f, 355.f + static_cast<float>(index) * 64.f});
}

void PingPongResultOverlay::show(const PingPongSession& session)
{
    visible_ = true; select(0);
    const Side winner = session.winner().value_or(Side::Left);
    title_.setString(session.playerName(winner) + " wins");
    score_.setString(session.playerName(Side::Left) + "  " + std::to_string(session.score().left) + "  -  " +
                     std::to_string(session.score().right) + "  " + session.playerName(Side::Right));
}

PingPongResultAction PingPongResultOverlay::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (!visible_) return PingPongResultAction::None;
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Up) select((selected_ + 2) % 3);
        else if (key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::Tab) select((selected_ + 1) % 3);
        else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
            return static_cast<PingPongResultAction>(selected_ + 1);
    }
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        for (std::size_t index = 0; index < buttons_.size(); ++index) {
            const bool hovered = buttons_[index].contains(point); buttons_[index].setHovered(hovered);
            if (hovered) select(index);
        }
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>(); click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        for (std::size_t index = 0; index < buttons_.size(); ++index)
            if (buttons_[index].contains(point)) return static_cast<PingPongResultAction>(index + 1);
    }
    return PingPongResultAction::None;
}

void PingPongResultOverlay::update(sf::Time deltaTime) { if (visible_) for (auto& button : buttons_) button.update(deltaTime); }
void PingPongResultOverlay::draw(sf::RenderTarget& target) const
{
    if (!visible_) return;
    target.draw(shade_); target.draw(panel_); target.draw(eyebrow_); target.draw(title_); target.draw(score_); target.draw(message_);
    for (const auto& button : buttons_) button.draw(target);
}
void PingPongResultOverlay::select(std::size_t index)
{
    selected_ = index;
    for (std::size_t item = 0; item < buttons_.size(); ++item) buttons_[item].setSelected(item == selected_);
}
