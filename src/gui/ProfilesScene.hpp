#pragma once

#include "ProfileDeleteOverlay.hpp"
#include "ProfileEditOverlay.hpp"
#include "AppContext.hpp"
#include "ProfileService.hpp"
#include "Scene.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

class ProfilesScene final : public Scene {
public:
    ProfilesScene(AppContext& context, persistence::ProfileService& profileService,
                  std::int64_t& selectedStatsProfileId);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;
    void onActivate() override;

private:
    static constexpr std::size_t visibleCount = 6;
    static constexpr std::size_t actionCount = 6;

    void moveProfileSelection(int offset);
    void moveActionSelection(int offset);
    void selectAction(std::size_t action, bool withSound);
    void selectProfile(std::size_t index, bool withSound);
    void activateAction(std::size_t action);
    void reload(std::optional<std::int64_t> preferredId = std::nullopt);
    void refresh();
    std::optional<persistence::Profile> selectedProfile() const;
    bool selectedIsActive() const;

    AppContext& context_;
    persistence::ProfileService& profileService_;
    std::int64_t& selectedStatsProfileId_;
    const sf::Font& regularFont_;
    sf::Text kicker_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text help_;
    sf::Text scrollStatus_;
    sf::Text actionError_;
    std::array<sf::RectangleShape, visibleCount> cards_;
    std::vector<sf::Text> names_;
    std::vector<sf::Text> badges_;
    std::array<UiButton, actionCount> actions_;
    ProfileEditOverlay editOverlay_;
    ProfileDeleteOverlay deleteOverlay_;
    std::vector<persistence::Profile> profiles_;
    std::size_t selectedProfileIndex_{};
    std::size_t firstVisible_{};
    std::size_t selectedAction_{3};
    std::int64_t pendingProfileId_{};
};
