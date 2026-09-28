#include "UiButton.hpp"

#include "Theme.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace {
sf::Color blend(const sf::Color& from, const sf::Color& to, float amount)
{
    const auto channel = [amount](std::uint8_t first, std::uint8_t second) {
        return static_cast<std::uint8_t>(std::lround(
            static_cast<float>(first) + (static_cast<float>(second) - static_cast<float>(first)) * amount));
    };

    return {channel(from.r, to.r), channel(from.g, to.g), channel(from.b, to.b), channel(from.a, to.a)};
}
} // namespace

UiButton::UiButton(const sf::Font& font, std::string text, sf::Vector2f size)
    : background_(size), accentBar_({5.f, size.y}), label_(font, std::move(text), Theme::buttonTextSize)
{
    background_.setFillColor(Theme::panel);
    background_.setOutlineThickness(1.f);
    background_.setOutlineColor(Theme::border);
    accentBar_.setFillColor(Theme::primary);
    label_.setFillColor(Theme::textPrimary);
}

void UiButton::setPosition(sf::Vector2f position)
{
    background_.setPosition(position);
    accentBar_.setPosition(position);
    centerLabel();
}

void UiButton::setText(std::string text)
{
    label_.setString(std::move(text));
    centerLabel();
}

bool UiButton::contains(sf::Vector2f point) const
{
    return background_.getGlobalBounds().contains(point);
}

void UiButton::update(sf::Time deltaTime, bool reducedMotion)
{
    const float target = (hovered_ || selected_) ? 1.f : 0.f;
    if (reducedMotion) {
        emphasis_ = target;
    } else {
        const float step = std::min(1.f, Theme::animationSpeed * deltaTime.asSeconds());
        emphasis_ += (target - emphasis_) * step;
    }

    background_.setFillColor(blend(Theme::panel, Theme::panelHover, emphasis_));
    background_.setOutlineColor(blend(Theme::border, Theme::primaryBright, emphasis_));
    accentBar_.setFillColor(blend(Theme::primary, Theme::secondary, emphasis_));
    accentBar_.setScale({1.f + emphasis_ * 0.8f, 1.f});
}

void UiButton::draw(sf::RenderTarget& target) const
{
    target.draw(background_);
    target.draw(accentBar_);
    target.draw(label_);
}

void UiButton::centerLabel()
{
    const auto bounds = label_.getLocalBounds();
    const auto buttonPosition = background_.getPosition();
    const auto buttonSize = background_.getSize();
    label_.setPosition({
        buttonPosition.x + 24.f,
        buttonPosition.y + (buttonSize.y - bounds.size.y) / 2.f - bounds.position.y - 1.f
    });
}
