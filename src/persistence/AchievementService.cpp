#include "AchievementService.hpp"

#include "AchievementCatalogue.hpp"
#include "AchievementEvaluator.hpp"
#include "Database.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <utility>

namespace persistence {
namespace {
std::int64_t currentUtcMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
} // namespace

AchievementService::AchievementService(Database& database, UtcNow utcNow)
    : repository_(database), database_(database), statistics_(database),
      utcNow_(utcNow ? std::move(utcNow) : UtcNow{currentUtcMilliseconds})
{
}

std::vector<achievements::AchievementDefinition>
AchievementService::evaluateAndUnlock(std::int64_t profileId)
{
    std::unordered_set<std::string> existing;
    for (const auto& unlock : repository_.listByProfile(profileId)) {
        existing.insert(unlock.achievementKey);
    }
    return unlockSatisfied(profileId, statistics_.achievementSnapshot(profileId), utcNow_(), existing);
}

std::size_t AchievementService::backfillAll()
{
    std::unordered_map<std::int64_t, std::unordered_set<std::string>> existing;
    for (const auto& unlock : repository_.listAll()) {
        existing[unlock.profileId].insert(unlock.achievementKey);
    }
    const auto recognitionTime = utcNow_();
    std::size_t inserted = 0;
    for (const auto& entry : statistics_.achievementSnapshotsForAllProfiles()) {
        inserted += unlockSatisfied(entry.first, entry.second, recognitionTime, existing[entry.first]).size();
    }
    return inserted;
}

std::vector<achievements::AchievementStatus> AchievementService::statuses(std::int64_t profileId) const
{
    std::unordered_map<std::string, std::int64_t> unlocked;
    for (const auto& unlock : repository_.listByProfile(profileId)) {
        unlocked.emplace(unlock.achievementKey, unlock.unlockedAt);
    }
    const auto evaluations = achievements::AchievementEvaluator::evaluate(
        statistics_.achievementSnapshot(profileId));
    const auto& catalogue = achievements::AchievementCatalogue::all();
    std::vector<achievements::AchievementStatus> result;
    result.reserve(catalogue.size());
    for (std::size_t index = 0; index < catalogue.size(); ++index) {
        std::optional<std::int64_t> unlockedAt;
        const auto found = unlocked.find(catalogue[index].key);
        if (found != unlocked.end()) unlockedAt = found->second;
        result.push_back({catalogue[index], unlockedAt, evaluations[index].progress});
    }
    return result;
}

std::size_t AchievementService::unlockedCount(std::int64_t profileId) const
{
    return repository_.listByProfile(profileId).size();
}

std::vector<achievements::AchievementDefinition> AchievementService::unlockSatisfied(
    std::int64_t profileId, const achievements::AchievementSnapshot& snapshot,
    std::int64_t recognitionTime, std::unordered_set<std::string>& existing)
{
    const auto evaluations = achievements::AchievementEvaluator::evaluate(snapshot);
    const auto& catalogue = achievements::AchievementCatalogue::all();
    std::vector<achievements::AchievementDefinition> unlocked;
    auto transaction = database_.transaction();
    for (std::size_t index = 0; index < evaluations.size(); ++index) {
        if (!evaluations[index].satisfied || existing.count(evaluations[index].key) != 0) continue;
        if (repository_.insert(profileId, evaluations[index].key, recognitionTime)) {
            existing.insert(evaluations[index].key);
            unlocked.push_back(catalogue[index]);
        }
    }
    transaction.commit();
    return unlocked;
}

} // namespace persistence
