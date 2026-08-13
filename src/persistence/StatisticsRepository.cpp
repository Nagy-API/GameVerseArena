#include "StatisticsRepository.hpp"

#include "Database.hpp"

#include <algorithm>
#include <cstdlib>
#include <map>

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

achievements::AchievementSnapshot StatisticsRepository::achievementSnapshot(std::int64_t profileId) const
{
    auto statement = database_.prepare(
        "SELECT COUNT(*),COALESCE(SUM(result='win'),0),"
        "COALESCE(SUM(game_key='classic_tic_tac_toe' AND result='win'),0),"
        "COALESCE(SUM(game_key='ping_pong' AND result='win'),0),"
        "COALESCE(SUM(game_key='classic_tic_tac_toe' AND result='win' AND profile_side_or_mark='X'),0),"
        "COALESCE(SUM(game_key='classic_tic_tac_toe' AND result='win' AND profile_side_or_mark='O'),0),"
        "COALESCE(MAX(game_key='ping_pong' AND result='win' AND profile_score=5 AND opponent_score=0),0),"
        "COALESCE(MAX(game_key='ping_pong' AND result='win' AND profile_score=5 AND opponent_score=4),0) "
        "FROM matches WHERE profile_id=?1;");
    statement.bind(1, profileId);
    achievements::AchievementSnapshot snapshot;
    if (statement.step()) {
        snapshot.totalMatches = statement.integer(0);
        snapshot.totalWins = statement.integer(1);
        snapshot.classicWins = statement.integer(2);
        snapshot.pingPongWins = statement.integer(3);
        snapshot.classicXWins = statement.integer(4);
        snapshot.classicOWins = statement.integer(5);
        snapshot.hasPingPongFiveZeroWin = statement.integer(6) != 0;
        snapshot.hasPingPongFiveFourWin = statement.integer(7) != 0;
    }
    auto streaks = database_.prepare(
        "SELECT result FROM matches WHERE profile_id=?1 ORDER BY completed_at ASC,id ASC;");
    streaks.bind(1, profileId);
    std::int64_t current = 0;
    while (streaks.step()) {
        if (streaks.text(0) == "win") {
            ++current;
            snapshot.bestWinStreak = std::max(snapshot.bestWinStreak, current);
        } else {
            current = 0;
        }
    }
    return snapshot;
}

std::map<std::int64_t, achievements::AchievementSnapshot>
StatisticsRepository::achievementSnapshotsForAllProfiles() const
{
    std::map<std::int64_t, achievements::AchievementSnapshot> snapshots;
    auto aggregate = database_.prepare(
        "SELECT p.id,COUNT(m.id),COALESCE(SUM(m.result='win'),0),"
        "COALESCE(SUM(m.game_key='classic_tic_tac_toe' AND m.result='win'),0),"
        "COALESCE(SUM(m.game_key='ping_pong' AND m.result='win'),0),"
        "COALESCE(SUM(m.game_key='classic_tic_tac_toe' AND m.result='win' AND m.profile_side_or_mark='X'),0),"
        "COALESCE(SUM(m.game_key='classic_tic_tac_toe' AND m.result='win' AND m.profile_side_or_mark='O'),0),"
        "COALESCE(MAX(m.game_key='ping_pong' AND m.result='win' AND m.profile_score=5 AND m.opponent_score=0),0),"
        "COALESCE(MAX(m.game_key='ping_pong' AND m.result='win' AND m.profile_score=5 AND m.opponent_score=4),0) "
        "FROM profiles p LEFT JOIN matches m ON m.profile_id=p.id GROUP BY p.id ORDER BY p.id ASC;");
    while (aggregate.step()) {
        auto& snapshot = snapshots[aggregate.integer(0)];
        snapshot.totalMatches = aggregate.integer(1);
        snapshot.totalWins = aggregate.integer(2);
        snapshot.classicWins = aggregate.integer(3);
        snapshot.pingPongWins = aggregate.integer(4);
        snapshot.classicXWins = aggregate.integer(5);
        snapshot.classicOWins = aggregate.integer(6);
        snapshot.hasPingPongFiveZeroWin = aggregate.integer(7) != 0;
        snapshot.hasPingPongFiveFourWin = aggregate.integer(8) != 0;
    }

    auto streaks = database_.prepare(
        "SELECT profile_id,result FROM matches ORDER BY profile_id ASC,completed_at ASC,id ASC;");
    std::int64_t priorProfile = -1;
    std::int64_t current = 0;
    while (streaks.step()) {
        const auto profileId = streaks.integer(0);
        if (profileId != priorProfile) {
            priorProfile = profileId;
            current = 0;
        }
        if (streaks.text(1) == "win") {
            ++current;
            snapshots[profileId].bestWinStreak = std::max(snapshots[profileId].bestWinStreak, current);
        } else {
            current = 0;
        }
    }
    return snapshots;
}
} // namespace persistence
