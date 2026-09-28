#pragma once

#include "SettingsService.hpp"
#include "SettingsTypes.hpp"
#include "SoundTypes.hpp"

#include <string>
#include <vector>

class AudioEngine;

// Holds the active app-wide settings, applies them immediately (audio mix, reduced motion),
// and persists every change through SettingsService. A failed save keeps the new values
// active for this session and reports the problem instead of discarding the change.
class SettingsController {
public:
    SettingsController(persistence::SettingsService& service, AudioEngine& audio);

    void load();
    const persistence::AppSettings& current() const noexcept { return current_; }
    bool reducedMotion() const noexcept { return current_.reducedMotion; }

    bool apply(const persistence::AppSettings& settings);
    bool resetToDefaults();

    const std::vector<std::string>& loadIssues() const noexcept { return loadIssues_; }
    const std::string& lastError() const noexcept { return lastError_; }

    static audio::MixLevels mixFor(const persistence::AppSettings& settings) noexcept;

private:
    void pushToAudio();

    persistence::SettingsService& service_;
    AudioEngine& audio_;
    persistence::AppSettings current_{};
    std::vector<std::string> loadIssues_;
    std::string lastError_;
};
