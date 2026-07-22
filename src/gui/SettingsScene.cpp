#include "SettingsScene.hpp"

#include "Theme.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <array>
#include <string>

SettingsScene::SettingsScene(const sf::Font& regularFont,
                             const sf::Font& semiboldFont,
                             SceneManager& sceneManager)
    : sceneManager_(sceneManager),
      kicker_(semiboldFont, "SETTINGS", Theme::labelSize),
      title_(semiboldFont, "Personalize your arena", Theme::pageTitleSize),
      subtitle_(regularFont, "A preview of planned controls. These options are not functional yet.", Theme::bodySize),
      backButton_(semiboldFont, "Back", {170.f, 54.f})
{
    kicker_.setPosition({Theme::pageMargin, 54.f});
    kicker_.setFillColor(Theme::secondary);
    title_.setPosition({Theme::pageMargin, 84.f});
    title_.setFillColor(Theme::textPrimary);
    subtitle_.setPosition({Theme::pageMargin, 145.f});
    subtitle_.setFillColor(Theme::textSecondary);

    const std::array<std::string, 4> labels{"Display", "Audio", "Controls", "Theme"};
    cards_.reserve(labels.size());
    cardTitles_.reserve(labels.size());
    cardStatuses_.reserve(labels.size());
    for (std::size_t index = 0; index < labels.size(); ++index) {
        const float x = 72.f + static_cast<float>(index % 2) * 568.f;
        const float y = 220.f + static_cast<float>(index / 2) * 150.f;

        cards_.emplace_back(sf::Vector2f{540.f, 122.f});
        cards_.back().setPosition({x, y});
        cards_.back().setFillColor(Theme::backgroundRaised);
        cards_.back().setOutlineThickness(1.f);
        cards_.back().setOutlineColor(Theme::border);

        cardTitles_.emplace_back(semiboldFont, labels[index], 23);
        cardTitles_.back().setPosition({x + 28.f, y + 24.f});
        cardTitles_.back().setFillColor(Theme::textPrimary);

        cardStatuses_.emplace_back(regularFont, "Future setting  /  Placeholder", Theme::labelSize);
        cardStatuses_.back().setPosition({x + 28.f, y + 72.f});
        cardStatuses_.back().setFillColor(Theme::textMuted);
    }

    backButton_.setPosition({Theme::pageMargin, 610.f});
    backButton_.setSelected(true);
}

void SettingsScene::handleEvent(const sf::Event& event, sf::RenderWindow& window)
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

void SettingsScene::update(sf::Time deltaTime)
{
    backButton_.update(deltaTime);
}

void SettingsScene::render(sf::RenderWindow& window) const
{
    window.draw(kicker_);
    window.draw(title_);
    window.draw(subtitle_);
    for (std::size_t index = 0; index < cards_.size(); ++index) {
        window.draw(cards_[index]);
        window.draw(cardTitles_[index]);
        window.draw(cardStatuses_[index]);
    }
    backButton_.draw(window);
}

void SettingsScene::onResize(sf::Vector2u)
{
}

void SettingsScene::goBack()
{
    sceneManager_.switchTo(SceneId::MainMenu);
}
