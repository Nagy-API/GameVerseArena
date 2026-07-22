#include "AboutScene.hpp"

#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

AboutScene::AboutScene(const sf::Font& regularFont,
                       const sf::Font& semiboldFont,
                       SceneManager& sceneManager)
    : sceneManager_(sceneManager),
      kicker_(semiboldFont, "ABOUT", Theme::labelSize),
      title_(semiboldFont, "GameVerseArena", Theme::pageTitleSize),
      detailsCard_({1136.f, 340.f}),
      technology_(semiboldFont, "C++17  |  SFML 3.1.0", 22),
      milestone_(semiboldFont, "GRAPHICAL SHELL MILESTONE", Theme::labelSize),
      description_(regularFont,
                   "This milestone introduces a polished graphical launcher and a scene-based foundation.\n"
                   "The 14 board games remain fully available in the separate console application; no game\n"
                   "has been migrated to the graphical interface yet.",
                   Theme::bodySize),
      backButton_(semiboldFont, "Back", {170.f, 54.f})
{
    kicker_.setPosition({Theme::pageMargin, 54.f});
    kicker_.setFillColor(Theme::secondary);
    title_.setPosition({Theme::pageMargin, 84.f});
    title_.setFillColor(Theme::textPrimary);

    detailsCard_.setPosition({Theme::pageMargin, 190.f});
    detailsCard_.setFillColor(Theme::backgroundRaised);
    detailsCard_.setOutlineThickness(1.f);
    detailsCard_.setOutlineColor(Theme::border);
    technology_.setPosition({108.f, 238.f});
    technology_.setFillColor(Theme::primaryBright);
    milestone_.setPosition({108.f, 302.f});
    milestone_.setFillColor(Theme::secondary);
    description_.setPosition({108.f, 350.f});
    description_.setFillColor(Theme::textSecondary);
    description_.setLineSpacing(1.55f);

    backButton_.setPosition({Theme::pageMargin, 610.f});
    backButton_.setSelected(true);
}

void AboutScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
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

void AboutScene::update(sf::Time deltaTime)
{
    backButton_.update(deltaTime);
}

void AboutScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_);
    window.draw(title_);
    window.draw(detailsCard_);
    window.draw(technology_);
    window.draw(milestone_);
    window.draw(description_);
    backButton_.draw(window);
}

void AboutScene::onResize(sf::Vector2u)
{
}

void AboutScene::goBack()
{
    sceneManager_.switchTo(SceneId::MainMenu);
}
