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
      subtitle_(regularFont, "Choose a turn-based classic or enter the real-time arcade.", Theme::bodySize),
      classicCard_(semiboldFont, "Classic Tic-Tac-Toe", {530.f, 118.f}),
      classicDetails_(regularFont, "PLAYABLE IN GUI  |  3x3  |  LOCAL OR AI", Theme::labelSize),
      consoleCard_({1136.f, 170.f}),
      pingPongCard_(semiboldFont, "Ping Pong", {530.f, 118.f}),
      pingPongDetails_(regularFont, "ARCADE GAMES  |  REAL-TIME  |  LOCAL OR AI", Theme::labelSize),
      consoleTitle_(semiboldFont, "13 More Board Games", 25),
      consoleBadge_(semiboldFont, "AVAILABLE IN CONSOLE", Theme::labelSize),
      consoleDescription_(regularFont, "Numerical, SUS, 5x5, Misere, Four-in-a-Row,\n4x4, Word, Pyramid, Diamond, Infinity,\nUltimate, Memory, and Obstacle.", 16),
      backButton_(semiboldFont, "Back", {170.f, 54.f})
{
    kicker_.setPosition({Theme::pageMargin, 42.f}); kicker_.setFillColor(Theme::secondary);
    title_.setPosition({Theme::pageMargin, 70.f}); title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({Theme::pageMargin, 125.f}); subtitle_.setFillColor(Theme::textSecondary);

    classicCard_.setPosition({72.f, 190.f});
    classicDetails_.setPosition({98.f, 268.f}); classicDetails_.setFillColor(Theme::secondary);
    pingPongCard_.setPosition({678.f, 190.f});
    pingPongDetails_.setPosition({704.f, 268.f}); pingPongDetails_.setFillColor(Theme::arcadeRight);
    consoleCard_.setPosition({72.f, 335.f}); consoleCard_.setFillColor(Theme::backgroundRaised);
    consoleCard_.setOutlineThickness(1.f); consoleCard_.setOutlineColor(Theme::border);

    consoleTitle_.setPosition({98.f, 362.f}); consoleTitle_.setFillColor(Theme::textPrimary);
    consoleBadge_.setPosition({98.f, 402.f}); consoleBadge_.setFillColor(Theme::warning);
    consoleDescription_.setPosition({98.f, 438.f}); consoleDescription_.setFillColor(Theme::textSecondary);
    consoleDescription_.setLineSpacing(1.25f);

    backButton_.setPosition({72.f, 610.f});
    refreshSelection();
}

void GameLibraryScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Escape) goBack();
        else if (key->code == sf::Keyboard::Key::Up || key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::Tab) {
            const int direction = key->code == sf::Keyboard::Key::Up ? -1 : 1;
            selectedIndex_ = static_cast<std::size_t>((static_cast<int>(selectedIndex_) + direction + 3) % 3);
            refreshSelection();
        } else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space) {
            if (selectedIndex_ == 0) openClassic();
            else if (selectedIndex_ == 1) openPingPong();
            else goBack();
        }
    }

    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        const auto point = window.mapPixelToCoords(moved->position);
        classicCard_.setHovered(classicCard_.contains(point));
        pingPongCard_.setHovered(pingPongCard_.contains(point));
        backButton_.setHovered(backButton_.contains(point));
        if (classicCard_.contains(point)) selectedIndex_ = 0;
        if (pingPongCard_.contains(point)) selectedIndex_ = 1;
        if (backButton_.contains(point)) selectedIndex_ = 2;
        refreshSelection();
    }

    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left) {
        const auto point = window.mapPixelToCoords(click->position);
        if (classicCard_.contains(point)) openClassic();
        else if (pingPongCard_.contains(point)) openPingPong();
        else if (backButton_.contains(point)) goBack();
    }
}

void GameLibraryScene::update(sf::Time deltaTime)
{
    classicCard_.update(deltaTime); pingPongCard_.update(deltaTime); backButton_.update(deltaTime);
}

void GameLibraryScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_); window.draw(title_); window.draw(subtitle_);
    classicCard_.draw(window); window.draw(classicDetails_);
    pingPongCard_.draw(window); window.draw(pingPongDetails_);
    window.draw(consoleCard_); window.draw(consoleTitle_); window.draw(consoleBadge_); window.draw(consoleDescription_);
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
void GameLibraryScene::openPingPong() { sceneManager_.switchTo(SceneId::PingPongSetup); }

void GameLibraryScene::refreshSelection()
{
    classicCard_.setSelected(selectedIndex_ == 0);
    pingPongCard_.setSelected(selectedIndex_ == 1);
    backButton_.setSelected(selectedIndex_ == 2);
}
