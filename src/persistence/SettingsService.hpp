#pragma once

#include "SettingsRepository.hpp"
#include "SettingsTypes.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <string>

namespace persistence {

class Database;

// Validates, loads, and saves app-wide settings. GUI code uses this service and never
// touches the settings SQL directly.
class SettingsService {
public:
    using UtcNow = std::function<std::int64_t()>;

    explicit SettingsService(Database& database, UtcNow utcNow = {});

    // Missing keys use defaults. Malformed or out-of-range stored values fall back to
    // their defaults and are reported in the returned issues.
    SettingsLoadResult load() const;

    // Persists every setting atomically. Throws std::invalid_argument for out-of-range
    // values; nothing is written in that case.
    void save(const AppSettings& settings);

    // Persists and returns the documented defaults.
    AppSettings resetToDefaults();

    static SettingsLoadResult parse(const std::map<std::string, std::string>& stored);

private:
    SettingsRepository repository_;
    UtcNow utcNow_;
};

} // namespace persistence
