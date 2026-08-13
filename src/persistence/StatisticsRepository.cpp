#include "StatisticsRepository.hpp"

#include "Database.hpp"

#include <algorithm>
#include <cstdlib>

namespace persistence {
namespace {
void applyStreaks(Database& database, std::int64_t profileId, const char* game, OverallStatistics& stats)
{
    auto statement = database.prepare(game == nullptr
        ? "SELECT result FROM matches WHERE profile_id=?1 ORDER BY completed_at ASC,id ASC;"
        : "SELECT result FROM matches WHERE profile_id=?1 AND game_key=?2 ORDER BY completed_at ASC,id ASC;");
    statement.bind(1, profileId); if (game != nullptr) statement.bind(2, std::string(game));
    std::int64_t run = 0;
    while (statement.step()) {
        if (statement.text(0) == "win") { ++run; stats.bestWinStreak=std::max(stats.bestWinStreak,run); }
        else run=0;
    }
    stats.currentWinStreak=run;
}
void finish(OverallStatistics& stats)
{ stats.winRate=stats.matches == 0 ? 0.0 : static_cast<double>(stats.wins)/static_cast<double>(stats.matches); }
} // namespace

OverallStatistics StatisticsRepository::overall(std::int64_t profileId) const
{
    auto statement=database_.prepare("SELECT COUNT(*),COALESCE(SUM(result='win'),0),COALESCE(SUM(result='loss'),0),"
        "COALESCE(SUM(result='draw'),0),COALESCE(SUM(duration_ms),0),MAX(completed_at) FROM matches WHERE profile_id=?1;");
    statement.bind(1,profileId); OverallStatistics stats;
    if(statement.step()) { stats.matches=statement.integer(0); stats.wins=statement.integer(1); stats.losses=statement.integer(2);
        stats.draws=statement.integer(3); stats.totalDurationMs=statement.integer(4); if(!statement.isNull(5)) stats.lastPlayedAt=statement.integer(5); }
    applyStreaks(database_,profileId,nullptr,stats); finish(stats); return stats;
}

GameStatistics StatisticsRepository::forGame(std::int64_t profileId, GameKey game) const
{
    auto statement=database_.prepare("SELECT COUNT(*),COALESCE(SUM(result='win'),0),COALESCE(SUM(result='loss'),0),"
        "COALESCE(SUM(result='draw'),0),COALESCE(SUM(duration_ms),0),MAX(completed_at),"
        "COALESCE(SUM(profile_score),0),COALESCE(SUM(opponent_score),0),"
        "COALESCE(MAX(profile_score-opponent_score),0),COALESCE(SUM(profile_side_or_mark='X'),0),"
        "COALESCE(SUM(profile_side_or_mark='O'),0),COALESCE(SUM(match_format='single'),0),"
        "COALESCE(SUM(match_format='best_of_3'),0),COALESCE(SUM(match_format='best_of_5'),0) "
        "FROM matches WHERE profile_id=?1 AND game_key=?2;");
    statement.bind(1,profileId); statement.bind(2,toStorage(game)); GameStatistics stats;
    if(statement.step()) { stats.matches=statement.integer(0); stats.wins=statement.integer(1); stats.losses=statement.integer(2);
        stats.draws=statement.integer(3); stats.totalDurationMs=statement.integer(4); if(!statement.isNull(5)) stats.lastPlayedAt=statement.integer(5);
        stats.pointsScored=statement.integer(6); stats.pointsConceded=statement.integer(7); stats.bestFinalMargin=statement.integer(8);
        stats.ticTacToeAsX=statement.integer(9); stats.ticTacToeAsO=statement.integer(10); stats.singleMatches=statement.integer(11);
        stats.bestOfThreeMatches=statement.integer(12); stats.bestOfFiveMatches=statement.integer(13); }
    applyStreaks(database_,profileId,toStorage(game),stats); finish(stats); return stats;
}
} // namespace persistence
