#pragma once

#include "ProfileService.hpp"
#include "Scene.hpp"
#include "SceneManager.hpp"
#include "StatisticsRepository.hpp"
#include "AchievementService.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/Text.hpp>

#include <array>
#include <cstdint>

class ProfileStatsScene final : public Scene {
public:
    ProfileStatsScene(const sf::Font& regularFont, const sf::Font& semiboldFont, SceneManager& scenes,
                      persistence::ProfileService& profiles, persistence::StatisticsRepository& statistics,
                      persistence::AchievementService& achievements,
                      std::int64_t& selectedProfileId);
    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u) override {}
    void onActivate() override;
private:
    void activate(std::size_t index);
    void select(std::size_t index);
    SceneManager& scenes_;
    persistence::ProfileService& profiles_;
    persistence::StatisticsRepository& statistics_;
    persistence::AchievementService& achievements_;
    std::int64_t& selectedProfileId_;
    const sf::Font& regular_;
    sf::Text kicker_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text empty_;
    sf::Text achievementSummary_;
    std::array<sf::Text, 3> cards_;
    std::array<UiButton, 3> buttons_;
    std::size_t selected_{};
};
