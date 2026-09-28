#pragma once

#include "AppContext.hpp"
#include "Scene.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include <string>

class AboutScene final : public Scene {
public:
    AboutScene(AppContext& context, std::string description);

    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    void update(sf::Time deltaTime) override;
    void render(sf::RenderWindow& window) const override;
    void onResize(sf::Vector2u size) override;
    void onActivate() override;

private:
    void goBack();

    AppContext& context_;
    sf::Text kicker_;
    sf::Text title_;
    sf::RectangleShape detailsCard_;
    sf::Text version_;
    sf::Text technology_;
    sf::Text milestone_;
    sf::Text description_;
    sf::Text audioStatus_;
    UiButton backButton_;
};
