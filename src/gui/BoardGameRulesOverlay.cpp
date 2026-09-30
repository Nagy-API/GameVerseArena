#include "BoardGameRulesOverlay.hpp"

#include "TextLayout.hpp"
#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>

namespace {
constexpr float textLeft = 250.f;
constexpr float textWidth = 780.f;
} // namespace

BoardGameRulesOverlay::BoardGameRulesOverlay(AppContext& context)
    : context_(context), shade_(Theme::logicalSize), panel_({880.f, 560.f}),
      eyebrow_(context.semiboldFont, "HOW TO PLAY", Theme::labelSize), title_(context.semiboldFont, "", 34),
      rules_(context.regularFont, "", Theme::bodySize), computerHeading_(context.semiboldFont, "COMPUTER OPPONENT", Theme::labelSize),
      computer_(context.regularFont, "", 17), closeButton_(context.semiboldFont, "Close", {180.f, 52.f})
{
    shade_.setFillColor(Theme::overlay);
    panel_.setPosition({200.f, 80.f});
    panel_.setFillColor(Theme::backgroundRaised);
    panel_.setOutlineThickness(1.f);
    panel_.setOutlineColor(Theme::primary);
    eyebrow_.setPosition({textLeft, 112.f});
    eyebrow_.setFillColor(Theme::secondary);
    title_.setPosition({textLeft, 136.f});
    title_.setFillColor(Theme::textPrimary);
    rules_.setPosition({textLeft, 198.f});
    rules_.setFillColor(Theme::textSecondary);
    rules_.setLineSpacing(1.1f);
    computerHeading_.setPosition({textLeft, 440.f});
    computerHeading_.setFillColor(Theme::secondary);
    computer_.setPosition({textLeft, 466.f});
    computer_.setFillColor(Theme::textSecondary);
    computer_.setLineSpacing(1.1f);
    closeButton_.setPosition({870.f, 572.f});
    closeButton_.setSelected(true);
}

void BoardGameRulesOverlay::show(const catalogue::GameDescriptor& game)
{
    title_.setString(toDisplay(fitToWidth(context_.semiboldFont, game.displayName, 34, textWidth)));
    rules_.setString(toDisplay(wrapToWidth(context_.regularFont, game.rules, Theme::bodySize, textWidth, 9)));
    const std::string strategy = game.humanVsComputer ? game.computerStrategy : "This game is for two players.";
    computer_.setString(toDisplay(wrapToWidth(context_.regularFont, strategy, 17, textWidth, 3)));
    // The computer section follows the rules text directly, however long the rules are.
    const auto rulesBounds = rules_.getGlobalBounds();
    const float below = rulesBounds.position.y + rulesBounds.size.y + 30.f;
    computerHeading_.setPosition({textLeft, below});
    computer_.setPosition({textLeft, below + 26.f});
    // The panel ends just below its text.
    const auto computerBounds = computer_.getGlobalBounds();
    const float closeTop = std::max(computerBounds.position.y + computerBounds.size.y + 34.f, 330.f);
    closeButton_.setPosition({870.f, closeTop});
    panel_.setSize({880.f, closeTop + 52.f + 28.f - panel_.getPosition().y});
    closeButton_.setHovered(false);
    visible_ = true;
}

bool BoardGameRulesOverlay::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (!visible_) return false;
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        using Key = sf::Keyboard::Key;
        return key->code == Key::Escape || key->code == Key::Enter || key->code == Key::Space || key->code == Key::F1;
    }
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        closeButton_.setHovered(closeButton_.contains(window.mapPixelToCoords(moved->position)));
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        return closeButton_.contains(window.mapPixelToCoords(click->position));
    }
    return false;
}

void BoardGameRulesOverlay::update(sf::Time deltaTime)
{
    if (visible_) closeButton_.update(deltaTime, context_.reducedMotion());
}

void BoardGameRulesOverlay::draw(sf::RenderTarget& target) const
{
    if (!visible_) return;
    target.draw(shade_);
    target.draw(panel_);
    target.draw(eyebrow_);
    target.draw(title_);
    target.draw(rules_);
    target.draw(computerHeading_);
    target.draw(computer_);
    closeButton_.draw(target);
}
