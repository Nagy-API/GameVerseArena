#pragma once

#include "MatchTypes.hpp"
#include "AchievementTypes.hpp"

#include <cstdint>
#include <map>

namespace persistence {
class Database;
class StatisticsRepository {
public:
    explicit StatisticsRepository(Database& database) : database_(database) {}
    OverallStatistics overall(std::int64_t profileId) const;
    GameStatistics forGame(std::int64_t profileId, GameKey game) const;
    // Totals for every game, in allGameKeys() order, including games never played (all zero).
    std::vector<GameSummary> perGame(std::int64_t profileId) const;
    achievements::AchievementSnapshot achievementSnapshot(std::int64_t profileId) const;
    std::map<std::int64_t, achievements::AchievementSnapshot> achievementSnapshotsForAllProfiles() const;
private:
    Database& database_;
};
} // namespace persistence
