#include "GameLibraryScene.hpp"

#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

GameLibraryScene::GameLibraryScene(const sf::Font& regularFont,
                                   const sf::Font& semiboldFont,
                                   SceneManager& sceneManager)
    : sceneManager_(sceneManager),
      kicker_(semiboldFont, "GAME LIBRARY", Theme::labelSize),
      title_(semiboldFont, "Choose your arena", Theme::pageTitleSize),
      subtitle_(regularFont, "The console collection still contains all 14 games. One is now also playable here.", Theme::bodySize),
      classicCard_(semiboldFont, "Classic Tic-Tac-Toe", {530.f, 118.f}),
      classicDetails_(regularFont, "PLAYABLE IN GUI  |  3x3  |  LOCAL OR AI", Theme::labelSize),
      consoleCard_({530.f, 190.f}), arcadeCard_({530.f, 190.f}),
      consoleTitle_(semiboldFont, "13 More Board Games", 25),
      consoleBadge_(semiboldFont, "AVAILABLE IN CONSOLE", Theme::labelSize),
      consoleDescription_(regularFont, "Numerical, SUS, 5x5, Misere, Four-in-a-Row,\n4x4, Word, Pyramid, Diamond, Infinity,\nUltimate, Memory, and Obstacle.", 16),
      arcadeTitle_(semiboldFont, "Ping Pong", 25),
      plannedBadge_(semiboldFont, "PLANNED", Theme::labelSize),
      arcadeDescription_(regularFont, "The future real-time arcade layer remains\non the roadmap and is not playable yet.", 16),
      backButton_(semiboldFont, "Back", {170.f, 54.f})
{
    kicker_.setPosition({Theme::pageMargin, 42.f}); kicker_.setFillColor(Theme::secondary);
    title_.setPosition({Theme::pageMargin, 70.f}); title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({Theme::pageMargin, 125.f}); subtitle_.setFillColor(Theme::textSecondary);

    classicCard_.setPosition({72.f, 190.f});
    classicDetails_.setPosition({98.f, 268.f}); classicDetails_.setFillColor(Theme::secondary);
    consoleCard_.setPosition({72.f, 335.f}); consoleCard_.setFillColor(Theme::backgroundRaised);
    consoleCard_.setOutlineThickness(1.f); consoleCard_.setOutlineColor(Theme::border);
    arcadeCard_.setPosition({678.f, 190.f}); arcadeCard_.setFillColor(Theme::backgroundRaised);
    arcadeCard_.setOutlineThickness(1.f); arcadeCard_.setOutlineColor(Theme::border);

    consoleTitle_.setPosition({98.f, 362.f}); consoleTitle_.setFillColor(Theme::textPrimary);
    consoleBadge_.setPosition({98.f, 402.f}); consoleBadge_.setFillColor(Theme::warning);
    consoleDescription_.setPosition({98.f, 438.f}); consoleDescription_.setFillColor(Theme::textSecondary);
    consoleDescription_.setLineSpacing(1.25f);
    arcadeTitle_.setPosition({704.f, 220.f}); arcadeTitle_.setFillColor(Theme::textPrimary);
    plannedBadge_.setPosition({704.f, 263.f}); plannedBadge_.setFillColor(Theme::warning);
    arcadeDescription_.setPosition({704.f, 310.f}); arcadeDescription_.setFillColor(Theme::textSecondary);
    arcadeDescription_.setLineSpacing(1.35f);

    backButton_.setPosition({72.f, 610.f});
    refreshSelection();
}

void GameLibraryScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) goBack();
        else if (key->code == sf::Keyboard::Key::Up || key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::Tab) {
            selectedIndex_ = 1 - selectedIndex_; refreshSelection();
        } else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) {
            selectedIndex_ == 0 ? openClassic() : goBack();
        }
    }

    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        classicCard_.setHovered(classicCard_.contains(point)); backButton_.setHovered(backButton_.contains(point));
        if (classicCard_.contains(point)) selectedIndex_ = 0;
        if (backButton_.contains(point)) selectedIndex_ = 1;
        refreshSelection();
    }

    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        if (classicCard_.contains(point)) openClassic();
        else if (backButton_.contains(point)) goBack();
    }
}

void GameLibraryScene::update(sf::Time deltaTime)
{
    classicCard_.update(deltaTime); backButton_.update(deltaTime);
}

void GameLibraryScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_); window.draw(title_); window.draw(subtitle_);
    classicCard_.draw(window); window.draw(classicDetails_);
    window.draw(consoleCard_); window.draw(consoleTitle_); window.draw(consoleBadge_); window.draw(consoleDescription_);
    window.draw(arcadeCard_); window.draw(arcadeTitle_); window.draw(plannedBadge_); window.draw(arcadeDescription_);
    backButton_.draw(window);
}

void GameLibraryScene::onResize(sf::Vector2u) {}

void GameLibraryScene::onActivate()
{
    selectedIndex_ = 0;
    refreshSelection();
}

void GameLibraryScene::goBack() { sceneManager_.switchTo(SceneId::MainMenu); }
void GameLibraryScene::openClassic() { sceneManager_.switchTo(SceneId::TicTacToeSetup); }

void GameLibraryScene::refreshSelection()
{
    classicCard_.setSelected(selectedIndex_ == 0);
    backButton_.setSelected(selectedIndex_ == 1);
}
