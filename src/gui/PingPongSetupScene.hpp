#pragma once

#include "PingPongSession.hpp"
#include "Scene.hpp"
#include "SceneManager.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <array>
#include <string>

class PingPongSetupScene final : public Scene {
public:
    PingPongSetupScene(const sf::Font& regularFont, const sf::Font& semiboldFont,
                       SceneManager& sceneManager, ping_pong::PingPongSession& session);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;
    void onActivate() override;

private:
    static constexpr std::size_t rowCount = 4;
    static constexpr std::size_t startIndex = 4;
    static constexpr std::size_t backIndex = 5;

    void moveSelection(int offset);
    void adjustSelected(int offset);
    void activateSelected();
    void editName(std::string& name, char32_t codepoint);
    void startMatch();
    void refresh();

    SceneManager& sceneManager_;
    ping_pong::PingPongSession& session_;
    sf::Text kicker_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text rules_;
    sf::Text help_;
    std::array<sf::RectangleShape, rowCount> rows_;
    std::array<sf::Text, rowCount> labels_;
    std::array<sf::Text, rowCount> values_;
    UiButton startButton_;
    UiButton backButton_;
    std::size_t selected_{};
    bool editing_{false};
    int mode_{};
    int difficulty_{1};
    std::string playerOne_{"Player 1"};
    std::string playerTwo_{"Player 2"};
};
