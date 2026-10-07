#pragma once

#include "AppContext.hpp"
#include "MatchRepository.hpp"
#include "ProfileService.hpp"
#include "Scene.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/Text.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

// A profile's completed matches, newest first, eight per page, filtered by game (any of the 15
// games) and result. The game filter is shared with Statistics, which opens this scene filtered
// to the game selected there.
class MatchHistoryScene final : public Scene {
public:
    MatchHistoryScene(AppContext& context,
        persistence::ProfileService& profiles, persistence::MatchRepository& matches, std::int64_t& selectedProfileId,
        std::optional<persistence::GameKey>& selectedGame);
    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u) override {}
    void onActivate() override;

    // Diagnostics for the smoke test.
    std::optional<persistence::GameKey> gameFilter() const noexcept { return filter_.game; }
    std::int64_t total() const noexcept { return total_; }

private:
    static constexpr std::size_t pageSize = 8;
    void reload();
    void refresh();
    void activate(std::size_t index);
    void select(std::size_t index, bool withSound = false);
    // Moves the game (index 0) or result (index 1) filter to its next or previous choice.
    void stepFilter(std::size_t index, int direction);

    AppContext& context_;
    SceneManager& scenes_;
    persistence::ProfileService& profiles_;
    persistence::MatchRepository& matches_;
    std::int64_t& profileId_;
    std::optional<persistence::GameKey>& selectedGame_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text status_;
    sf::Text empty_;
    sf::Text help_;
    std::vector<sf::Text> rows_;
    std::array<UiButton, 5> buttons_;
    persistence::MatchFilter filter_;
    std::optional<std::int64_t> shownProfileId_;
    std::size_t resultFilter_{};
    std::size_t page_{};
    std::size_t selected_{};
    std::int64_t total_{};
};
