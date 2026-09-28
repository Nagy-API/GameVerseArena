#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace persistence {

class Database;

// Owns all SQL for the `app_settings` key/value table.
class SettingsRepository {
public:
    explicit SettingsRepository(Database& database) : database_(database) {}

    std::map<std::string, std::string> loadAll() const;
    void upsert(const std::vector<std::pair<std::string, std::string>>& values, std::int64_t updatedAt);

private:
    Database& database_;
};

} // namespace persistence
