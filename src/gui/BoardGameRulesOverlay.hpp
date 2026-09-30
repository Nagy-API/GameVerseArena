#pragma once

#include "AppContext.hpp"
#include "GameCatalogue.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

// "How to play" panel for the shared board-game scenes: the game's rules and a truthful
// description of its computer opponent, from the catalogue.
class BoardGameRulesOverlay {
public:
    explicit BoardGameRulesOverlay(AppContext& context);

    void show(const catalogue::GameDescriptor& game);
    void hide() noexcept { visible_ = false; }
    bool visible() const noexcept { return visible_; }
    // Returns true when the player closed the panel.
    bool handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(sf::Time deltaTime);
    void draw(sf::RenderTarget& target) const;

private:
    AppContext& context_;
    sf::RectangleShape shade_;
    sf::RectangleShape panel_;
    sf::Text eyebrow_;
    sf::Text title_;
    sf::Text rules_;
    sf::Text computerHeading_;
    sf::Text computer_;
    UiButton closeButton_;
    bool visible_{false};
};
