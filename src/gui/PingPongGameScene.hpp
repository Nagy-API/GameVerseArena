#pragma once

#include "PingPongAI.hpp"
#include "PingPongPauseOverlay.hpp"
#include "PingPongResultOverlay.hpp"
#include "PingPongSession.hpp"
#include "PingPongSimulation.hpp"
#include "AppContext.hpp"
#include "Scene.hpp"
#include "UiButton.hpp"
#include "MatchRecorder.hpp"
#include "AchievementService.hpp"
#include "AchievementNotificationQueue.hpp"

#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <deque>

class PingPongGameScene final : public Scene {
public:
    PingPongGameScene(AppContext& context, ping_pong::PingPongSession& session,
                      persistence::MatchRecorder& matchRecorder,
                      persistence::AchievementService& achievements,
                      achievements::AchievementNotificationQueue& notifications);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;
    void onActivate() override;

    // Read-only diagnostics used by the GUI smoke test.
    bool paused() const noexcept { return paused_; }
    const ping_pong::SimulationState& simulationState() const noexcept { return simulation_.state(); }

private:
    struct HeldInput {
        bool w{};
        bool s{};
        bool up{};
        bool down{};
    };

    void setKey(sf::Keyboard::Key key, bool held);
    void clearHeldInput() noexcept;
    ping_pong::Movement direction(bool up, bool down) const noexcept;
    ping_pong::ControlInput controlsForStep(double seconds);
    void setPaused(bool paused);
    void restartMatch();
    void handlePauseAction(PingPongPauseAction action);
    void handleResultAction(PingPongResultAction action);
    void recordIfComplete();
    void playMatchResultSound();
    void drawPlayfield(sf::RenderTarget& target) const;
    void drawCentered(sf::RenderTarget& target, const std::string& value, sf::Vector2f center,
                      unsigned int size, const sf::Color& color, bool strong = false) const;

    static constexpr double fixedStep = 1.0 / 120.0;
    static constexpr int maximumCatchUpSteps = 8;

    AppContext& context_;
    ping_pong::PingPongSession& session_;
    persistence::MatchRecorder& matchRecorder_;
    persistence::AchievementService& achievements_;
    achievements::AchievementNotificationQueue& notifications_;
    ping_pong::PingPongSimulation simulation_;
    ping_pong::PingPongAI ai_;
    const sf::Font& regularFont_;
    const sf::Font& semiboldFont_;
    UiButton pauseButton_;
    PingPongPauseOverlay pauseOverlay_;
    PingPongResultOverlay resultOverlay_;
    HeldInput held_{};
    std::deque<ping_pong::Vec2> trail_;
    double accumulator_{};
    bool paused_{false};
};
