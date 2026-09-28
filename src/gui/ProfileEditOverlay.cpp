#include "ProfileEditOverlay.hpp"

#include "Theme.hpp"
#include "Utf8Text.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

namespace {
std::string visibleValue(const std::string& value)
{
    return value;
}

} // namespace

ProfileEditOverlay::ProfileEditOverlay(AppContext& context)
    : context_(context), backdrop_(Theme::logicalSize), panel_({650.f, 360.f}), field_({554.f, 64.f}),
      title_(context.semiboldFont, "Create profile", 32),
      prompt_(context.semiboldFont, "DISPLAY NAME", Theme::labelSize),
      valueText_(context.regularFont, "", 22), errorText_(context.regularFont, "", 16),
      help_(context.regularFont, "Type a name (24 characters max)  |  Enter confirms  |  Escape cancels", 14),
      confirmButton_(context.semiboldFont, "Create", {210.f, 54.f}),
      cancelButton_(context.semiboldFont, "Cancel", {180.f, 54.f})
{
    backdrop_.setFillColor(Theme::overlay);
    panel_.setPosition({315.f, 180.f}); panel_.setFillColor(Theme::backgroundRaised);
    panel_.setOutlineThickness(1.f); panel_.setOutlineColor(Theme::border);
    title_.setPosition({363.f, 220.f}); title_.setFillColor(Theme::textPrimary);
    prompt_.setPosition({363.f, 281.f}); prompt_.setFillColor(Theme::textMuted);
    field_.setPosition({363.f, 309.f}); field_.setFillColor(Theme::panel);
    field_.setOutlineThickness(2.f); field_.setOutlineColor(Theme::primaryBright);
    valueText_.setPosition({384.f, 327.f}); valueText_.setFillColor(Theme::textPrimary);
    errorText_.setPosition({363.f, 383.f}); errorText_.setFillColor(Theme::danger);
    help_.setPosition({363.f, 420.f}); help_.setFillColor(Theme::textMuted);
    confirmButton_.setPosition({363.f, 463.f}); cancelButton_.setPosition({737.f, 463.f});
}

void ProfileEditOverlay::open(Mode mode, std::string initialValue)
{
    mode_ = mode;
    value_ = std::move(initialValue);
    error_.clear();
    selectedButton_ = 0;
    open_ = true;
    confirmButton_.setHovered(false);
    cancelButton_.setHovered(false);
    refresh();
}

void ProfileEditOverlay::setError(std::string error)
{
    error_ = std::move(error);
    context_.play(audio::SoundId::UiError);
    refresh();
}

ProfileEditOverlay::Result ProfileEditOverlay::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (!open_) return Result::None;
    if (const auto* text = event.getIf<sf::Event::TextEntered>(); text && text->unicode >= 32) {
        if (utf8_text::appendPrintable(value_, text->unicode, 24)) {
            error_.clear();
            refresh();
        } else {
            context_.play(audio::SoundId::UiError);
        }
    }
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) {
            open_ = false;
            context_.play(audio::SoundId::UiBack);
            return Result::Cancel;
        }
        if (key->code == sf::Keyboard::Key::Backspace && !value_.empty()) {
            utf8_text::eraseLast(value_); error_.clear(); refresh();
        } else if (key->code == sf::Keyboard::Key::Tab || key->code == sf::Keyboard::Key::Left ||
                   key->code == sf::Keyboard::Key::Right) {
            selectedButton_ = 1 - selectedButton_; refresh();
            context_.play(audio::SoundId::UiFocus);
        } else if (key->code == sf::Keyboard::Key::Enter) {
            if (selectedButton_ == 0) return Result::Confirm;
            open_ = false;
            context_.play(audio::SoundId::UiBack);
            return Result::Cancel;
        }
    }
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        confirmButton_.setHovered(confirmButton_.contains(point));
        cancelButton_.setHovered(cancelButton_.contains(point));
        const auto previous = selectedButton_;
        if (confirmButton_.contains(point)) selectedButton_ = 0;
        if (cancelButton_.contains(point)) selectedButton_ = 1;
        if (previous != selectedButton_) context_.play(audio::SoundId::UiFocus);
        refresh();
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        if (confirmButton_.contains(point)) return Result::Confirm;
        if (cancelButton_.contains(point)) { open_ = false; context_.play(audio::SoundId::UiBack); return Result::Cancel; }
    }
    return Result::None;
}

void ProfileEditOverlay::update(sf::Time deltaTime)
{
    if (open_) {
        confirmButton_.update(deltaTime, context_.reducedMotion());
        cancelButton_.update(deltaTime, context_.reducedMotion());
    }
}

void ProfileEditOverlay::render(sf::RenderWindow& window) const
{
    if (!open_) return;
    window.draw(backdrop_); window.draw(panel_); window.draw(title_); window.draw(prompt_);
    window.draw(field_); window.draw(valueText_); window.draw(errorText_); window.draw(help_);
    confirmButton_.draw(window); cancelButton_.draw(window);
}

void ProfileEditOverlay::refresh()
{
    title_.setString(mode_ == Mode::Create ? "Create profile" : "Rename profile");
    confirmButton_.setText(mode_ == Mode::Create ? "Create" : "Save");
    valueText_.setString(visibleValue(value_) + " |");
    errorText_.setString(error_);
    confirmButton_.setSelected(selectedButton_ == 0);
    cancelButton_.setSelected(selectedButton_ == 1);
}
