#include "TicTacToeGameScene.hpp"

#include "Theme.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

using namespace classic_ttt;

namespace {
constexpr float cellSize = Theme::boardSize / 3.f;

std::string matchLength(BestOf bestOf)
{
    if (bestOf == BestOf::Single) return "Single Game";
    return "Best of " + std::to_string(static_cast<int>(bestOf));
}
} // namespace

TicTacToeGameScene::TicTacToeGameScene(const sf::Font& regularFont, const sf::Font& semiboldFont,
                                       SceneManager& sceneManager, TicTacToeSession& session,
                                       persistence::MatchRecorder& matchRecorder,
                                       persistence::AchievementService& achievements,
                                       achievements::AchievementNotificationQueue& notifications)
    : sceneManager_(sceneManager), session_(session), matchRecorder_(matchRecorder), achievements_(achievements),
      notifications_(notifications), regularFont_(regularFont), semiboldFont_(semiboldFont),
      backButton_(semiboldFont, "Back to Library", {190.f, 50.f}),
      restartButton_(semiboldFont, "Restart Round", {190.f, 50.f}),
      newMatchButton_(semiboldFont, "New Match", {190.f, 50.f}),
      exitYesButton_(semiboldFont, "Leave Match", {190.f, 52.f}),
      exitNoButton_(semiboldFont, "Keep Playing", {190.f, 52.f}),
      resultOverlay_(regularFont, semiboldFont)
{
    backButton_.setPosition({42.f, 30.f});
    restartButton_.setPosition({1028.f, 590.f});
    newMatchButton_.setPosition({1028.f, 650.f});
    exitYesButton_.setPosition({425.f, 425.f});
    exitNoButton_.setPosition({665.f, 425.f});
    updateButtonStates();
}

void TicTacToeGameScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (resultOverlay_.visible()) {
        handleResultAction(resultOverlay_.handleEvent(event, window));
        return;
    }

    if (exitConfirmation_) {
        if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
            if (key->code == sf::Keyboard::Key::Escape) exitConfirmation_ = false;
            else if (key->code == sf::Keyboard::Key::Left || key->code == sf::Keyboard::Key::Right || key->code == sf::Keyboard::Key::Tab) {
                exitYesSelected_ = !exitYesSelected_;
            } else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) {
                if (exitYesSelected_) { cancelAI(); matchRecorder_.abandon(); sceneManager_.switchTo(SceneId::GameLibrary); }
                else exitConfirmation_ = false;
            }
        }
        if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
            const auto point = window.mapPixelToCoords(moved->position);
            exitYesButton_.setHovered(exitYesButton_.contains(point));
            exitNoButton_.setHovered(exitNoButton_.contains(point));
            if (exitYesButton_.contains(point)) exitYesSelected_ = true;
            if (exitNoButton_.contains(point)) exitYesSelected_ = false;
        }
        if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
            click && click->button == sf::Mouse::Button::Left) {
            const auto point = window.mapPixelToCoords(click->position);
            if (exitYesButton_.contains(point)) { cancelAI(); matchRecorder_.abandon(); sceneManager_.switchTo(SceneId::GameLibrary); }
            else if (exitNoButton_.contains(point)) exitConfirmation_ = false;
        }
        updateButtonStates();
        return;
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) requestLibraryExit();
        else if (key->code == sf::Keyboard::Key::Left) selectedCell_.column = (selectedCell_.column + 2) % 3;
        else if (key->code == sf::Keyboard::Key::Right) selectedCell_.column = (selectedCell_.column + 1) % 3;
        else if (key->code == sf::Keyboard::Key::Up) selectedCell_.row = (selectedCell_.row + 2) % 3;
        else if (key->code == sf::Keyboard::Key::Down) selectedCell_.row = (selectedCell_.row + 1) % 3;
        else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) tryMove(selectedCell_);
    }

    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        hoveredCell_ = cellAt(point);
        if (hoveredCell_) selectedCell_ = *hoveredCell_;
        backButton_.setHovered(backButton_.contains(point));
        restartButton_.setHovered(restartButton_.contains(point));
        newMatchButton_.setHovered(newMatchButton_.contains(point));
    }

    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        if (backButton_.contains(point)) requestLibraryExit();
        else if (restartButton_.contains(point)) { session_.restartRound(); resetVisualState(); scheduleAI(); }
        else if (newMatchButton_.contains(point)) { session_.rematch(); matchRecorder_.restartTicTacToe(session_.config()); resetVisualState(); scheduleAI(); }
        else if (const auto position = cellAt(point)) tryMove(*position);
    }
}

void TicTacToeGameScene::update(sf::Time deltaTime)
{
    const float seconds = deltaTime.asSeconds();
    markAnimation_ = std::min(1.f, markAnimation_ + seconds * 5.5f);
    selectionPulse_ += seconds;
    backButton_.update(deltaTime); restartButton_.update(deltaTime); newMatchButton_.update(deltaTime);
    exitYesButton_.update(deltaTime); exitNoButton_.update(deltaTime); resultOverlay_.update(deltaTime);

    if (!resultOverlay_.visible() && !exitConfirmation_ && session_.isComputerTurn() && !aiPending_) scheduleAI();
    if (aiPending_ && !exitConfirmation_) {
        aiElapsed_ += seconds;
        if (aiElapsed_ >= Theme::aiThinkingDelay) performAI();
    }
}

void TicTacToeGameScene::render(sf::RenderWindow& window) const
{
    const auto drawText = [&](const std::string& value, sf::Vector2f position, unsigned int size,
                              const sf::Color& color, bool strong = false) {
        sf::Text text(strong ? semiboldFont_ : regularFont_, value, size);
        text.setPosition(position); text.setFillColor(color); window.draw(text);
    };

    backButton_.draw(window);
    drawText("CLASSIC TIC-TAC-TOE", {505.f, 30.f}, Theme::labelSize, Theme::secondary, true);
    drawText("Round " + std::to_string(session_.roundNumber()) + "  |  " + matchLength(session_.config().bestOf),
             {500.f, 55.f}, Theme::bodySize, Theme::textSecondary);

    sf::RectangleShape leftPanel({315.f, 260.f}); leftPanel.setPosition({55.f, 205.f});
    leftPanel.setFillColor(Theme::backgroundRaised); leftPanel.setOutlineThickness(1.f); leftPanel.setOutlineColor(Theme::border);
    sf::RectangleShape rightPanel = leftPanel; rightPanel.setPosition({910.f, 205.f});
    window.draw(leftPanel); window.draw(rightPanel);

    const auto& score = session_.score();
    drawText("PLAYER X", {85.f, 235.f}, Theme::labelSize, Theme::markX, true);
    drawText(session_.playerName(Cell::X), {85.f, 270.f}, 25, Theme::textPrimary, true);
    drawText("Wins", {85.f, 330.f}, Theme::labelSize, Theme::textMuted);
    drawText(std::to_string(score.xWins), {85.f, 352.f}, 48, Theme::markX, true);
    drawText("PLAYER O", {940.f, 235.f}, Theme::labelSize, Theme::markO, true);
    drawText(session_.playerName(Cell::O), {940.f, 270.f}, 25, Theme::textPrimary, true);
    drawText("Wins", {940.f, 330.f}, Theme::labelSize, Theme::textMuted);
    drawText(std::to_string(score.oWins), {940.f, 352.f}, 48, Theme::markO, true);
    drawText("Draws  " + std::to_string(score.draws), {85.f, 425.f}, Theme::bodySize, Theme::textSecondary, true);
    drawText("First to " + std::to_string(session_.winsNeeded()) + " wins", {940.f, 425.f}, Theme::bodySize, Theme::textSecondary);

    drawBoard(window);
    if (aiPending_) drawText("Computer is thinking...", {526.f, 620.f}, Theme::bodySize, Theme::warning, true);
    else if (session_.board().status() == GameStatus::InProgress)
        drawText(session_.playerName(session_.currentTurn()) + "'s turn  |  " + markCharacter(session_.currentTurn()),
                 {518.f, 620.f}, Theme::bodySize, Theme::textPrimary, true);
    drawText("Arrows select  |  Enter / Space plays", {490.f, 665.f}, Theme::labelSize, Theme::textMuted);
    restartButton_.draw(window); newMatchButton_.draw(window);
    resultOverlay_.draw(window);
    if (exitConfirmation_) drawExitConfirmation(window);
}

void TicTacToeGameScene::onResize(sf::Vector2u) {}

void TicTacToeGameScene::onActivate()
{
    exitConfirmation_ = false;
    resultOverlay_.hide();
    resetVisualState();
    scheduleAI();
}

std::optional<Position> TicTacToeGameScene::cellAt(sf::Vector2f point) const
{
    const sf::FloatRect bounds(boardPosition_, {Theme::boardSize, Theme::boardSize});
    if (!bounds.contains(point)) return std::nullopt;
    return Position{static_cast<std::size_t>((point.y - boardPosition_.y) / cellSize),
                    static_cast<std::size_t>((point.x - boardPosition_.x) / cellSize)};
}

bool TicTacToeGameScene::humanMayPlay() const
{
    return !aiPending_ && !session_.isComputerTurn() && !resultOverlay_.visible() && !exitConfirmation_ &&
           session_.board().status() == GameStatus::InProgress && !session_.matchFinished();
}

void TicTacToeGameScene::tryMove(Position position)
{
    if (!humanMayPlay() || !session_.playMove(position)) return;
    lastMove_ = position;
    markAnimation_ = 0.f;
    if (session_.board().status() != GameStatus::InProgress) { resultOverlay_.show(session_); recordIfComplete(); }
    else if (session_.isComputerTurn()) scheduleAI();
}

void TicTacToeGameScene::scheduleAI()
{
    cancelAI();
    if (session_.isComputerTurn() && !resultOverlay_.visible()) aiPending_ = true;
}

void TicTacToeGameScene::cancelAI()
{
    aiPending_ = false;
    aiElapsed_ = 0.f;
}

void TicTacToeGameScene::performAI()
{
    if (!aiPending_ || !session_.isComputerTurn()) { cancelAI(); return; }
    const auto move = ai_.chooseMove(session_.board(), session_.computerMark(), session_.config().difficulty);
    cancelAI();
    if (!move || !session_.playMove(*move)) return;
    lastMove_ = *move;
    markAnimation_ = 0.f;
    if (session_.board().status() != GameStatus::InProgress) { resultOverlay_.show(session_); recordIfComplete(); }
}

void TicTacToeGameScene::handleResultAction(TicTacToeResultAction action)
{
    if (action == TicTacToeResultAction::None) return;
    resultOverlay_.hide(); cancelAI();
    if (action == TicTacToeResultAction::NextRound) session_.nextRound();
    else if (action == TicTacToeResultAction::RestartRound) session_.restartRound();
    else if (action == TicTacToeResultAction::Rematch) { session_.rematch(); matchRecorder_.restartTicTacToe(session_.config()); }
    else if (action == TicTacToeResultAction::NewSetup) { matchRecorder_.abandon(); sceneManager_.switchTo(SceneId::TicTacToeSetup); return; }
    else if (action == TicTacToeResultAction::ReturnToLibrary) { matchRecorder_.abandon(); sceneManager_.switchTo(SceneId::GameLibrary); return; }
    resetVisualState(); scheduleAI();
}

void TicTacToeGameScene::resetVisualState()
{
    cancelAI();
    selectedCell_ = {1, 1};
    hoveredCell_.reset(); lastMove_.reset(); markAnimation_ = 1.f;
}

void TicTacToeGameScene::requestLibraryExit()
{
    cancelAI();
    exitConfirmation_ = true;
    exitYesSelected_ = false;
    updateButtonStates();
}

void TicTacToeGameScene::updateButtonStates()
{
    exitYesButton_.setSelected(exitConfirmation_ && exitYesSelected_);
    exitNoButton_.setSelected(exitConfirmation_ && !exitYesSelected_);
}

void TicTacToeGameScene::recordIfComplete()
{
    if (!session_.matchFinished()) return;
    try {
        if (!matchRecorder_.completeTicTacToe(session_)) return;
        try { notifications_.enqueue(achievements_.evaluateAndUnlock(matchRecorder_.profileId())); }
        catch (const std::exception& error) {
            resultOverlay_.setWarning("Match saved, but achievements could not be updated.");
            std::cerr << "GameVerseArenaGUI: Tic-Tac-Toe achievement evaluation failed: " << error.what() << '\n';
        }
    }
    catch (const std::exception& error) {
        resultOverlay_.setWarning("Match complete, but history could not be saved.");
        std::cerr << "GameVerseArenaGUI: Tic-Tac-Toe history save failed: " << error.what() << '\n';
    }
}

void TicTacToeGameScene::drawBoard(sf::RenderTarget& target) const
{
    sf::RectangleShape board({Theme::boardSize, Theme::boardSize});
    board.setPosition(boardPosition_); board.setFillColor(Theme::backgroundRaised);
    board.setOutlineThickness(2.f); board.setOutlineColor(Theme::border); target.draw(board);

    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column) {
            const Position position{row, column};
            const sf::Vector2f topLeft{boardPosition_.x + column * cellSize, boardPosition_.y + row * cellSize};
            const bool legal = session_.board().cell(position) == Cell::Empty && humanMayPlay();
            const bool selected = position == selectedCell_;
            const bool hovered = hoveredCell_ && *hoveredCell_ == position;
            bool winning = false;
            if (session_.board().winningLine())
                for (const auto winCell : *session_.board().winningLine()) if (winCell == position) winning = true;
            if (winning || (legal && (selected || hovered))) {
                sf::RectangleShape highlight({cellSize - 8.f, cellSize - 8.f});
                highlight.setPosition({topLeft.x + 4.f, topLeft.y + 4.f});
                if (winning) highlight.setFillColor(Theme::winningCell);
                else {
                    const auto alpha = static_cast<std::uint8_t>(20 + 10 * (0.5f + 0.5f * std::sin(selectionPulse_ * 4.f)));
                    highlight.setFillColor({Theme::primary.r, Theme::primary.g, Theme::primary.b, alpha});
                    highlight.setOutlineThickness(2.f); highlight.setOutlineColor(Theme::primaryBright);
                }
                target.draw(highlight);
            }

            const Cell mark = session_.board().cell(position);
            if (mark == Cell::Empty) continue;
            const float scale = lastMove_ && *lastMove_ == position ? std::clamp(markAnimation_, 0.05f, 1.f) : 1.f;
            const sf::Vector2f center{topLeft.x + cellSize / 2.f, topLeft.y + cellSize / 2.f};
            if (mark == Cell::X) {
                for (float angle : {45.f, -45.f}) {
                    sf::RectangleShape stroke({82.f * scale, 9.f});
                    stroke.setOrigin({41.f * scale, 4.5f}); stroke.setPosition(center);
                    stroke.setRotation(sf::degrees(angle)); stroke.setFillColor(Theme::markX); target.draw(stroke);
                }
            } else {
                sf::CircleShape ring(39.f * scale);
                ring.setOrigin({39.f * scale, 39.f * scale}); ring.setPosition(center);
                ring.setFillColor(sf::Color::Transparent); ring.setOutlineThickness(9.f * scale);
                ring.setOutlineColor(Theme::markO); target.draw(ring);
            }
        }
    }

    for (int line = 1; line < 3; ++line) {
        sf::RectangleShape vertical({3.f, Theme::boardSize});
        vertical.setPosition({boardPosition_.x + line * cellSize - 1.5f, boardPosition_.y}); vertical.setFillColor(Theme::border);
        sf::RectangleShape horizontal({Theme::boardSize, 3.f});
        horizontal.setPosition({boardPosition_.x, boardPosition_.y + line * cellSize - 1.5f}); horizontal.setFillColor(Theme::border);
        target.draw(vertical); target.draw(horizontal);
    }
}

void TicTacToeGameScene::drawExitConfirmation(sf::RenderTarget& target) const
{
    sf::RectangleShape shade(Theme::logicalSize); shade.setFillColor(Theme::overlay); target.draw(shade);
    sf::RectangleShape panel({660.f, 270.f}); panel.setPosition({310.f, 225.f});
    panel.setFillColor(Theme::backgroundRaised); panel.setOutlineThickness(1.f); panel.setOutlineColor(Theme::danger); target.draw(panel);
    sf::Text title(semiboldFont_, "Leave this match?", 34); title.setPosition({405.f, 270.f}); title.setFillColor(Theme::textPrimary); target.draw(title);
    sf::Text body(regularFont_, "The current in-memory match score will be discarded.", Theme::bodySize);
    body.setPosition({405.f, 330.f}); body.setFillColor(Theme::textSecondary); target.draw(body);
    exitYesButton_.draw(target); exitNoButton_.draw(target);
}
