#pragma once

#include "AppContext.hpp"
#include "ProfileService.hpp"
#include "Scene.hpp"
#include "StatisticsRepository.hpp"
#include "AchievementService.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/Text.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// A profile's statistics: a scrollable table with an "All games" row and one row per game, and a
// detail card for the selected row. Every number is derived from the completed-match history.
// The selected game is shared with Match History, which opens filtered to it.
class ProfileStatsScene final : public Scene {
public:
    ProfileStatsScene(AppContext& context,
                      persistence::ProfileService& profiles, persistence::StatisticsRepository& statistics,
                      persistence::AchievementService& achievements,
                      std::int64_t& selectedProfileId, std::optional<persistence::GameKey>& selectedGame);
    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u) override {}
    void onActivate() override;

    // Diagnostics for the smoke test.
    std::size_t rowCount() const noexcept { return rows_.size(); }
    std::size_t selectedRow() const noexcept { return selectedRow_; }
    std::size_t firstVisibleRow() const noexcept { return scroll_; }
    std::int64_t profileId() const noexcept { return selectedProfileId_; }
    // The row's game (nullopt for "All games") and its number of matches.
    std::optional<persistence::GameKey> rowGame(std::size_t row) const { return rows_.at(row).game; }
    std::int64_t rowMatches(std::size_t row) const { return rows_.at(row).summary.matches; }
    // The detail card's text for the selected row (UTF-8).
    const std::string& detailText() const noexcept { return detailText_; }

private:
    static constexpr std::size_t visibleRows = 10;

    struct Row {
        std::optional<persistence::GameKey> game;  // nullopt: every game together
        std::string name;
        persistence::GameSummary summary;
        std::array<sf::Text, 5> cells;
    };

    void activate(std::size_t index);
    void select(std::size_t index, bool withSound = false);
    void selectRow(std::size_t row, bool withSound);
    void scrollBy(int rows);
    void layoutRows();
    void showDetail();
    std::optional<std::size_t> rowAt(sf::Vector2f point) const;

    AppContext& context_;
    SceneManager& scenes_;
    persistence::ProfileService& profiles_;
    persistence::StatisticsRepository& statistics_;
    persistence::AchievementService& achievements_;
    std::int64_t& selectedProfileId_;
    std::optional<persistence::GameKey>& selectedGame_;
    std::optional<std::int64_t> shownProfileId_;
    const sf::Font& regular_;
    const sf::Font& semibold_;
    sf::Text kicker_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text achievementSummary_;
    sf::Text error_;
    std::array<sf::Text, 5> headers_;
    sf::Text scrollStatus_;
    sf::Text detailTitle_;
    sf::Text detailBody_;
    std::vector<Row> rows_;
    persistence::OverallStatistics overall_;
    std::array<UiButton, 3> buttons_;
    std::size_t selected_{};
    std::size_t selectedRow_{};
    std::size_t scroll_{};
    std::optional<std::size_t> hoveredRow_;
    std::string detailText_;
    float wheelRemainder_{0.f};
    bool loaded_{false};
};
