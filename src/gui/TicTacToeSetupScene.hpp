#pragma once

#include "AppContext.hpp"
#include "Scene.hpp"
#include "TicTacToeSession.hpp"
#include "UiButton.hpp"
#include "ProfileService.hpp"
#include "MatchRecorder.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <array>
#include <string>
#include <vector>

class TicTacToeSetupScene final : public Scene {
public:
    TicTacToeSetupScene(AppContext& context, classic_ttt::TicTacToeSession& session,
                        persistence::ProfileService& profileService, persistence::MatchRecorder& matchRecorder);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;
    void onActivate() override;

private:
    static constexpr std::size_t rowCount = 6;
    static constexpr std::size_t startIndex = 6;
    static constexpr std::size_t backIndex = 7;

    void moveSelection(int offset);
    void select(std::size_t index, bool withSound);
    void goBack();
    void adjustSelected(int offset);
    void activateSelected();
    void startMatch();
    void refresh();
    void editName(std::string& name, char32_t codepoint);

    AppContext& context_;
    classic_ttt::TicTacToeSession& session_;
    persistence::ProfileService& profileService_;
    persistence::MatchRecorder& matchRecorder_;
    const sf::Font& regularFont_;
    const sf::Font& semiboldFont_;
    sf::Text kicker_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text help_;
    std::array<sf::RectangleShape, rowCount> rows_;
    std::vector<sf::Text> labels_;
    std::vector<sf::Text> values_;
    UiButton startButton_;
    UiButton backButton_;
    std::size_t selected_{};
    bool editing_{false};
    int mode_{};
    int humanMark_{};
    int difficulty_{1};
    int bestOf_{};
    std::string playerOne_{"Player 1"};
    std::string playerTwo_{"Player 2"};
};
