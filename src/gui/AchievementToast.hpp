#pragma once

#include "AchievementNotificationQueue.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>

class AchievementToast {
public:
    AchievementToast(const sf::Font& regularFont, const sf::Font& semiboldFont,
                     achievements::AchievementNotificationQueue& queue);

    bool handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(sf::Time deltaTime);
    void render(sf::RenderTarget& target) const;

private:
    const sf::Font& regularFont_;
    const sf::Font& semiboldFont_;
    achievements::AchievementNotificationQueue& queue_;
    sf::RectangleShape panel_;
};
