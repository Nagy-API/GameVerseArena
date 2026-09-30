#pragma once

#include "AchievementNotificationQueue.hpp"
#include "AchievementService.hpp"
#include "AppContext.hpp"
#include "BoardGameHost.hpp"
#include "BoardGameResultOverlay.hpp"
#include "BoardGameRulesOverlay.hpp"
#include "ComputerMoveTask.hpp"
#include "MatchRecorder.hpp"
#include "Scene.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Text.hpp>

#include <random>
#include <string>

// The shared scene that plays every migrated board game. It owns turn flow and everything common
// to the games: seat panels, the computer's background search (with a short minimum "thinking"
// pause), sounds, the rules panel, leaving with confirmation, the result panel, and recording
// each finished game exactly once. The game's BoardView draws the board and turns input into moves.
class BoardGameScene final : public Scene {
public:
    enum class Focus { Board, Restart, Rules, Back };

    BoardGameScene(AppContext& context, BoardGameHost& host, persistence::MatchRecorder& matchRecorder,
                   persistence::AchievementService& achievements,
                   achievements::AchievementNotificationQueue& notifications);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;
    void onActivate() override;

    // Where board views are laid out (logical coordinates).
    static sf::FloatRect boardArea();

    // Diagnostics for the smoke test.
    bool resultVisible() const noexcept { return resultOverlay_.visible(); }
    bool reviewingBoard() const noexcept { return reviewingBoard_; }
    bool exitConfirmationVisible() const noexcept { return exitConfirmation_; }
    bool rulesVisible() const noexcept { return rulesOverlay_.visible(); }
    bool computerThinking() const noexcept { return computer_.running(); }
    Focus focus() const noexcept { return focus_; }

private:
    bool ready() const;
    bool gameFinished() const;
    bool humanMayPlay() const;
    void applyResponse(const board_view::Response& response);
    void applyMove(turn_based::MoveId move);
    void startComputerIfNeeded();
    void pollComputer(float seconds);
    void computerFailed(const std::string& detail);
    void finishGame(const turn_based::Outcome& outcome);
    void recordCompletion(const turn_based::Outcome& outcome);
    void restartGame();
    void requestExit();
    void leaveTo(SceneId scene);
    void showRules();
    void showResultAgain();
    void handleResultAction(BoardGameResultAction action);
    void handleExitConfirmation(const sf::Event& event, sf::RenderWindow& window);
    void setFocus(Focus focus, bool withSound);
    void activateFocus();
    void updateButtonStates();
    std::string statusText() const;
    void drawCenteredLine(sf::RenderTarget& target, const std::string& text, float y, unsigned int size,
                          const sf::Color& color, bool strong) const;
    void drawSeatPanel(sf::RenderTarget& target, turn_based::Seat seat, sf::Vector2f position) const;
    void drawExitConfirmation(sf::RenderTarget& target) const;

    AppContext& context_;
    BoardGameHost& host_;
    persistence::MatchRecorder& matchRecorder_;
    persistence::AchievementService& achievements_;
    achievements::AchievementNotificationQueue& notifications_;
    UiButton backButton_;
    UiButton rulesButton_;
    UiButton restartButton_;
    UiButton exitYesButton_;
    UiButton exitNoButton_;
    BoardGameResultOverlay resultOverlay_;
    BoardGameRulesOverlay rulesOverlay_;
    turn_based::ComputerMoveTask computer_;
    std::mt19937 seeds_;
    float computerElapsed_{};
    bool computerStuck_{false};
    std::string errorMessage_;
    Focus focus_{Focus::Board};
    bool exitConfirmation_{false};
    bool exitYesSelected_{false};
    bool reviewingBoard_{false};
    float time_{};
};
