#pragma once

#include "AppContext.hpp"
#include "PingPongSession.hpp"
#include "Scene.hpp"
#include "UiButton.hpp"
#include "ProfileService.hpp"
#include "MatchRecorder.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <array>
#include <string>

class PingPongSetupScene final : public Scene {
public:
    PingPongSetupScene(AppContext& context, ping_pong::PingPongSession& session,
                       persistence::ProfileService& profileService, persistence::MatchRecorder& matchRecorder);

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
    void select(std::size_t index, bool withSound);
    void goBack();
    void adjustSelected(int offset);
    void activateSelected();
    void editName(std::string& name, char32_t codepoint);
    void startMatch();
    void refresh();

    AppContext& context_;
    ping_pong::PingPongSession& session_;
    persistence::ProfileService& profileService_;
    persistence::MatchRecorder& matchRecorder_;
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
