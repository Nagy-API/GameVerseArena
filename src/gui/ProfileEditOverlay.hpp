#pragma once

#include "AppContext.hpp"
#include "UiButton.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

#include <string>

namespace sf { class RenderWindow; }

class ProfileEditOverlay {
public:
    enum class Mode { Create, Rename };
    enum class Result { None, Confirm, Cancel };

    explicit ProfileEditOverlay(AppContext& context);

    void open(Mode mode, std::string initialValue = {});
    bool isOpen() const noexcept { return open_; }
    void close() noexcept { open_ = false; }
    const std::string& value() const noexcept { return value_; }
    void setError(std::string error);
    Result handleEvent(const sf::Event& event, sf::RenderWindow& window);
    void update(sf::Time deltaTime);
    void render(sf::RenderWindow& window) const;

private:
    void refresh();

    AppContext& context_;
    sf::RectangleShape backdrop_;
    sf::RectangleShape panel_;
    sf::RectangleShape field_;
    sf::Text title_;
    sf::Text prompt_;
    sf::Text valueText_;
    sf::Text errorText_;
    sf::Text help_;
    UiButton confirmButton_;
    UiButton cancelButton_;
    Mode mode_{Mode::Create};
    std::string value_;
    std::string error_;
    std::size_t selectedButton_{};
    bool open_{false};
};
