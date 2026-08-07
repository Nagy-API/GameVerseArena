#include "PingPongPauseOverlay.hpp"

#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

PingPongPauseOverlay::PingPongPauseOverlay(const sf::Font& regularFont, const sf::Font& semiboldFont)
    : shade_(Theme::logicalSize), panel_({650.f, 500.f}),
      eyebrow_(semiboldFont, "MATCH PAUSED", Theme::labelSize), title_(semiboldFont, "Take a breather", 40),
      message_(regularFont, "The simulation and serve clock are stopped.", Theme::bodySize),
      buttons_{UiButton(semiboldFont, "Resume", {390.f, 54.f}),
               UiButton(semiboldFont, "Restart Match", {390.f, 54.f}),
               UiButton(semiboldFont, "New Setup", {390.f, 54.f}),
               UiButton(semiboldFont, "Return to Library", {390.f, 54.f})}
{
    shade_.setFillColor(Theme::overlay); panel_.setPosition({315.f, 110.f});
    panel_.setFillColor(Theme::backgroundRaised); panel_.setOutlineThickness(1.f); panel_.setOutlineColor(Theme::arcadeLeft);
    eyebrow_.setPosition({365.f, 150.f}); eyebrow_.setFillColor(Theme::arcadeRight);
    title_.setPosition({365.f, 180.f}); title_.setFillColor(Theme::textPrimary);
    message_.setPosition({365.f, 240.f}); message_.setFillColor(Theme::textSecondary);
    for (std::size_t index = 0; index < buttons_.size(); ++index)
        buttons_[index].setPosition({445.f, 292.f + static_cast<float>(index) * 64.f});
    resetSelection();
}

PingPongPauseAction PingPongPauseOverlay::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) return PingPongPauseAction::Resume;
        if (key->code == sf::Keyboard::Key::Up) select((selected_ + buttons_.size() - 1) % buttons_.size());
        else if (key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::Tab) select((selected_ + 1) % buttons_.size());
        else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) return actionFor(selected_);
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
            if (buttons_[index].contains(point)) return actionFor(index);
    }
    return PingPongPauseAction::None;
}

void PingPongPauseOverlay::update(sf::Time deltaTime) { for (auto& button : buttons_) button.update(deltaTime); }
void PingPongPauseOverlay::draw(sf::RenderTarget& target) const
{
    target.draw(shade_); target.draw(panel_); target.draw(eyebrow_); target.draw(title_); target.draw(message_);
    for (const auto& button : buttons_) button.draw(target);
}
void PingPongPauseOverlay::resetSelection() { select(0); }
void PingPongPauseOverlay::select(std::size_t index)
{
    selected_ = index;
    for (std::size_t item = 0; item < buttons_.size(); ++item) buttons_[item].setSelected(item == selected_);
}
PingPongPauseAction PingPongPauseOverlay::actionFor(std::size_t index) const
{
    return static_cast<PingPongPauseAction>(index + 1);
}
