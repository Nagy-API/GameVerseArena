#pragma once

#include "MatchTypes.hpp"

#include <cstdint>

namespace persistence {
class Database;
class StatisticsRepository {
public:
    explicit StatisticsRepository(Database& database) : database_(database) {}
    OverallStatistics overall(std::int64_t profileId) const;
    GameStatistics forGame(std::int64_t profileId, GameKey game) const;
private:
    Database& database_;
};
} // namespace persistence
