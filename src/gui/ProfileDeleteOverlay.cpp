#include "ProfileDeleteOverlay.hpp"

#include "Theme.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

ProfileDeleteOverlay::ProfileDeleteOverlay(AppContext& context)
    : context_(context), backdrop_(Theme::logicalSize), panel_({620.f, 300.f}),
      title_(context.semiboldFont, "Delete profile", 32), question_(context.regularFont, "", 23),
      explanation_(context.regularFont, "", 17),
      deleteButton_(context.semiboldFont, "Delete", {190.f, 54.f}),
      cancelButton_(context.semiboldFont, "Cancel", {190.f, 54.f})
{
    backdrop_.setFillColor(Theme::overlay);
    panel_.setPosition({330.f, 210.f}); panel_.setFillColor(Theme::backgroundRaised);
    panel_.setOutlineThickness(1.f); panel_.setOutlineColor(Theme::border);
    title_.setPosition({378.f, 250.f}); title_.setFillColor(Theme::danger);
    question_.setPosition({378.f, 314.f}); question_.setFillColor(Theme::textPrimary);
    explanation_.setPosition({378.f, 359.f}); explanation_.setFillColor(Theme::textSecondary);
    deleteButton_.setPosition({378.f, 421.f}); cancelButton_.setPosition({712.f, 421.f});
}

void ProfileDeleteOverlay::open(std::string profileName, bool activeProfile)
{
    profileName_ = std::move(profileName);
    activeProfile_ = activeProfile;
    selectedButton_ = 1;
    open_ = true;
    deleteButton_.setHovered(false);
    cancelButton_.setHovered(false);
    refresh();
}

ProfileDeleteOverlay::Result ProfileDeleteOverlay::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (!open_) return Result::None;
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) {
            open_ = false; context_.play(audio::SoundId::UiBack); return Result::Cancel;
        }
        if (key->code == sf::Keyboard::Key::Tab || key->code == sf::Keyboard::Key::Left ||
            key->code == sf::Keyboard::Key::Right) {
            selectedButton_ = 1 - selectedButton_; refresh();
            context_.play(audio::SoundId::UiFocus);
        } else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) {
            if (selectedButton_ == 0) return Result::Confirm;
            open_ = false; context_.play(audio::SoundId::UiBack); return Result::Cancel;
        }
    }
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        deleteButton_.setHovered(deleteButton_.contains(point));
        cancelButton_.setHovered(cancelButton_.contains(point));
        const auto previous = selectedButton_;
        if (deleteButton_.contains(point)) selectedButton_ = 0;
        if (cancelButton_.contains(point)) selectedButton_ = 1;
        if (previous != selectedButton_) context_.play(audio::SoundId::UiFocus);
        refresh();
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        if (deleteButton_.contains(point)) return Result::Confirm;
        if (cancelButton_.contains(point)) { open_ = false; context_.play(audio::SoundId::UiBack); return Result::Cancel; }
    }
    return Result::None;
}

void ProfileDeleteOverlay::update(sf::Time deltaTime)
{
    if (open_) {
        deleteButton_.update(deltaTime, context_.reducedMotion());
        cancelButton_.update(deltaTime, context_.reducedMotion());
    }
}

void ProfileDeleteOverlay::render(sf::RenderWindow& window) const
{
    if (!open_) return;
    window.draw(backdrop_); window.draw(panel_); window.draw(title_); window.draw(question_);
    window.draw(explanation_); deleteButton_.draw(window); cancelButton_.draw(window);
}

void ProfileDeleteOverlay::refresh()
{
    question_.setString("Delete \"" + profileName_ + "\"?");
    explanation_.setString(activeProfile_
        ? "This is active; a replacement is chosen automatically.\nIts match history and achievements are deleted too."
        : "It is removed from this device together with\nits match history and achievements.");
    deleteButton_.setSelected(selectedButton_ == 0);
    cancelButton_.setSelected(selectedButton_ == 1);
}
