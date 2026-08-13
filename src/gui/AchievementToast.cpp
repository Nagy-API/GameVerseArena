#include "AchievementToast.hpp"

#include "Theme.hpp"
#include "AchievementToastText.hpp"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Mouse.hpp>

AchievementToast::AchievementToast(const sf::Font& regularFont, const sf::Font& semiboldFont,
                                   achievements::AchievementNotificationQueue& queue)
    : regularFont_(regularFont), semiboldFont_(semiboldFont), queue_(queue), panel_({410.f, 128.f})
{
    panel_.setPosition({830.f, 26.f});
    panel_.setFillColor(Theme::backgroundRaised);
    panel_.setOutlineThickness(2.f);
    panel_.setOutlineColor(Theme::warning);
}

bool AchievementToast::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (!queue_.current()) return false;
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left &&
        panel_.getGlobalBounds().contains(window.mapPixelToCoords(click->position))) {
        queue_.dismiss();
        return true;
    }
    return false;
}

void AchievementToast::update(sf::Time deltaTime)
{
    queue_.update(deltaTime.asSeconds());
}

void AchievementToast::render(sf::RenderTarget& target) const
{
    const auto* achievement = queue_.current();
    if (!achievement) return;
    target.draw(panel_);
    sf::Text heading(semiboldFont_, "ACHIEVEMENT UNLOCKED", 14);
    heading.setPosition({854.f, 42.f});
    heading.setFillColor(Theme::warning);
    target.draw(heading);
    sf::Text title(semiboldFont_, achievement->title, 23);
    title.setPosition({854.f, 67.f});
    title.setFillColor(Theme::textPrimary);
    target.draw(title);
    sf::Text description(regularFont_, wrapAchievementToastDescription(achievement->description, 42), 14);
    description.setPosition({854.f, 99.f});
    description.setFillColor(Theme::textSecondary);
    target.draw(description);
    sf::Text dismiss(regularFont_, "Click to dismiss", 12);
    dismiss.setPosition({1120.f, 132.f});
    dismiss.setFillColor(Theme::textMuted);
    target.draw(dismiss);
}
