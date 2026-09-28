#include "SettingsController.hpp"

#include "AudioEngine.hpp"

#include <exception>
#include <iostream>

SettingsController::SettingsController(persistence::SettingsService& service, AudioEngine& audio)
    : service_(service), audio_(audio)
{
    pushToAudio();
}

void SettingsController::load()
{
    try {
        auto result = service_.load();
        current_ = result.settings;
        loadIssues_ = std::move(result.issues);
        for (const auto& issue : loadIssues_) std::cerr << "GameVerseArenaGUI: " << issue << '\n';
    } catch (const std::exception& error) {
        current_ = persistence::defaultSettings();
        loadIssues_ = {std::string("Settings could not be read; defaults are active. ") + error.what()};
        std::cerr << "GameVerseArenaGUI: " << loadIssues_.front() << '\n';
    }
    pushToAudio();
}

bool SettingsController::apply(const persistence::AppSettings& settings)
{
    if (!persistence::validSettings(settings)) {
        lastError_ = "Volume settings must be between 0 and 100.";
        return false;
    }
    current_ = settings;
    pushToAudio();
    try {
        service_.save(current_);
        lastError_.clear();
        loadIssues_.clear();
        return true;
    } catch (const std::exception& error) {
        lastError_ = "Settings are active but could not be saved.";
        std::cerr << "GameVerseArenaGUI: settings save failed: " << error.what() << '\n';
        return false;
    }
}

bool SettingsController::resetToDefaults()
{
    return apply(persistence::defaultSettings());
}

audio::MixLevels SettingsController::mixFor(const persistence::AppSettings& settings) noexcept
{
    audio::MixLevels levels;
    levels.master = settings.masterVolume;
    levels.ui = settings.uiVolume;
    levels.gameplay = settings.gameplayVolume;
    levels.achievement = settings.achievementVolume;
    levels.muted = settings.audioMuted;
    return levels;
}

void SettingsController::pushToAudio()
{
    audio_.setMix(mixFor(current_));
}
