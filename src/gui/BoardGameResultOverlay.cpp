#include "BoardGameResultOverlay.hpp"

#include "TextLayout.hpp"
#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

using turn_based::Seat;

namespace {
constexpr float textLeft = 340.f;
constexpr float textWidth = 600.f;
} // namespace

BoardGameResultOverlay::BoardGameResultOverlay(AppContext& context)
    : context_(context), shade_(Theme::logicalSize), panel_({700.f, 470.f}),
      eyebrow_(context.semiboldFont, "GAME COMPLETE", Theme::labelSize), title_(context.semiboldFont, "", 40),
      reason_(context.regularFont, "", Theme::bodySize), score_(context.semiboldFont, "", 20),
      warning_(context.regularFont, "", 15),
      buttons_{UiButton(context.semiboldFont, "Rematch", {410.f, 52.f}),
               UiButton(context.semiboldFont, "View Final Board", {410.f, 52.f}),
               UiButton(context.semiboldFont, "New Setup", {410.f, 52.f}),
               UiButton(context.semiboldFont, "Return to Library", {410.f, 52.f})}
{
    shade_.setFillColor(Theme::overlay);
    panel_.setPosition({290.f, 125.f});
    panel_.setFillColor(Theme::backgroundRaised);
    panel_.setOutlineThickness(1.f);
    panel_.setOutlineColor(Theme::primary);
    eyebrow_.setPosition({textLeft, 160.f});
    eyebrow_.setFillColor(Theme::secondary);
    title_.setPosition({textLeft, 186.f});
    title_.setFillColor(Theme::textPrimary);
    reason_.setPosition({textLeft, 244.f});
    reason_.setFillColor(Theme::textSecondary);
    score_.setPosition({textLeft, 278.f});
    score_.setFillColor(Theme::textPrimary);
    warning_.setPosition({textLeft, 310.f});
    warning_.setFillColor(Theme::warning);
    for (std::size_t index = 0; index < buttons_.size(); ++index) {
        buttons_[index].setPosition({435.f, 340.f + static_cast<float>(index) * 62.f});
    }
}

void BoardGameResultOverlay::show(const turn_based::BoardGameSession& session, const turn_based::Outcome& outcome,
                                  const catalogue::GameDescriptor& game)
{
    const auto fit = [this](const sf::Font& font, const std::string& text, unsigned int size) {
        return toDisplay(fitToWidth(font, text, size, textWidth));
    };
    if (outcome.winner) {
        title_.setString(fit(context_.semiboldFont, session.nameOf(*outcome.winner) + " wins", 40));
    } else {
        title_.setString("It's a draw");
    }
    reason_.setString(fit(context_.regularFont, outcome.reason, Theme::bodySize));
    std::string score;
    if (const auto points = session.game().scores()) {
        score = session.nameOf(Seat::First) + " (" + game.firstSeatLabel + ")  " + std::to_string(points->first) +
                " - " + std::to_string(points->second) + "  " + session.nameOf(Seat::Second) + " (" +
                game.secondSeatLabel + ")";
    }
    score_.setString(fit(context_.semiboldFont, score, 20));
    warning_.setString("");
    for (auto& button : buttons_) button.setHovered(false);
    select(0);
    visible_ = true;
}

void BoardGameResultOverlay::reveal()
{
    for (auto& button : buttons_) button.setHovered(false);
    select(0);
    visible_ = true;
}

void BoardGameResultOverlay::setWarning(const std::string& warning)
{
    warning_.setString(toDisplay(warning));
}

BoardGameResultAction BoardGameResultOverlay::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (!visible_) return BoardGameResultAction::None;
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        using Key = sf::Keyboard::Key;
        if (key->code == Key::Up || (key->code == Key::Tab && key->shift)) {
            select((selected_ + buttonCount - 1) % buttonCount, true);
        } else if (key->code == Key::Down || key->code == Key::Tab) {
            select((selected_ + 1) % buttonCount, true);
        } else if (key->code == Key::Enter || key->code == Key::Space) {
            return actionFor(selected_);
        } else if (key->code == Key::Escape) {
            return BoardGameResultAction::ViewBoard;
        }
    }
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        for (std::size_t index = 0; index < buttons_.size(); ++index) {
            const bool hovered = buttons_[index].contains(point);
            buttons_[index].setHovered(hovered);
            if (hovered) select(index, true);
        }
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        for (std::size_t index = 0; index < buttons_.size(); ++index) {
            if (buttons_[index].contains(point)) return actionFor(index);
        }
    }
    return BoardGameResultAction::None;
}

void BoardGameResultOverlay::update(sf::Time deltaTime)
{
    if (!visible_) return;
    for (auto& button : buttons_) button.update(deltaTime, context_.reducedMotion());
}

void BoardGameResultOverlay::draw(sf::RenderTarget& target) const
{
    if (!visible_) return;
    target.draw(shade_);
    target.draw(panel_);
    target.draw(eyebrow_);
    target.draw(title_);
    target.draw(reason_);
    target.draw(score_);
    target.draw(warning_);
    for (const auto& button : buttons_) button.draw(target);
}

void BoardGameResultOverlay::select(std::size_t index, bool withSound)
{
    if (withSound && index != selected_) context_.play(audio::SoundId::UiFocus);
    selected_ = index;
    for (std::size_t item = 0; item < buttons_.size(); ++item) buttons_[item].setSelected(item == selected_);
}

BoardGameResultAction BoardGameResultOverlay::actionFor(std::size_t index)
{
    switch (index) {
    case 0: return BoardGameResultAction::Rematch;
    case 1: return BoardGameResultAction::ViewBoard;
    case 2: return BoardGameResultAction::NewSetup;
    default: return BoardGameResultAction::ReturnToLibrary;
    }
}
