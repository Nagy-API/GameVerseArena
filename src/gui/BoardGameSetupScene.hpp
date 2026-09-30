#pragma once

#include "AppContext.hpp"
#include "BoardGameHost.hpp"
#include "MatchRecorder.hpp"
#include "ProfileService.hpp"
#include "Scene.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <array>
#include <string>
#include <vector>

// Player setup for every shared board game: mode, names, and (against the computer) which side
// the human plays, next to the game's rules. Starting creates the game, its board view, and
// arms the match recorder for the active profile.
class BoardGameSetupScene final : public Scene {
public:
    BoardGameSetupScene(AppContext& context, BoardGameHost& host, persistence::ProfileService& profileService,
                        persistence::MatchRecorder& matchRecorder);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;
    void onActivate() override;

    // Diagnostics for the smoke test.
    bool humanVsComputer() const noexcept { return mode_ == 1; }
    bool humanPlaysFirst() const noexcept { return humanSeat_ == 0; }

private:
    static constexpr std::size_t rowCount = 4;
    static constexpr std::size_t startIndex = 4;
    static constexpr std::size_t backIndex = 5;

    const catalogue::GameDescriptor& game() const { return *host_.game; }
    void moveSelection(int offset);
    void select(std::size_t index, bool withSound);
    void goBack();
    void adjustSelected(int offset);
    void activateSelected();
    void startGame();
    void refresh();
    void refreshRules();
    void editName(std::string& name, char32_t codepoint);
    bool nameEditable(std::size_t row) const;

    AppContext& context_;
    BoardGameHost& host_;
    persistence::ProfileService& profileService_;
    persistence::MatchRecorder& matchRecorder_;
    sf::Text kicker_;
    sf::Text title_;
    sf::Text subtitle_;
    sf::Text help_;
    sf::Text message_;
    std::array<sf::RectangleShape, rowCount> rows_;
    std::vector<sf::Text> labels_;
    std::vector<sf::Text> values_;
    sf::RectangleShape rulesPanel_;
    sf::Text rulesHeading_;
    sf::Text rulesText_;
    sf::Text computerHeading_;
    sf::Text computerText_;
    UiButton startButton_;
    UiButton backButton_;
    std::size_t selected_{};
    bool editing_{false};
    int mode_{0};       // 0 = human vs human, 1 = human vs computer
    int humanSeat_{0};  // against the computer: 0 = the human moves first
    std::string playerOne_{"Player 1"};
    std::string playerTwo_{"Player 2"};
};
