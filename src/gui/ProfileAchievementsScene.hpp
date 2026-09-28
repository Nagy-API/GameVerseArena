#pragma once

#include "AchievementService.hpp"
#include "AppContext.hpp"
#include "ProfileService.hpp"
#include "Scene.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/Text.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class ProfileAchievementsScene final : public Scene {
public:
    ProfileAchievementsScene(AppContext& context, persistence::ProfileService& profiles,
                             persistence::AchievementService& achievements,
                             std::int64_t& selectedProfileId);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u) override {}
    void onActivate() override;

private:
    static constexpr std::size_t visibleRows = 4;

    void activate(std::size_t index);
    void select(std::size_t index, bool withSound = false);
    void moveScroll(int direction);
    void rebuildFilter();
    bool matchesFilter(const achievements::AchievementStatus& status) const;

    AppContext& context_;
    SceneManager& scenes_;
    persistence::ProfileService& profiles_;
    persistence::AchievementService& achievements_;
    std::int64_t& selectedProfileId_;
    const sf::Font& regularFont_;
    const sf::Font& semiboldFont_;
    sf::Text kicker_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text summary_;
    sf::Text scrollStatus_;
    std::array<UiButton, 5> buttons_;
    std::vector<achievements::AchievementStatus> statuses_;
    std::vector<achievements::AchievementStatus> filtered_;
    std::size_t category_{};
    std::size_t selected_{};
    std::size_t scroll_{};
};
