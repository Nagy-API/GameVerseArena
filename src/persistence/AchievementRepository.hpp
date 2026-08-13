#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace persistence {

class Database;

struct AchievementUnlock {
    std::int64_t profileId{};
    std::string achievementKey;
    std::int64_t unlockedAt{};
};

class AchievementRepository {
public:
    explicit AchievementRepository(Database& database) : database_(database) {}

    bool insert(std::int64_t profileId, const std::string& achievementKey, std::int64_t unlockedAt);
    std::vector<AchievementUnlock> listByProfile(std::int64_t profileId) const;
    std::vector<AchievementUnlock> listAll() const;

private:
    Database& database_;
};

} // namespace persistence
