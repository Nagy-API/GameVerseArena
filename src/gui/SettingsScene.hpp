#pragma once

#include "AppContext.hpp"
#include "Scene.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <string>

class SettingsScene final : public Scene {
public:
    explicit SettingsScene(AppContext& context);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;
    void onActivate() override;

private:
    static constexpr std::size_t sliderCount = 4;
    static constexpr std::size_t rowCount = 6;
    static constexpr std::size_t resetIndex = 6;
    static constexpr std::size_t backIndex = 7;
    static constexpr std::size_t focusCount = 8;

    void select(std::size_t index, bool withSound);
    void moveSelection(int offset);
    void activateSelected();
    void adjustSelected(int direction, bool fine);
    void setVolume(std::size_t slider, int value, bool persist);
    void toggle(std::size_t row, std::optional<bool> forced = std::nullopt);
    void resetDefaults();
    void goBack();
    void finishDrag();
    int volumeAt(std::size_t slider) const;
    int& volumeRef(persistence::AppSettings& settings, std::size_t slider) const;
    std::optional<int> valueAtTrack(std::size_t slider, float x) const;
    sf::FloatRect rowBounds(std::size_t row) const;
    sf::FloatRect trackBounds(std::size_t slider) const;
    void commit(const persistence::AppSettings& settings, bool persist);
    void refreshStatus();

    AppContext& context_;
    sf::Text kicker_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text help_;
    sf::Text status_;
    sf::Text sideTitle_;
    sf::Text sideBody_;
    std::array<sf::Text, rowCount> labels_;
    std::array<sf::Text, rowCount> values_;
    UiButton resetButton_;
    UiButton backButton_;
    std::size_t selected_{};
    std::optional<std::size_t> dragging_;
    persistence::AppSettings draft_{};
};
