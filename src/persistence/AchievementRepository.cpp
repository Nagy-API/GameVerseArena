#include "AchievementRepository.hpp"

#include "Database.hpp"

#include <stdexcept>

namespace persistence {
namespace {
AchievementUnlock readUnlock(Statement& statement)
{
    return {statement.integer(0), statement.text(1), statement.integer(2)};
}
} // namespace

bool AchievementRepository::insert(std::int64_t profileId, const std::string& achievementKey,
                                   std::int64_t unlockedAt)
{
    if (profileId <= 0 || achievementKey.empty() || unlockedAt < 0) {
        throw std::invalid_argument("Achievement unlock data is invalid");
    }
    auto statement = database_.prepare(
        "INSERT OR IGNORE INTO achievement_unlocks(profile_id, achievement_key, unlocked_at) "
        "VALUES(?1, ?2, ?3);");
    statement.bind(1, profileId);
    statement.bind(2, achievementKey);
    statement.bind(3, unlockedAt);
    statement.step();
    auto changed = database_.prepare("SELECT changes();");
    return changed.step() && changed.integer(0) == 1;
}

std::vector<AchievementUnlock> AchievementRepository::listByProfile(std::int64_t profileId) const
{
    auto statement = database_.prepare(
        "SELECT profile_id, achievement_key, unlocked_at FROM achievement_unlocks "
        "WHERE profile_id=?1 ORDER BY unlocked_at DESC, achievement_key ASC;");
    statement.bind(1, profileId);
    std::vector<AchievementUnlock> result;
    while (statement.step()) result.push_back(readUnlock(statement));
    return result;
}

std::vector<AchievementUnlock> AchievementRepository::listAll() const
{
    auto statement = database_.prepare(
        "SELECT profile_id, achievement_key, unlocked_at FROM achievement_unlocks "
        "ORDER BY profile_id ASC, unlocked_at DESC, achievement_key ASC;");
    std::vector<AchievementUnlock> result;
    while (statement.step()) result.push_back(readUnlock(statement));
    return result;
}

} // namespace persistence
