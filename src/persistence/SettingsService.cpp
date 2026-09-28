#include "SettingsService.hpp"

#include <chrono>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace persistence {
namespace {
std::int64_t currentUtcMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// Accepts only plain decimal digits so values such as " 50", "+50", "5e1", or "50%" are
// treated as malformed instead of being partially parsed.
std::optional<int> parseVolume(const std::string& text)
{
    if (text.empty() || text.size() > 3) return std::nullopt;
    int value = 0;
    for (const char character : text) {
        if (character < '0' || character > '9') return std::nullopt;
        value = value * 10 + (character - '0');
    }
    if (!validVolume(value)) return std::nullopt;
    return value;
}

std::optional<bool> parseFlag(const std::string& text)
{
    if (text == "1") return true;
    if (text == "0") return false;
    return std::nullopt;
}

void readVolume(const std::map<std::string, std::string>& stored, const char* key, int& target,
                std::vector<std::string>& issues)
{
    const auto found = stored.find(key);
    if (found == stored.end()) return;
    if (const auto value = parseVolume(found->second)) {
        target = *value;
    } else {
        issues.push_back(std::string("Setting '") + key + "' had an invalid stored value; the default " +
                         std::to_string(target) + " is used instead.");
    }
}

void readFlag(const std::map<std::string, std::string>& stored, const char* key, bool& target,
              std::vector<std::string>& issues)
{
    const auto found = stored.find(key);
    if (found == stored.end()) return;
    if (const auto value = parseFlag(found->second)) {
        target = *value;
    } else {
        issues.push_back(std::string("Setting '") + key + "' had an invalid stored value; the default " +
                         (target ? "On" : "Off") + " is used instead.");
    }
}
} // namespace

SettingsService::SettingsService(Database& database, UtcNow utcNow)
    : repository_(database), utcNow_(utcNow ? std::move(utcNow) : UtcNow{currentUtcMilliseconds})
{
}

SettingsLoadResult SettingsService::parse(const std::map<std::string, std::string>& stored)
{
    SettingsLoadResult result;
    result.settings = defaultSettings();
    readVolume(stored, settings_keys::masterVolume, result.settings.masterVolume, result.issues);
    readVolume(stored, settings_keys::uiVolume, result.settings.uiVolume, result.issues);
    readVolume(stored, settings_keys::gameplayVolume, result.settings.gameplayVolume, result.issues);
    readVolume(stored, settings_keys::achievementVolume, result.settings.achievementVolume, result.issues);
    readFlag(stored, settings_keys::audioMuted, result.settings.audioMuted, result.issues);
    readFlag(stored, settings_keys::reducedMotion, result.settings.reducedMotion, result.issues);
    return result;
}

SettingsLoadResult SettingsService::load() const
{
    return parse(repository_.loadAll());
}

void SettingsService::save(const AppSettings& settings)
{
    if (!validSettings(settings)) {
        throw std::invalid_argument("Volume settings must be between 0 and 100");
    }
    repository_.upsert({
        {settings_keys::masterVolume, std::to_string(settings.masterVolume)},
        {settings_keys::uiVolume, std::to_string(settings.uiVolume)},
        {settings_keys::gameplayVolume, std::to_string(settings.gameplayVolume)},
        {settings_keys::achievementVolume, std::to_string(settings.achievementVolume)},
        {settings_keys::audioMuted, settings.audioMuted ? "1" : "0"},
        {settings_keys::reducedMotion, settings.reducedMotion ? "1" : "0"},
    }, utcNow_());
}

AppSettings SettingsService::resetToDefaults()
{
    const auto defaults = defaultSettings();
    save(defaults);
    return defaults;
}

} // namespace persistence
