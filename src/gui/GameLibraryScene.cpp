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
      subtitle_(regularFont, "The graphical library is a preview; existing games remain in the console application.", Theme::bodySize),
      boardCard_({530.f, 280.f}),
      arcadeCard_({530.f, 280.f}),
      boardTitle_(semiboldFont, "Board Games", 28),
      boardCount_(semiboldFont, "14 AVAILABLE", Theme::labelSize),
      boardDescription_(regularFont, "All 14 turn-based games are playable today\nin the existing console version.", Theme::bodySize),
      arcadeTitle_(semiboldFont, "Arcade Games", 28),
      plannedBadge_(semiboldFont, "PING PONG  -  PLANNED", Theme::labelSize),
      arcadeDescription_(regularFont, "Real-time games are part of the roadmap.\nNo arcade game is playable in this shell yet.", Theme::bodySize),
      backButton_(semiboldFont, "Back", {170.f, 54.f})
{
    kicker_.setPosition({Theme::pageMargin, 54.f});
    kicker_.setFillColor(Theme::secondary);
    title_.setPosition({Theme::pageMargin, 84.f});
    title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({Theme::pageMargin, 145.f});
    subtitle_.setFillColor(Theme::textSecondary);

    boardCard_.setPosition({Theme::pageMargin, 220.f});
    boardCard_.setFillColor(Theme::backgroundRaised);
    boardCard_.setOutlineThickness(1.f);
    boardCard_.setOutlineColor(Theme::primary);
    arcadeCard_.setPosition({678.f, 220.f});
    arcadeCard_.setFillColor(Theme::backgroundRaised);
    arcadeCard_.setOutlineThickness(1.f);
    arcadeCard_.setOutlineColor(Theme::border);

    boardTitle_.setPosition({104.f, 260.f});
    boardTitle_.setFillColor(Theme::textPrimary);
    boardCount_.setPosition({104.f, 320.f});
    boardCount_.setFillColor(Theme::secondary);
    boardDescription_.setPosition({104.f, 382.f});
    boardDescription_.setFillColor(Theme::textSecondary);
    boardDescription_.setLineSpacing(1.5f);

    arcadeTitle_.setPosition({710.f, 260.f});
    arcadeTitle_.setFillColor(Theme::textPrimary);
    plannedBadge_.setPosition({710.f, 320.f});
    plannedBadge_.setFillColor(Theme::warning);
    arcadeDescription_.setPosition({710.f, 382.f});
    arcadeDescription_.setFillColor(Theme::textSecondary);
    arcadeDescription_.setLineSpacing(1.5f);

    backButton_.setPosition({Theme::pageMargin, 610.f});
    backButton_.setSelected(true);
}

void GameLibraryScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>();
        key && (key->code == sf::Keyboard::Key::Escape || key->code == sf::Keyboard::Key::Enter)) {
        goBack();
    }

    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        backButton_.setHovered(backButton_.contains(window.mapPixelToCoords(moved->position)));
    }

    if (const auto* click = event.getIf<sf::Event::MouseButtonReleased>();
        click && click->button == sf::Mouse::Button::Left &&
        backButton_.contains(window.mapPixelToCoords(click->position))) {
        goBack();
    }
}

void GameLibraryScene::update(sf::Time deltaTime)
{
    backButton_.update(deltaTime);
}

void GameLibraryScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_);
    window.draw(title_);
    window.draw(subtitle_);
    window.draw(boardCard_);
    window.draw(arcadeCard_);
    window.draw(boardTitle_);
    window.draw(boardCount_);
    window.draw(boardDescription_);
    window.draw(arcadeTitle_);
    window.draw(plannedBadge_);
    window.draw(arcadeDescription_);
    backButton_.draw(window);
}

void GameLibraryScene::onResize(sf::Vector2u)
{
}

void GameLibraryScene::goBack()
{
    sceneManager_.switchTo(SceneId::MainMenu);
}
