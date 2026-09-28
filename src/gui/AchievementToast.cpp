#include "AchievementToast.hpp"

#include "Theme.hpp"
#include "AchievementToastText.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>

namespace {
constexpr sf::Vector2f restingPosition{830.f, 26.f};
constexpr float slideDistance = 460.f;
constexpr float slideSeconds = 0.22f;
} // namespace

AchievementToast::AchievementToast(AppContext& context, achievements::AchievementNotificationQueue& queue)
    : context_(context), queue_(queue), panel_({410.f, 128.f})
{
    panel_.setPosition(restingPosition);
    panel_.setFillColor(Theme::backgroundRaised);
    panel_.setOutlineThickness(2.f);
    panel_.setOutlineColor(Theme::warning);
}

bool AchievementToast::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    const auto insideToast = [&](sf::Vector2i pixel) {
        auto bounds = panel_.getGlobalBounds();
        bounds.position.x += slideOffset();
        return bounds.contains(window.mapPixelToCoords(pixel));
    };
    // A click dismisses the toast only when both the press and the release happen on it, so
    // a drag that started in the scene (for example a volume slider) always gets its release.
    if (const auto* press = event.getIf<sf::Event::MouseButtonPressed>();
        press && press->button == sf::Mouse::Button::Left) {
        pressStartedOnToast_ = queue_.current() != nullptr && insideToast(press->position);
        return pressStartedOnToast_;
    }
    if (const auto* release = event.getIf<sf::Event::MouseButtonReleased>();
        release && release->button == sf::Mouse::Button::Left) {
        const bool dismiss = pressStartedOnToast_ && queue_.current() != nullptr && insideToast(release->position);
        const bool consumed = pressStartedOnToast_;
        pressStartedOnToast_ = false;
        if (dismiss) queue_.dismiss();
        return consumed;
    }
    return false;
}

void AchievementToast::update(sf::Time deltaTime)
{
    queue_.update(deltaTime.asSeconds());
    if (queue_.current() && queue_.sequence() != shownSequence_) {
        // A new notification became visible: start its (optional) slide-in and play the
        // unlock chime exactly once.
        shownSequence_ = queue_.sequence();
        slide_ = context_.reducedMotion() ? 1.f : 0.f;
        context_.play(audio::SoundId::AchievementUnlocked);
    }
    if (context_.reducedMotion()) slide_ = 1.f;
    else slide_ = std::min(1.f, slide_ + deltaTime.asSeconds() / slideSeconds);
}

float AchievementToast::slideOffset() const noexcept
{
    const float eased = 1.f - (1.f - slide_) * (1.f - slide_);
    return (1.f - eased) * slideDistance;
}

void AchievementToast::render(sf::RenderTarget& target) const
{
    const auto* achievement = queue_.current();
    if (!achievement) return;
    const float offset = slideOffset();
    sf::RectangleShape panel = panel_;
    panel.move({offset, 0.f});
    target.draw(panel);
    sf::Text heading(context_.semiboldFont, "ACHIEVEMENT UNLOCKED", 14);
    heading.setPosition({854.f + offset, 42.f});
    heading.setFillColor(Theme::warning);
    target.draw(heading);
    sf::Text title(context_.semiboldFont, achievement->title, 23);
    title.setPosition({854.f + offset, 67.f});
    title.setFillColor(Theme::textPrimary);
    target.draw(title);
    sf::Text description(context_.regularFont, wrapAchievementToastDescription(achievement->description, 42), 14);
    description.setPosition({854.f + offset, 99.f});
    description.setFillColor(Theme::textSecondary);
    target.draw(description);
    sf::Text dismiss(context_.regularFont, "Click to dismiss", 12);
    dismiss.setPosition({1120.f + offset, 132.f});
    dismiss.setFillColor(Theme::textMuted);
    target.draw(dismiss);
}
