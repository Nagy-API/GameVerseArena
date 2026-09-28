#pragma once

#include "AchievementNotificationQueue.hpp"
#include "AppContext.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>

#include <cstdint>

class AchievementToast {
public:
    AchievementToast(AppContext& context, achievements::AchievementNotificationQueue& queue);

    bool handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(sf::Time deltaTime);
    void render(sf::RenderTarget& target) const;

private:
    float slideOffset() const noexcept;

    AppContext& context_;
    achievements::AchievementNotificationQueue& queue_;
    sf::RectangleShape panel_;
    std::uint64_t shownSequence_{};
    float slide_{1.f};
    bool pressStartedOnToast_{false};
};
