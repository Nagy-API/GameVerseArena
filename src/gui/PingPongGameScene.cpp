#include "PingPongGameScene.hpp"

#include "Theme.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

using namespace ping_pong;

PingPongGameScene::PingPongGameScene(const sf::Font& regularFont, const sf::Font& semiboldFont,
                                     SceneManager& sceneManager, PingPongSession& session,
                                     persistence::MatchRecorder& matchRecorder,
                                     persistence::AchievementService& achievements,
                                     achievements::AchievementNotificationQueue& notifications)
    : sceneManager_(sceneManager), session_(session), matchRecorder_(matchRecorder), achievements_(achievements),
      notifications_(notifications), regularFont_(regularFont), semiboldFont_(semiboldFont),
      pauseButton_(semiboldFont, "Pause", {150.f, 48.f}),
      pauseOverlay_(regularFont, semiboldFont), resultOverlay_(regularFont, semiboldFont)
{
    pauseButton_.setPosition({42.f, 28.f});
}

void PingPongGameScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (event.is<sf::Event::FocusLost>()) {
        clearHeldInput();
        if (!resultOverlay_.visible()) setPaused(true);
        return;
    }
    if (event.is<sf::Event::FocusGained>()) {
        clearHeldInput();
        return;
    }
    if (resultOverlay_.visible()) {
        handleResultAction(resultOverlay_.handleEvent(event, window));
        return;
    }
    if (paused_) {
        handlePauseAction(pauseOverlay_.handleEvent(event, window));
        return;
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) { setPaused(true); return; }
        setKey(key->code, true);
    }
    if (const auto* key = event.getIf<sf::Event::KeyReleased>()) setKey(key->code, false);

    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        pauseButton_.setHovered(pauseButton_.contains(window.mapPixelToCoords(moved->position)));
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left &&
        pauseButton_.contains(window.mapPixelToCoords(click->position))) setPaused(true);
}

void PingPongGameScene::update(sf::Time deltaTime)
{
    pauseButton_.update(deltaTime);
    if (resultOverlay_.visible()) { resultOverlay_.update(deltaTime); return; }
    if (paused_) { pauseOverlay_.update(deltaTime); return; }

    accumulator_ += std::min(0.1, static_cast<double>(deltaTime.asSeconds()));
    int steps = 0;
    while (accumulator_ >= fixedStep && steps < maximumCatchUpSteps) {
        accumulator_ -= fixedStep;
        ++steps;
        session_.update(fixedStep);
        if (session_.consumePointResetRequest()) {
            simulation_.resetForServe();
            ai_.reset();
            trail_.clear();
        }
        if (session_.state() != MatchState::Playing) continue;

        if (const auto scorer = simulation_.step(fixedStep, controlsForStep(fixedStep))) {
            session_.awardPoint(*scorer);
            if (session_.matchFinished()) { resultOverlay_.show(session_); recordIfComplete(); }
        }
        trail_.push_back(simulation_.state().ball.position);
        while (trail_.size() > 8) trail_.pop_front();
    }
    if (steps == maximumCatchUpSteps && accumulator_ >= fixedStep) accumulator_ = 0.0;
}

void PingPongGameScene::render(sf::RenderWindow& window) const
{
    pauseButton_.draw(window);
    drawCentered(window, "ARCADE GAMES  /  PING PONG", {640.f, 37.f}, Theme::labelSize, Theme::arcadeRight, true);
    const auto& score = session_.score();
    drawCentered(window, session_.playerName(Side::Left), {470.f, 76.f}, 19, Theme::textSecondary, true);
    drawCentered(window, std::to_string(score.left), {570.f, 91.f}, 48, Theme::arcadeLeft, true);
    drawCentered(window, ":", {640.f, 91.f}, 38, Theme::textMuted, true);
    drawCentered(window, std::to_string(score.right), {710.f, 91.f}, 48, Theme::arcadeRight, true);
    drawCentered(window, session_.playerName(Side::Right), {810.f, 76.f}, 19, Theme::textSecondary, true);
    drawCentered(window, "FIRST TO 5", {1120.f, 48.f}, Theme::labelSize, Theme::textMuted, true);

    drawPlayfield(window);
    if (session_.state() == MatchState::ServeCountdown) {
        const int count = std::max(1, static_cast<int>(std::ceil(session_.stateSecondsRemaining())));
        drawCentered(window, "SERVE IN", {640.f, 300.f}, Theme::labelSize, Theme::textSecondary, true);
        drawCentered(window, std::to_string(count), {640.f, 345.f}, 64, Theme::textPrimary, true);
    } else if (session_.state() == MatchState::PointScored && session_.lastScorer()) {
        drawCentered(window, session_.playerName(*session_.lastScorer()) + " scores", {640.f, 370.f}, 32,
                     *session_.lastScorer() == Side::Left ? Theme::arcadeLeft : Theme::arcadeRight, true);
    }

    const int speedPercent = static_cast<int>(std::lround(simulation_.state().ballSpeed /
                                                          PingPongSimulation::baseBallSpeed * 100.0));
    const std::string controls = session_.config().mode == GameMode::HumanVsComputer
        ? "W / S or Arrows move  |  Escape pauses"
        : "W / S left  |  Arrows right  |  Escape pauses";
    drawCentered(window, controls, {640.f, 681.f}, Theme::labelSize, Theme::textMuted);
    drawCentered(window, "RALLY SPEED  " + std::to_string(speedPercent) + "%", {1090.f, 681.f}, 13,
                 Theme::textMuted, true);
    if (paused_) pauseOverlay_.draw(window);
    resultOverlay_.draw(window);
}

void PingPongGameScene::onResize(sf::Vector2u) {}

void PingPongGameScene::onActivate()
{
    clearHeldInput();
    accumulator_ = 0.0;
    paused_ = false;
    resultOverlay_.hide();
    session_.consumePointResetRequest();
    simulation_.resetForServe();
    ai_.reset();
    trail_.clear();
}

void PingPongGameScene::setKey(sf::Keyboard::Key key, bool held)
{
    if (key == sf::Keyboard::Key::W) held_.w = held;
    else if (key == sf::Keyboard::Key::S) held_.s = held;
    else if (key == sf::Keyboard::Key::Up) held_.up = held;
    else if (key == sf::Keyboard::Key::Down) held_.down = held;
}

void PingPongGameScene::clearHeldInput() noexcept { held_ = {}; }

Movement PingPongGameScene::direction(bool up, bool down) const noexcept
{
    if (up == down) return Movement::None;
    return up ? Movement::Up : Movement::Down;
}

ControlInput PingPongGameScene::controlsForStep(double seconds)
{
    ControlInput input;
    if (session_.config().mode == GameMode::HumanVsComputer) {
        input.left = direction(held_.w || held_.up, held_.s || held_.down);
        const auto command = ai_.decide(seconds, simulation_.state(), session_.config().difficulty);
        input.right = command.movement;
        input.rightSpeedScale = command.speedScale;
    } else {
        input.left = direction(held_.w, held_.s);
        input.right = direction(held_.up, held_.down);
    }
    return input;
}

void PingPongGameScene::setPaused(bool paused)
{
    paused_ = paused;
    if (paused_) matchRecorder_.pause(); else matchRecorder_.resume();
    clearHeldInput();
    accumulator_ = 0.0;
    if (paused_) pauseOverlay_.resetSelection();
}

void PingPongGameScene::restartMatch()
{
    session_.rematch();
    matchRecorder_.restartPingPong(session_.config());
    simulation_.resetForServe();
    session_.consumePointResetRequest();
    ai_.reset(); trail_.clear(); clearHeldInput(); accumulator_ = 0.0;
    paused_ = false; resultOverlay_.hide();
}

void PingPongGameScene::handlePauseAction(PingPongPauseAction action)
{
    if (action == PingPongPauseAction::Resume) setPaused(false);
    else if (action == PingPongPauseAction::RestartMatch) restartMatch();
    else if (action == PingPongPauseAction::NewSetup) { clearHeldInput(); matchRecorder_.abandon(); sceneManager_.switchTo(SceneId::PingPongSetup); }
    else if (action == PingPongPauseAction::ReturnToLibrary) { clearHeldInput(); matchRecorder_.abandon(); sceneManager_.switchTo(SceneId::GameLibrary); }
}

void PingPongGameScene::handleResultAction(PingPongResultAction action)
{
    if (action == PingPongResultAction::Rematch) restartMatch();
    else if (action == PingPongResultAction::NewSetup) { matchRecorder_.abandon(); sceneManager_.switchTo(SceneId::PingPongSetup); }
    else if (action == PingPongResultAction::ReturnToLibrary) { matchRecorder_.abandon(); sceneManager_.switchTo(SceneId::GameLibrary); }
}

void PingPongGameScene::recordIfComplete()
{
    try {
        if (!matchRecorder_.completePingPong(session_)) return;
        try { notifications_.enqueue(achievements_.evaluateAndUnlock(matchRecorder_.profileId())); }
        catch (const std::exception& error) {
            resultOverlay_.setWarning("Match saved, but achievements could not be updated.");
            std::cerr << "GameVerseArenaGUI: Ping Pong achievement evaluation failed: " << error.what() << '\n';
        }
    }
    catch (const std::exception& error) {
        resultOverlay_.setWarning("Match complete, but history could not be saved.");
        std::cerr << "GameVerseArenaGUI: Ping Pong history save failed: " << error.what() << '\n';
    }
}

void PingPongGameScene::drawPlayfield(sf::RenderTarget& target) const
{
    const auto& state = simulation_.state();
    sf::RectangleShape arena({static_cast<float>(state.field.width), static_cast<float>(state.field.height)});
    arena.setPosition({static_cast<float>(state.field.left), static_cast<float>(state.field.top)});
    arena.setFillColor(Theme::playfield); arena.setOutlineThickness(2.f); arena.setOutlineColor(Theme::border); target.draw(arena);

    for (int index = 0; index < 12; ++index) {
        sf::RectangleShape dash({3.f, 24.f});
        dash.setPosition({638.5f, static_cast<float>(state.field.top + 11.0 + index * 41.0)});
        dash.setFillColor(Theme::divider); target.draw(dash);
    }
    for (std::size_t index = 0; index < trail_.size(); ++index) {
        const float radius = 3.f + static_cast<float>(index) * 0.55f;
        sf::CircleShape trail(radius); trail.setOrigin({radius, radius});
        trail.setPosition({static_cast<float>(trail_[index].x), static_cast<float>(trail_[index].y)});
        const auto alpha = static_cast<std::uint8_t>(10 + index * 5);
        trail.setFillColor({Theme::textPrimary.r, Theme::textPrimary.g, Theme::textPrimary.b, alpha}); target.draw(trail);
    }
    const auto drawPaddle = [&target](const PaddleState& paddle, const sf::Color& color) {
        sf::RectangleShape glow({static_cast<float>(paddle.width + 10.0), static_cast<float>(paddle.height + 10.0)});
        glow.setPosition({static_cast<float>(paddle.position.x - 5.0), static_cast<float>(paddle.position.y - 5.0)});
        glow.setFillColor({color.r, color.g, color.b, 24}); target.draw(glow);
        sf::RectangleShape shape({static_cast<float>(paddle.width), static_cast<float>(paddle.height)});
        shape.setPosition({static_cast<float>(paddle.position.x), static_cast<float>(paddle.position.y)});
        shape.setFillColor(color); target.draw(shape);
    };
    drawPaddle(state.leftPaddle, Theme::arcadeLeft); drawPaddle(state.rightPaddle, Theme::arcadeRight);

    sf::CircleShape glow(static_cast<float>(state.ball.radius + 8.0));
    glow.setOrigin({static_cast<float>(state.ball.radius + 8.0), static_cast<float>(state.ball.radius + 8.0)});
    glow.setPosition({static_cast<float>(state.ball.position.x), static_cast<float>(state.ball.position.y)});
    glow.setFillColor(Theme::ballGlow); target.draw(glow);
    sf::CircleShape ball(static_cast<float>(state.ball.radius));
    ball.setOrigin({static_cast<float>(state.ball.radius), static_cast<float>(state.ball.radius)});
    ball.setPosition({static_cast<float>(state.ball.position.x), static_cast<float>(state.ball.position.y)});
    ball.setFillColor(Theme::textPrimary); target.draw(ball);
}

void PingPongGameScene::drawCentered(sf::RenderTarget& target, const std::string& value, sf::Vector2f center,
                                     unsigned int size, const sf::Color& color, bool strong) const
{
    sf::Text text(strong ? semiboldFont_ : regularFont_, value, size);
    const auto bounds = text.getLocalBounds();
    text.setOrigin({bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f});
    text.setPosition(center); text.setFillColor(color); target.draw(text);
}
