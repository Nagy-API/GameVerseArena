#include "SettingsRepository.hpp"

#include "Database.hpp"

namespace persistence {

std::map<std::string, std::string> SettingsRepository::loadAll() const
{
    std::map<std::string, std::string> values;
    auto statement = database_.prepare("SELECT key, value FROM app_settings ORDER BY key;");
    while (statement.step()) values.emplace(statement.text(0), statement.text(1));
    return values;
}

void SettingsRepository::upsert(const std::vector<std::pair<std::string, std::string>>& values,
                                std::int64_t updatedAt)
{
    auto transaction = database_.transaction();
    for (const auto& [key, value] : values) {
        auto statement = database_.prepare(
            "INSERT INTO app_settings(key, value, updated_at) VALUES(?1, ?2, ?3) "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value, updated_at = excluded.updated_at;");
        statement.bind(1, key);
        statement.bind(2, value);
        statement.bind(3, updatedAt);
        statement.step();
    }
    transaction.commit();
}

} // namespace persistence
