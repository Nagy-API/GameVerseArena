#include "AboutScene.hpp"

#include "Theme.hpp"
#include "Version.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

AboutScene::AboutScene(AppContext& context, std::string description)
    : context_(context),
      kicker_(context.semiboldFont, "ABOUT", Theme::labelSize),
      title_(context.semiboldFont, "GameVerseArena", Theme::pageTitleSize),
      detailsCard_({1136.f, 380.f}),
      version_(context.semiboldFont, std::string("Version ") + app_version::text, 20),
      technology_(context.semiboldFont, "C++17  |  SFML 3.1.0  |  SQLite 3.53.4", 22),
      milestone_(context.semiboldFont, "TURN-BASED + REAL-TIME  |  LOCAL ONLY", Theme::labelSize),
      description_(context.regularFont, std::move(description), Theme::bodySize),
      audioStatus_(context.regularFont, "", 15),
      backButton_(context.semiboldFont, "Back", {170.f, 54.f})
{
    kicker_.setPosition({Theme::pageMargin, 54.f});
    kicker_.setFillColor(Theme::secondary);
    title_.setPosition({Theme::pageMargin, 84.f});
    title_.setFillColor(Theme::textPrimary);

    detailsCard_.setPosition({Theme::pageMargin, 180.f});
    detailsCard_.setFillColor(Theme::backgroundRaised);
    detailsCard_.setOutlineThickness(1.f);
    detailsCard_.setOutlineColor(Theme::border);
    version_.setPosition({108.f, 212.f});
    version_.setFillColor(Theme::warning);
    technology_.setPosition({108.f, 250.f});
    technology_.setFillColor(Theme::primaryBright);
    milestone_.setPosition({108.f, 300.f});
    milestone_.setFillColor(Theme::secondary);
    description_.setPosition({108.f, 340.f});
    description_.setFillColor(Theme::textSecondary);
    description_.setLineSpacing(1.45f);
    audioStatus_.setPosition({108.f, 520.f});
    audioStatus_.setFillColor(Theme::textMuted);

    backButton_.setPosition({Theme::pageMargin, 610.f});
    backButton_.setSelected(true);
}

void AboutScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
{
    if (const auto* key = event.getIf<sf::Event::KeyPressed>();
        key && (key->code == sf::Keyboard::Key::Escape || key->code == sf::Keyboard::Key::Enter ||
                key->code == sf::Keyboard::Key::Space)) {
        goBack();
        return;
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
    backButton_.update(deltaTime, context_.reducedMotion());
}

void AboutScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_);
    window.draw(title_);
    window.draw(detailsCard_);
    window.draw(version_);
    window.draw(technology_);
    window.draw(milestone_);
    window.draw(description_);
    window.draw(audioStatus_);
    backButton_.draw(window);
}

void AboutScene::onResize(sf::Vector2u)
{
}

void AboutScene::onActivate()
{
    backButton_.setHovered(false);
    audioStatus_.setString("Audio: " + context_.audio.status());
}

void AboutScene::goBack()
{
    context_.play(audio::SoundId::UiBack);
    context_.scenes.switchTo(SceneId::MainMenu);
}
