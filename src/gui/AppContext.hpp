#pragma once

#include "AudioEngine.hpp"
#include "SceneManager.hpp"
#include "SettingsController.hpp"
#include "SoundTypes.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

// Shared UI infrastructure handed to every scene. Game and persistence services are still
// passed explicitly to the scenes that need them.
struct AppContext {
    const sf::Font& regularFont;
    const sf::Font& semiboldFont;
    SceneManager& scenes;
    sf::RenderWindow& window;
    AudioEngine& audio;
    SettingsController& settings;

    bool reducedMotion() const noexcept { return settings.reducedMotion(); }
    void play(audio::SoundId id) const { audio.play(id); }
};
