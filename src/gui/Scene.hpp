#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>

class Scene {
public:
    virtual ~Scene() = default;

    virtual void handleEvent(const sf::Event& event, sf::RenderWindow& window) = 0;
    virtual void update(sf::Time deltaTime) = 0;
    virtual void render(sf::RenderWindow& window) const = 0;
    virtual void onResize(sf::Vector2u size) = 0;
    virtual void onActivate() {}
};
