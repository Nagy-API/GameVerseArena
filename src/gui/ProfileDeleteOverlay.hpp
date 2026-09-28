#pragma once

#include "AppContext.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

#include <string>

namespace sf { class RenderWindow; }

class ProfileDeleteOverlay {
public:
    enum class Result { None, Confirm, Cancel };

    explicit ProfileDeleteOverlay(AppContext& context);
    void open(std::string profileName, bool activeProfile);
    bool isOpen() const noexcept { return open_; }
    void close() noexcept { open_ = false; }
    Result handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(sf::Time deltaTime);
    void render(sf::RenderWindow& window) const;

private:
    void refresh();

    AppContext& context_;
    sf::RectangleShape backdrop_;
    sf::RectangleShape panel_;
    sf::Text title_;
    sf::Text question_;
    sf::Text explanation_;
    UiButton deleteButton_;
    UiButton cancelButton_;
    std::string profileName_;
    bool activeProfile_{};
    bool open_{};
    std::size_t selectedButton_{1};
};
