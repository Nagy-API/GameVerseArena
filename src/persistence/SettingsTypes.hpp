#pragma once

#include <string>
#include <vector>

namespace persistence {

// App-wide preferences. They are stored separately from player profiles and apply to
// every profile on this machine.
struct AppSettings {
    int masterVolume{80};
    int uiVolume{70};
    int gameplayVolume{80};
    int achievementVolume{85};
    bool audioMuted{false};
    bool reducedMotion{false};
};

inline bool operator==(const AppSettings& left, const AppSettings& right)
{
    return left.masterVolume == right.masterVolume && left.uiVolume == right.uiVolume &&
           left.gameplayVolume == right.gameplayVolume && left.achievementVolume == right.achievementVolume &&
           left.audioMuted == right.audioMuted && left.reducedMotion == right.reducedMotion;
}

inline bool operator!=(const AppSettings& left, const AppSettings& right) { return !(left == right); }

namespace settings_keys {
inline constexpr const char* masterVolume = "audio.master_volume";
inline constexpr const char* uiVolume = "audio.ui_volume";
inline constexpr const char* gameplayVolume = "audio.gameplay_volume";
inline constexpr const char* achievementVolume = "audio.achievement_volume";
inline constexpr const char* audioMuted = "audio.muted";
inline constexpr const char* reducedMotion = "accessibility.reduced_motion";
} // namespace settings_keys

inline constexpr int minimumVolume = 0;
inline constexpr int maximumVolume = 100;

inline AppSettings defaultSettings() { return AppSettings{}; }

inline bool validVolume(int value) noexcept { return value >= minimumVolume && value <= maximumVolume; }

inline bool validSettings(const AppSettings& settings) noexcept
{
    return validVolume(settings.masterVolume) && validVolume(settings.uiVolume) &&
           validVolume(settings.gameplayVolume) && validVolume(settings.achievementVolume);
}

// Result of loading settings. Stored values that cannot be parsed or are out of range are
// replaced by their defaults and described in `issues`; they are never silently accepted.
struct SettingsLoadResult {
    AppSettings settings;
    std::vector<std::string> issues;
};

} // namespace persistence
