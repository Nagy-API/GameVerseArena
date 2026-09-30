#pragma once

#include "AppContext.hpp"
#include "GameCatalogue.hpp"
#include "GameLauncher.hpp"
#include "Scene.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

// Searchable, filterable, scrollable grid of every catalogue game.
class GameLibraryScene final : public Scene {
public:
    enum class Zone { Search, Categories, Grid, Back };

    GameLibraryScene(AppContext& context, GameLauncher& launcher);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;
    void onActivate() override;

    // Read-only diagnostics used by the GUI smoke test.
    std::size_t resultCount() const noexcept { return results_.size(); }
    std::size_t firstVisibleRow() const noexcept { return firstRow_; }
    std::size_t selectedIndex() const noexcept { return selected_; }
    Zone focusZone() const noexcept { return zone_; }
    const std::string& query() const noexcept { return query_; }
    bool hasMessage() const { return !message_.getString().isEmpty(); }

private:
    static constexpr std::size_t columns = 3;
    static constexpr std::size_t visibleRows = 3;

    void applyFilter();
    void setZone(Zone zone, bool withSound);
    void selectCard(std::size_t index, bool withSound);
    void moveInGrid(int dx, int dy);
    void moveByPage(int pages);
    void scrollRows(int rows);
    void ensureSelectionVisible();
    void setCategory(std::size_t category);
    void activate();
    void launch(std::size_t index);
    void goBack();
    void editQuery(char32_t codepoint);
    void eraseQuery();
    std::size_t rowCount() const;
    std::size_t maximumFirstRow() const;
    sf::FloatRect cardBounds(std::size_t index) const;
    std::optional<std::size_t> cardAt(sf::Vector2f point) const;
    const std::string& cachedText(const std::string& cacheKey, const std::string& source, bool semibold,
                                  unsigned int size, float width, std::size_t lines) const;
    void drawCard(sf::RenderTarget& target, std::size_t index) const;
    void drawRing(sf::RenderTarget& target, sf::FloatRect bounds, const sf::Color& color) const;

    AppContext& context_;
    GameLauncher& launcher_;
    sf::Text kicker_;
    sf::Text title_;
    sf::Text summary_;
    sf::Text searchLabel_;
    sf::Text searchText_;
    sf::Text emptyState_;
    sf::Text scrollStatus_;
    sf::Text help_;
    sf::Text message_;
    sf::RectangleShape searchField_;
    std::array<UiButton, 3> categoryButtons_;
    UiButton backButton_;
    std::string query_;
    std::size_t category_{0};
    std::vector<const catalogue::GameDescriptor*> results_;
    std::size_t selected_{0};
    std::size_t firstRow_{0};
    Zone zone_{Zone::Grid};
    std::optional<std::size_t> hovered_;
    std::optional<std::size_t> hoveredCategory_;
    float wheelRemainder_{0.f};
    mutable std::map<std::string, std::string> textCache_;
};
