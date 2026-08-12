#pragma once

#include "PersistenceTypes.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace persistence {

class Database;
class Statement;

class ProfileRepository {
public:
    explicit ProfileRepository(Database& database) : database_(database) {}

    std::vector<Profile> list() const;
    std::optional<Profile> findById(std::int64_t id) const;
    std::optional<Profile> findByName(const std::string& displayName,
                                      std::optional<std::int64_t> excludingId = std::nullopt) const;
    Profile create(const std::string& displayName);
    Profile rename(std::int64_t id, const std::string& displayName);
    bool remove(std::int64_t id);

    std::optional<Profile> active() const;
    void setActive(std::int64_t id);
    void clearActive();

private:
    static Profile readProfile(Statement& statement);

    Database& database_;
};

} // namespace persistence
