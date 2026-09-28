#include "TicTacToeResultOverlay.hpp"

#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <string>

using namespace classic_ttt;

TicTacToeResultOverlay::TicTacToeResultOverlay(AppContext& context)
    : context_(context), shade_(Theme::logicalSize), panel_({700.f, 450.f}),
      eyebrow_(context.semiboldFont, "ROUND COMPLETE", Theme::labelSize),
      title_(context.semiboldFont, "", 40), roundResult_(context.regularFont, "", Theme::bodySize),
      score_(context.semiboldFont, "", 20), warning_(context.regularFont, "", 14),
      buttons_{UiButton(context.semiboldFont, "", {410.f, 54.f}), UiButton(context.semiboldFont, "", {410.f, 54.f}),
               UiButton(context.semiboldFont, "", {410.f, 54.f})}
{
    shade_.setFillColor(Theme::overlay);
    panel_.setPosition({290.f, 135.f});
    panel_.setFillColor(Theme::backgroundRaised);
    panel_.setOutlineThickness(1.f);
    panel_.setOutlineColor(Theme::primary);
    eyebrow_.setPosition({340.f, 175.f}); eyebrow_.setFillColor(Theme::secondary);
    title_.setPosition({340.f, 205.f}); title_.setFillColor(Theme::textPrimary);
    roundResult_.setPosition({340.f, 265.f}); roundResult_.setFillColor(Theme::textSecondary);
    score_.setPosition({340.f, 302.f}); score_.setFillColor(Theme::textPrimary);
    warning_.setPosition({340.f, 330.f}); warning_.setFillColor(Theme::warning);
    for (std::size_t index = 0; index < buttons_.size(); ++index)
        buttons_[index].setPosition({435.f, 355.f + static_cast<float>(index) * 64.f});
}

void TicTacToeResultOverlay::show(const TicTacToeSession& session)
{
    visible_ = true;
    warning_.setString("");
    matchFinished_ = session.matchFinished();
    selected_ = 0;
    const auto status = session.board().status();
    if (status == GameStatus::Draw) {
        title_.setString("It's a draw");
        roundResult_.setString(matchFinished_ ? "The single game ends in a draw."
                                              : "A balanced round. The match continues without awarding a win.");
    } else {
        const Cell winner = status == GameStatus::XWon ? Cell::X : Cell::O;
        title_.setString(session.playerName(winner) + " wins the round");
        roundResult_.setString(std::string("Winning mark: ") + markCharacter(winner));
    }
    const auto& points = session.score();
    score_.setString(session.playerName(Cell::X) + "  " + std::to_string(points.xWins) + "  -  " +
                     std::to_string(points.oWins) + "  " + session.playerName(Cell::O) +
                     "     Draws " + std::to_string(points.draws));
    eyebrow_.setString(matchFinished_ ? "MATCH COMPLETE" : "ROUND COMPLETE");
    if (matchFinished_) {
        if (status != GameStatus::Draw) roundResult_.setString(roundResult_.getString() + "  |  Match winner");
        buttons_[0].setText("Rematch"); buttons_[1].setText("New Setup");
    } else {
        buttons_[0].setText("Next Round"); buttons_[1].setText("Restart Round");
    }
    buttons_[2].setText("Return to Library");
    for (auto& button : buttons_) button.setHovered(false);
    select(0);
}

TicTacToeResultAction TicTacToeResultOverlay::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (!visible_) return TicTacToeResultAction::None;
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Up || (key->code == sf::Keyboard::Key::Tab && key->shift)) select((selected_ + 2) % 3, true);
        else if (key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::Tab) select((selected_ + 1) % 3, true);
        else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) return actionFor(selected_);
    }
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        for (std::size_t index = 0; index < buttons_.size(); ++index) {
            const bool hovered = buttons_[index].contains(point);
            buttons_[index].setHovered(hovered);
            if (hovered) select(index, true);
        }
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        for (std::size_t index = 0; index < buttons_.size(); ++index)
            if (buttons_[index].contains(point)) return actionFor(index);
    }
    return TicTacToeResultAction::None;
}

void TicTacToeResultOverlay::update(sf::Time deltaTime)
{
    if (visible_) for (auto& button : buttons_) button.update(deltaTime, context_.reducedMotion());
}

void TicTacToeResultOverlay::draw(sf::RenderTarget& target) const
{
    if (!visible_) return;
    target.draw(shade_); target.draw(panel_); target.draw(eyebrow_); target.draw(title_);
    target.draw(roundResult_); target.draw(score_); target.draw(warning_);
    for (const auto& button : buttons_) button.draw(target);
}

void TicTacToeResultOverlay::select(std::size_t index, bool withSound)
{
    if (withSound && index != selected_) context_.play(audio::SoundId::UiFocus);
    selected_ = index;
    for (std::size_t item = 0; item < buttons_.size(); ++item) buttons_[item].setSelected(item == selected_);
}

TicTacToeResultAction TicTacToeResultOverlay::actionFor(std::size_t index) const
{
    if (index == 2) return TicTacToeResultAction::ReturnToLibrary;
    if (matchFinished_) return index == 0 ? TicTacToeResultAction::Rematch : TicTacToeResultAction::NewSetup;
    return index == 0 ? TicTacToeResultAction::NextRound : TicTacToeResultAction::RestartRound;
}
