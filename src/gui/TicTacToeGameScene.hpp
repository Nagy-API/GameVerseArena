#pragma once

#include "Scene.hpp"
#include "SceneManager.hpp"
#include "TicTacToeAI.hpp"
#include "TicTacToeResultOverlay.hpp"
#include "TicTacToeSession.hpp"
#include "UiButton.hpp"
#include "MatchRecorder.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <optional>

class TicTacToeGameScene final : public Scene {
public:
    TicTacToeGameScene(const sf::Font& regularFont, const sf::Font& semiboldFont,
                       SceneManager& sceneManager, classic_ttt::TicTacToeSession& session,
                       persistence::MatchRecorder& matchRecorder);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;
    void onActivate() override;

private:
    std::optional<classic_ttt::Position> cellAt(sf::Vector2f point) const;
    bool humanMayPlay() const;
    void tryMove(classic_ttt::Position position);
    void scheduleAI();
    void cancelAI();
    void performAI();
    void handleResultAction(TicTacToeResultAction action);
    void resetVisualState();
    void requestLibraryExit();
    void updateButtonStates();
    void recordIfComplete();
    void drawBoard(sf::RenderTarget& target) const;
    void drawExitConfirmation(sf::RenderTarget& target) const;

    SceneManager& sceneManager_;
    classic_ttt::TicTacToeSession& session_;
    persistence::MatchRecorder& matchRecorder_;
    classic_ttt::TicTacToeAI ai_;
    const sf::Font& regularFont_;
    const sf::Font& semiboldFont_;
    UiButton backButton_;
    UiButton restartButton_;
    UiButton newMatchButton_;
    UiButton exitYesButton_;
    UiButton exitNoButton_;
    TicTacToeResultOverlay resultOverlay_;
    sf::Vector2f boardPosition_{445.f, 205.f};
    classic_ttt::Position selectedCell_{1, 1};
    std::optional<classic_ttt::Position> hoveredCell_;
    std::optional<classic_ttt::Position> lastMove_;
    float markAnimation_{1.f};
    float selectionPulse_{};
    float aiElapsed_{};
    bool aiPending_{false};
    bool exitConfirmation_{false};
    bool exitYesSelected_{false};
};
