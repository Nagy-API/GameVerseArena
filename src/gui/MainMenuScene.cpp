#include "MainMenuScene.hpp"

#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <array>
#include <string>

using audio::SoundId;

MainMenuScene::MainMenuScene(AppContext& context)
    : context_(context),
      eyebrow_(context.semiboldFont, "WELCOME TO THE ARENA", Theme::labelSize),
      title_(context.semiboldFont, "GameVerseArena", Theme::titleSize),
      subtitle_(context.regularFont, "A unified C++ arena for timeless board games\nand real-time arcade experiences.", Theme::subtitleSize),
      footer_(context.regularFont, "Navigate with mouse or Up / Down / Enter", Theme::labelSize),
      featurePanel_({520.f, 720.f}),
      glowPrimary_(230.f),
      glowSecondary_(150.f)
{
    featurePanel_.setPosition({0.f, 0.f});
    featurePanel_.setFillColor(Theme::backgroundRaised);

    glowPrimary_.setPosition({-125.f, 430.f});
    glowPrimary_.setFillColor({Theme::primary.r, Theme::primary.g, Theme::primary.b, 18});
    glowSecondary_.setPosition({330.f, -110.f});
    glowSecondary_.setFillColor({Theme::secondary.r, Theme::secondary.g, Theme::secondary.b, 15});

    eyebrow_.setPosition({72.f, 172.f});
    eyebrow_.setFillColor(Theme::secondary);
    title_.setPosition({68.f, 205.f});
    title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({72.f, 294.f});
    subtitle_.setFillColor(Theme::textSecondary);
    subtitle_.setLineSpacing(1.35f);
    footer_.setPosition({72.f, 650.f});
    footer_.setFillColor(Theme::textMuted);

    const std::array<std::string, 5> labels{"Play", "Profiles", "Settings", "About", "Exit"};
    buttons_.reserve(labels.size());
    for (std::size_t index = 0; index < labels.size(); ++index) {
        buttons_.emplace_back(context.semiboldFont, labels[index], sf::Vector2f{Theme::buttonWidth, Theme::buttonHeight});
        buttons_.back().setPosition({820.f, 174.f + static_cast<float>(index) * (Theme::buttonHeight + Theme::buttonGap)});
    }
    select(0, false);
}

void MainMenuScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Up) {
            moveSelection(-1);
        } else if (key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::Tab) {
            moveSelection(key->code == sf::Keyboard::Key::Tab && key->shift ? -1 : 1);
        } else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) {
            activate(selectedIndex_);
        } else if (key->code == sf::Keyboard::Key::Escape) {
            context_.window.close();
        }
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
        for (std::size_t index = 0; index < buttons_.size(); ++index) {
            if (buttons_[index].contains(point)) {
                activate(index);
                break;
            }
        }
    }
}

void MainMenuScene::update(sf::Time deltaTime)
{
    for (auto& button : buttons_) {
        button.update(deltaTime, context_.reducedMotion());
    }
}

void MainMenuScene::render(sf::RenderWindow& window) const
{
    window.draw(featurePanel_);
    window.draw(glowPrimary_);
    window.draw(glowSecondary_);
    window.draw(eyebrow_);
    window.draw(title_);
    window.draw(subtitle_);
    window.draw(footer_);
    for (const auto& button : buttons_) {
        button.draw(window);
    }
}

void MainMenuScene::onResize(sf::Vector2u)
{
}

void MainMenuScene::onActivate()
{
    for (auto& button : buttons_) button.setHovered(false);
}

void MainMenuScene::activate(std::size_t index)
{
    select(index, false);
    switch (index) {
    case 0:
        context_.play(SoundId::UiConfirm);
        context_.scenes.switchTo(SceneId::GameLibrary);
        break;
    case 1:
        context_.play(SoundId::UiConfirm);
        context_.scenes.switchTo(SceneId::Profiles);
        break;
    case 2:
        context_.play(SoundId::UiConfirm);
        context_.scenes.switchTo(SceneId::Settings);
        break;
    case 3:
        context_.play(SoundId::UiConfirm);
        context_.scenes.switchTo(SceneId::About);
        break;
    case 4:
        context_.window.close();
        break;
    default:
        break;
    }
}

void MainMenuScene::moveSelection(int offset)
{
    const int count = static_cast<int>(buttons_.size());
    select(static_cast<std::size_t>((static_cast<int>(selectedIndex_) + offset + count) % count), true);
}

void MainMenuScene::select(std::size_t index, bool withSound)
{
    if (withSound && index != selectedIndex_) context_.play(SoundId::UiFocus);
    selectedIndex_ = index;
    for (std::size_t item = 0; item < buttons_.size(); ++item) {
        buttons_[item].setSelected(item == selectedIndex_);
    }
}
