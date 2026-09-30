#include "BoardGameScene.hpp"

#include "TextLayout.hpp"
#include "Theme.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>
#include <iterator>

using audio::SoundId;
using turn_based::Seat;

namespace {
constexpr sf::Vector2f firstPanel{55.f, 150.f};
constexpr sf::Vector2f secondPanel{935.f, 150.f};
constexpr sf::Vector2f panelSize{290.f, 330.f};

std::string uppercase(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) { return static_cast<char>(std::toupper(character)); });
    return text;
}

int pointsOf(const turn_based::SeatScores& scores, Seat seat)
{
    return seat == Seat::First ? scores.first : scores.second;
}
} // namespace

BoardGameScene::BoardGameScene(AppContext& context, BoardGameHost& host, persistence::MatchRecorder& matchRecorder,
                               persistence::AchievementService& achievements,
                               achievements::AchievementNotificationQueue& notifications)
    : context_(context), host_(host), matchRecorder_(matchRecorder), achievements_(achievements),
      notifications_(notifications), backButton_(context.semiboldFont, "Back to Library", {190.f, 50.f}),
      rulesButton_(context.semiboldFont, "Rules", {190.f, 50.f}),
      restartButton_(context.semiboldFont, "Restart Game", {panelSize.x, 50.f}),
      exitYesButton_(context.semiboldFont, "Leave Game", {190.f, 52.f}),
      exitNoButton_(context.semiboldFont, "Keep Playing", {190.f, 52.f}), resultOverlay_(context),
      rulesOverlay_(context), seeds_(std::random_device{}())
{
    backButton_.setPosition({firstPanel.x, 30.f});
    rulesButton_.setPosition({secondPanel.x + panelSize.x - 190.f, 30.f});
    restartButton_.setPosition({secondPanel.x, 500.f});
    exitYesButton_.setPosition({425.f, 425.f});
    exitNoButton_.setPosition({665.f, 425.f});
    updateButtonStates();
}

sf::FloatRect BoardGameScene::boardArea()
{
    return {{375.f, 105.f}, {530.f, 510.f}};
}

bool BoardGameScene::ready() const
{
    return host_.game != nullptr && host_.view != nullptr && host_.session.started();
}

bool BoardGameScene::gameFinished() const
{
    return ready() && host_.session.game().outcome().finished();
}

bool BoardGameScene::humanMayPlay() const
{
    return ready() && !gameFinished() && !host_.session.isComputerTurn() && !resultOverlay_.visible() &&
           !rulesOverlay_.visible() && !exitConfirmation_ && !computerStuck_;
}

void BoardGameScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (!ready()) {
        if (const auto* key = event.getIf<sf::Event::KeyPressed>(); key && key->code == sf::Keyboard::Key::Escape) {
            leaveTo(SceneId::GameLibrary);
        }
        return;
    }
    if (rulesOverlay_.visible()) {
        if (rulesOverlay_.handleEvent(event, window)) {
            rulesOverlay_.hide();
            context_.play(SoundId::UiBack);
        }
        return;
    }
    if (resultOverlay_.visible()) {
        handleResultAction(resultOverlay_.handleEvent(event, window));
        return;
    }
    if (exitConfirmation_) {
        handleExitConfirmation(event, window);
        return;
    }

    const auto& game = host_.session.game();
    auto& view = *host_.view;
    if (const auto* text = event.getIf<sf::Event::TextEntered>(); text && focus_ == Focus::Board) {
        applyResponse(view.character(text->unicode, game, humanMayPlay()));
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        using Key = sf::Keyboard::Key;
        const auto step = [&](int columns, int rows) {
            setFocus(Focus::Board, false);
            applyResponse(view.navigate(columns, rows, game, humanMayPlay()));
        };
        switch (key->code) {
        case Key::Escape:
            if (reviewingBoard_) showResultAgain();
            else if (view.cancelSelection()) context_.play(SoundId::UiBack);
            else requestExit();
            return;
        case Key::F1:
            showRules();
            return;
        case Key::Tab: {
            // Board -> Restart -> Rules -> Back, and back to the board.
            constexpr Focus order[]{Focus::Board, Focus::Restart, Focus::Rules, Focus::Back};
            const auto current = static_cast<int>(std::find(std::begin(order), std::end(order), focus_) - std::begin(order));
            setFocus(order[(current + (key->shift ? 3 : 1)) % 4], true);
            break;
        }
        case Key::Left: step(-1, 0); break;
        case Key::Right: step(1, 0); break;
        case Key::Up: step(0, -1); break;
        case Key::Down: step(0, 1); break;
        case Key::Enter:
        case Key::Space:
            if (focus_ != Focus::Board) {
                activateFocus();
                return;
            }
            if (reviewingBoard_) {
                showResultAgain();
                return;
            }
            applyResponse(view.activate(game, humanMayPlay()));
            break;
        default: break;
        }
    }

    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        backButton_.setHovered(backButton_.contains(point));
        rulesButton_.setHovered(rulesButton_.contains(point));
        restartButton_.setHovered(restartButton_.contains(point));
        applyResponse(view.pointerMoved(point, game, humanMayPlay()));
    }

    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        if (backButton_.contains(point)) {
            requestExit();
            return;
        }
        if (rulesButton_.contains(point)) {
            showRules();
            return;
        }
        if (restartButton_.contains(point)) {
            restartGame();
            return;
        }
        if (reviewingBoard_) {
            showResultAgain();
            return;
        }
        setFocus(Focus::Board, false);
        applyResponse(view.pointerReleased(point, game, humanMayPlay()));
    }
}

void BoardGameScene::update(sf::Time deltaTime)
{
    const float seconds = deltaTime.asSeconds();
    const bool reduced = context_.reducedMotion();
    time_ += seconds;
    for (auto* button : {&backButton_, &rulesButton_, &restartButton_, &exitYesButton_, &exitNoButton_}) {
        button->update(deltaTime, reduced);
    }
    resultOverlay_.update(deltaTime);
    rulesOverlay_.update(deltaTime);
    if (!ready()) return;
    startComputerIfNeeded();
    pollComputer(seconds);
    // After the computer's move, so a move applied this frame starts its animation from here
    // (and, with Reduced Motion, is drawn at full size straight away).
    host_.view->update(seconds, reduced);
}

void BoardGameScene::render(sf::RenderWindow& window) const
{
    if (!ready()) return;
    const auto& game = host_.session.game();
    const bool computerGame = host_.session.computerSeat().has_value();

    backButton_.draw(window);
    rulesButton_.draw(window);
    drawCenteredLine(window, uppercase(host_.game->displayName), 30.f, Theme::labelSize, Theme::secondary, true);
    drawCenteredLine(window,
                     "Game " + std::to_string(host_.session.gameNumber()) + "  |  " +
                         (computerGame ? "Human vs Computer" : "Human vs Human"),
                     55.f, Theme::bodySize, Theme::textSecondary, false);
    drawSeatPanel(window, Seat::First, firstPanel);
    drawSeatPanel(window, Seat::Second, secondPanel);
    host_.view->draw(window, game,
                     board_view::DrawState{context_.regularFont, context_.semiboldFont, humanMayPlay(),
                                           context_.reducedMotion(), time_});

    sf::Color statusColor = Theme::textPrimary;
    if (!errorMessage_.empty()) statusColor = Theme::danger;
    else if (host_.session.isComputerTurn()) statusColor = Theme::warning;
    drawCenteredLine(window, statusText(), 626.f, Theme::bodySize, statusColor, true);
    const std::string help = game.outcome().finished()
        ? "Tab: buttons  |  F1: rules"
        : host_.view->controls() + "  |  Tab: buttons  |  F1: rules  |  Esc: leave";
    drawCenteredLine(window, help, 664.f, Theme::labelSize, Theme::textMuted, false);
    restartButton_.draw(window);

    resultOverlay_.draw(window);
    rulesOverlay_.draw(window);
    if (exitConfirmation_) drawExitConfirmation(window);
}

void BoardGameScene::onResize(sf::Vector2u) {}

void BoardGameScene::onActivate()
{
    computer_.cancel();
    computerStuck_ = false;
    errorMessage_.clear();
    computerElapsed_ = 0.f;
    exitConfirmation_ = false;
    exitYesSelected_ = false;
    reviewingBoard_ = false;
    resultOverlay_.hide();
    rulesOverlay_.hide();
    for (auto* button : {&backButton_, &rulesButton_, &restartButton_, &exitYesButton_, &exitNoButton_}) {
        button->setHovered(false);
    }
    focus_ = Focus::Board;
    time_ = 0.f;
    if (ready()) {
        host_.view->layout(boardArea());
        host_.view->reset(host_.session.game());
    }
    updateButtonStates();
}

void BoardGameScene::applyResponse(const board_view::Response& response)
{
    using Feedback = board_view::Response::Feedback;
    if (response.feedback == Feedback::Focus) context_.play(SoundId::UiFocus);
    else if (response.feedback == Feedback::Select) context_.play(SoundId::UiConfirm);
    else if (response.feedback == Feedback::Invalid) context_.play(SoundId::UiError);
    if (response.move && humanMayPlay()) applyMove(*response.move);
}

void BoardGameScene::applyMove(turn_based::MoveId move)
{
    const auto& game = host_.session.game();
    const Seat mover = game.currentSeat();
    const auto before = game.scores();
    if (!host_.session.play(move)) {
        context_.play(SoundId::UiError);
        return;
    }
    host_.view->movePlayed(game, move);
    context_.play(mover == Seat::First ? SoundId::MovePrimary : SoundId::MoveSecondary);
    const auto after = game.scores();
    if (before && after && pointsOf(*after, mover) > pointsOf(*before, mover)) context_.play(SoundId::SpecialEvent);
    if (const auto outcome = host_.session.takeCompletedOutcome()) finishGame(*outcome);
}

void BoardGameScene::startComputerIfNeeded()
{
    if (computerStuck_ || computer_.running() || !host_.session.isComputerTurn()) return;
    if (resultOverlay_.visible() || rulesOverlay_.visible() || exitConfirmation_) return;
    try {
        computer_.start(host_.session.game().clone(), seeds_());
    } catch (const std::exception& error) {
        // For example, the worker thread could not be created.
        computerFailed(error.what());
        return;
    }
    computerElapsed_ = 0.f;
}

void BoardGameScene::pollComputer(float seconds)
{
    if (!computer_.running()) return;
    computerElapsed_ += seconds;
    // A finished search waits while the player reads the rules or decides whether to leave, and
    // is never applied before the short minimum thinking pause.
    if (rulesOverlay_.visible() || exitConfirmation_ || computerElapsed_ < Theme::aiThinkingDelay) return;
    std::optional<std::optional<turn_based::MoveId>> result;
    try {
        result = computer_.poll();
    } catch (const std::exception& error) {
        computerFailed(error.what());
        return;
    }
    if (!result) return;
    if (!*result || !host_.session.isComputerTurn() || !host_.session.game().isLegal(**result)) {
        computerFailed("the search returned no legal move");
        return;
    }
    applyMove(**result);
}

void BoardGameScene::computerFailed(const std::string& detail)
{
    computer_.cancel();
    computerStuck_ = true;
    errorMessage_ = "The computer could not choose a move. Restart the game or return to the library.";
    std::cerr << "GameVerseArenaGUI: " << host_.game->displayName << " computer move failed: " << detail << '\n';
    context_.play(SoundId::UiError);
}

void BoardGameScene::finishGame(const turn_based::Outcome& outcome)
{
    computer_.cancel();
    reviewingBoard_ = false;
    const auto computerSeat = host_.session.computerSeat();
    if (!outcome.winner) context_.play(SoundId::RoundDraw);
    else context_.play(computerSeat && *outcome.winner == *computerSeat ? SoundId::RoundLoss : SoundId::RoundWin);
    resultOverlay_.show(host_.session, outcome, *host_.game);
    recordCompletion(outcome);
}

void BoardGameScene::recordCompletion(const turn_based::Outcome& outcome)
{
    const auto& setup = host_.session.setup();
    const Seat profileSeat = setup.mode == turn_based::PlayMode::HumanVsComputer ? setup.humanSeat : Seat::First;
    auto result = persistence::MatchResult::Draw;
    if (outcome.winner) {
        result = *outcome.winner == profileSeat ? persistence::MatchResult::Win : persistence::MatchResult::Loss;
    }
    std::optional<int> ownPoints;
    std::optional<int> opponentPoints;
    if (const auto scores = host_.session.game().scores()) {
        ownPoints = pointsOf(*scores, profileSeat);
        opponentPoints = pointsOf(*scores, turn_based::otherSeat(profileSeat));
    }
    try {
        if (!matchRecorder_.completeBoardGame(result, ownPoints, opponentPoints)) {
            if (host_.recordingUnavailable) resultOverlay_.setWarning("Game complete, but it could not be saved to history.");
            return;
        }
        try {
            notifications_.enqueue(achievements_.evaluateAndUnlock(matchRecorder_.profileId()));
        } catch (const std::exception& error) {
            resultOverlay_.setWarning("Game saved, but achievements could not be updated.");
            std::cerr << "GameVerseArenaGUI: " << host_.game->displayName
                      << " achievement evaluation failed: " << error.what() << '\n';
        }
    } catch (const std::exception& error) {
        resultOverlay_.setWarning("Game complete, but history could not be saved.");
        std::cerr << "GameVerseArenaGUI: " << host_.game->displayName << " history save failed: " << error.what() << '\n';
    }
}

void BoardGameScene::restartGame()
{
    computer_.cancel();
    computerStuck_ = false;
    errorMessage_.clear();
    computerElapsed_ = 0.f;
    host_.session.restart();
    matchRecorder_.restartBoardGame();
    host_.view->reset(host_.session.game());
    resultOverlay_.hide();
    reviewingBoard_ = false;
    exitConfirmation_ = false;
    context_.play(SoundId::UiConfirm);
}

void BoardGameScene::requestExit()
{
    // Nothing is lost when no move has been made or the game is already over (and recorded).
    if (gameFinished() || host_.session.game().movesPlayed() == 0) {
        leaveTo(SceneId::GameLibrary);
        return;
    }
    exitConfirmation_ = true;
    exitYesSelected_ = false;
    exitYesButton_.setHovered(false);
    exitNoButton_.setHovered(false);
    context_.play(SoundId::UiBack);
    updateButtonStates();
}

void BoardGameScene::leaveTo(SceneId scene)
{
    computer_.cancel();
    matchRecorder_.abandon();
    exitConfirmation_ = false;
    reviewingBoard_ = false;
    resultOverlay_.hide();
    rulesOverlay_.hide();
    context_.play(SoundId::UiBack);
    context_.scenes.switchTo(scene);
}

void BoardGameScene::showRules()
{
    if (!host_.game) return;
    rulesOverlay_.show(*host_.game);
    context_.play(SoundId::UiConfirm);
}

void BoardGameScene::showResultAgain()
{
    reviewingBoard_ = false;
    resultOverlay_.reveal();
    context_.play(SoundId::UiConfirm);
}

void BoardGameScene::handleResultAction(BoardGameResultAction action)
{
    switch (action) {
    case BoardGameResultAction::None: return;
    case BoardGameResultAction::Rematch: restartGame(); return;
    case BoardGameResultAction::ViewBoard:
        resultOverlay_.hide();
        reviewingBoard_ = true;
        setFocus(Focus::Board, false);
        context_.play(SoundId::UiBack);
        return;
    case BoardGameResultAction::NewSetup: leaveTo(SceneId::BoardGameSetup); return;
    case BoardGameResultAction::ReturnToLibrary: leaveTo(SceneId::GameLibrary); return;
    }
}

void BoardGameScene::handleExitConfirmation(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        using Key = sf::Keyboard::Key;
        if (key->code == Key::Escape) {
            exitConfirmation_ = false;
            context_.play(SoundId::UiBack);
        } else if (key->code == Key::Left || key->code == Key::Right || key->code == Key::Tab) {
            exitYesSelected_ = !exitYesSelected_;
            context_.play(SoundId::UiFocus);
        } else if (key->code == Key::Enter || key->code == Key::Space) {
            if (exitYesSelected_) {
                leaveTo(SceneId::GameLibrary);
                return;
            }
            exitConfirmation_ = false;
            context_.play(SoundId::UiConfirm);
        }
    }
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        exitYesButton_.setHovered(exitYesButton_.contains(point));
        exitNoButton_.setHovered(exitNoButton_.contains(point));
        const bool previous = exitYesSelected_;
        if (exitYesButton_.contains(point)) exitYesSelected_ = true;
        if (exitNoButton_.contains(point)) exitYesSelected_ = false;
        if (previous != exitYesSelected_) context_.play(SoundId::UiFocus);
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        if (exitYesButton_.contains(point)) {
            leaveTo(SceneId::GameLibrary);
            return;
        }
        if (exitNoButton_.contains(point)) {
            exitConfirmation_ = false;
            context_.play(SoundId::UiConfirm);
        }
    }
    updateButtonStates();
}

void BoardGameScene::setFocus(Focus focus, bool withSound)
{
    if (withSound && focus != focus_) context_.play(SoundId::UiFocus);
    focus_ = focus;
    updateButtonStates();
}

void BoardGameScene::activateFocus()
{
    switch (focus_) {
    case Focus::Board: return;
    case Focus::Restart: restartGame(); return;
    case Focus::Rules: showRules(); return;
    case Focus::Back: requestExit(); return;
    }
}

void BoardGameScene::updateButtonStates()
{
    restartButton_.setSelected(focus_ == Focus::Restart);
    rulesButton_.setSelected(focus_ == Focus::Rules);
    backButton_.setSelected(focus_ == Focus::Back);
    exitYesButton_.setSelected(exitConfirmation_ && exitYesSelected_);
    exitNoButton_.setSelected(exitConfirmation_ && !exitYesSelected_);
}

std::string BoardGameScene::statusText() const
{
    if (!errorMessage_.empty()) return errorMessage_;
    const auto& game = host_.session.game();
    if (game.outcome().finished()) {
        return reviewingBoard_ ? "Final position  |  Enter, Escape, or a click shows the result again" : "";
    }
    if (host_.session.isComputerTurn()) return "Computer is thinking...";
    return host_.session.nameOf(game.currentSeat()) + "'s turn  |  " + host_.view->moveHint(game);
}

void BoardGameScene::drawCenteredLine(sf::RenderTarget& target, const std::string& text, float y, unsigned int size,
                                      const sf::Color& color, bool strong) const
{
    if (text.empty()) return;
    const auto& font = strong ? context_.semiboldFont : context_.regularFont;
    sf::Text line(font, toDisplay(fitToWidth(font, text, size, 1180.f)), size);
    const auto bounds = line.getLocalBounds();
    line.setPosition({Theme::logicalSize.x / 2.f - bounds.size.x / 2.f - bounds.position.x, y});
    line.setFillColor(color);
    target.draw(line);
}

void BoardGameScene::drawSeatPanel(sf::RenderTarget& target, Seat seat, sf::Vector2f position) const
{
    const auto& game = host_.session.game();
    const auto outcome = game.outcome();
    const bool toMove = !outcome.finished() && game.currentSeat() == seat;
    const sf::Color seatColor = seat == Seat::First ? Theme::markX : Theme::markO;

    sf::RectangleShape panel(panelSize);
    panel.setPosition(position);
    panel.setFillColor(Theme::backgroundRaised);
    panel.setOutlineThickness(toMove ? 2.f : 1.f);
    panel.setOutlineColor(toMove ? seatColor : Theme::border);
    target.draw(panel);

    const float left = position.x + 28.f;
    const float width = panelSize.x - 56.f;
    const auto text = [&](const std::string& value, float y, unsigned int size, const sf::Color& color, bool strong) {
        const auto& font = strong ? context_.semiboldFont : context_.regularFont;
        sf::Text label(font, toDisplay(fitToWidth(font, value, size, width)), size);
        label.setPosition({left, position.y + y});
        label.setFillColor(color);
        target.draw(label);
    };

    text(uppercase(seat == Seat::First ? host_.game->firstSeatLabel : host_.game->secondSeatLabel), 26.f,
         Theme::labelSize, seatColor, true);
    text(host_.session.nameOf(seat), 52.f, 25, Theme::textPrimary, true);
    const auto computer = host_.session.computerSeat();
    std::string role = computer ? (*computer == seat ? "Opponent" : "You") : (seat == Seat::First ? "Player 1" : "Player 2");
    role += seat == Seat::First ? "  |  moves first" : "  |  moves second";
    text(role, 92.f, 16, Theme::textMuted, false);

    float y = 140.f;
    if (const auto scores = game.scores()) {
        text("Points", y, Theme::labelSize, Theme::textMuted, false);
        text(std::to_string(pointsOf(*scores, seat)), y + 20.f, 48, seatColor, true);
        y += 96.f;
    }
    if (const auto detail = host_.view->seatDetail(game, seat); !detail.empty()) {
        text(detail, y, 16, Theme::textSecondary, false);
    }

    std::string badge;
    sf::Color badgeColor = Theme::textMuted;
    if (outcome.finished()) {
        if (!outcome.winner) {
            badge = "DRAW";
        } else if (*outcome.winner == seat) {
            badge = "WINNER";
            badgeColor = Theme::secondary;
        }
    } else if (toMove) {
        badge = computer && *computer == seat ? "THINKING" : "TO MOVE";
        badgeColor = seatColor;
    }
    if (!badge.empty()) text(badge, panelSize.y - 44.f, Theme::labelSize, badgeColor, true);
}

void BoardGameScene::drawExitConfirmation(sf::RenderTarget& target) const
{
    sf::RectangleShape shade(Theme::logicalSize);
    shade.setFillColor(Theme::overlay);
    target.draw(shade);
    sf::RectangleShape panel({660.f, 270.f});
    panel.setPosition({310.f, 225.f});
    panel.setFillColor(Theme::backgroundRaised);
    panel.setOutlineThickness(1.f);
    panel.setOutlineColor(Theme::danger);
    target.draw(panel);
    sf::Text title(context_.semiboldFont, "Leave this game?", 34);
    title.setPosition({405.f, 270.f});
    title.setFillColor(Theme::textPrimary);
    target.draw(title);
    sf::Text body(context_.regularFont, "The game in progress will be discarded and not recorded.", Theme::bodySize);
    body.setPosition({405.f, 330.f});
    body.setFillColor(Theme::textSecondary);
    target.draw(body);
    exitYesButton_.draw(target);
    exitNoButton_.draw(target);
}
