#pragma once

#include "AchievementRepository.hpp"
#include "AchievementTypes.hpp"
#include "StatisticsRepository.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace persistence {

class Database;

class AchievementService {
public:
    using UtcNow = std::function<std::int64_t()>;

    explicit AchievementService(Database& database, UtcNow utcNow = {});

    std::vector<achievements::AchievementDefinition> evaluateAndUnlock(std::int64_t profileId);
    std::size_t backfillAll();
    std::vector<achievements::AchievementStatus> statuses(std::int64_t profileId) const;
    std::size_t unlockedCount(std::int64_t profileId) const;

private:
    std::vector<achievements::AchievementDefinition> unlockSatisfied(
        std::int64_t profileId, const achievements::AchievementSnapshot& snapshot,
        std::int64_t recognitionTime, std::unordered_set<std::string>& existing);

    AchievementRepository repository_;
    Database& database_;
    StatisticsRepository statistics_;
    UtcNow utcNow_;
};

} // namespace persistence
