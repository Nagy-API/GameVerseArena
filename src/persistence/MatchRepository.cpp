#include "MatchRepository.hpp"

#include "Database.hpp"

#include <algorithm>

namespace persistence {
namespace {
void bindOptional(Statement& statement, int index, const std::optional<std::string>& value)
{ if (value) statement.bind(index, *value); else statement.bindNull(index); }
void bindOptional(Statement& statement, int index, const std::optional<int>& value)
{ if (value) statement.bind(index, static_cast<std::int64_t>(*value)); else statement.bindNull(index); }
} // namespace

std::int64_t MatchRepository::insert(const CompletedMatch& value)
{
    auto statement = database_.prepare(
        "INSERT INTO matches(profile_id, game_key, mode_key, opponent_name, profile_display_name, "
        "profile_side_or_mark, opponent_side_or_mark, result, profile_score, opponent_score, draw_value, "
        "difficulty_key, match_format, duration_ms, started_at, completed_at) "
        "VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14,?15,?16);");
    statement.bind(1, value.profileId); statement.bind(2, toStorage(value.game));
    statement.bind(3, toStorage(value.mode)); statement.bind(4, value.opponentName);
    statement.bind(5, value.profileDisplayName); bindOptional(statement, 6, value.profileSideOrMark);
    bindOptional(statement, 7, value.opponentSideOrMark); statement.bind(8, toStorage(value.result));
    bindOptional(statement, 9, value.profileScore); bindOptional(statement, 10, value.opponentScore);
    bindOptional(statement, 11, value.drawValue); statement.bind(12, toStorage(value.difficulty));
    statement.bind(13, value.matchFormat); statement.bind(14, value.durationMs);
    statement.bind(15, value.startedAt); statement.bind(16, value.completedAt); statement.step();
    return database_.lastInsertId();
}

std::vector<CompletedMatch> MatchRepository::recent(std::int64_t profileId, const MatchFilter& filter,
                                                     std::size_t limit, std::size_t offset) const
{
    constexpr const char* columns =
        "SELECT id,profile_id,game_key,mode_key,opponent_name,profile_display_name,profile_side_or_mark,"
        "opponent_side_or_mark,result,profile_score,opponent_score,draw_value,difficulty_key,match_format,"
        "duration_ms,started_at,completed_at FROM matches WHERE profile_id=?1 ";
    std::string sql = columns;
    int next = 2;
    if (filter.game) sql += "AND game_key=?" + std::to_string(next++) + " ";
    if (filter.result) sql += "AND result=?" + std::to_string(next++) + " ";
    const int limitIndex = next++;
    const int offsetIndex = next++;
    sql += "ORDER BY completed_at DESC,id DESC LIMIT ?" + std::to_string(limitIndex) +
           " OFFSET ?" + std::to_string(offsetIndex) + ";";
    auto statement = database_.prepare(sql.c_str());
    statement.bind(1, profileId);
    int bind = 2;
    if (filter.game) statement.bind(bind++, toStorage(*filter.game));
    if (filter.result) statement.bind(bind++, toStorage(*filter.result));
    statement.bind(bind++, static_cast<std::int64_t>(std::min<std::size_t>(limit, 100)));
    statement.bind(bind, static_cast<std::int64_t>(offset));
    std::vector<CompletedMatch> result;
    while (statement.step()) result.push_back(read(statement));
    return result;
}

std::int64_t MatchRepository::count(std::int64_t profileId, const MatchFilter& filter) const
{
    std::string sql = "SELECT COUNT(*) FROM matches WHERE profile_id=?1 ";
    int next = 2;
    if (filter.game) sql += "AND game_key=?" + std::to_string(next++) + " ";
    if (filter.result) sql += "AND result=?" + std::to_string(next) + " ";
    sql += ";";
    auto statement = database_.prepare(sql.c_str());
    statement.bind(1, profileId);
    int bind = 2;
    if (filter.game) statement.bind(bind++, toStorage(*filter.game));
    if (filter.result) statement.bind(bind, toStorage(*filter.result));
    return statement.step() ? statement.integer(0) : 0;
}

CompletedMatch MatchRepository::read(Statement& s)
{
    CompletedMatch value;
    value.id=s.integer(0); value.profileId=s.integer(1); value.game=gameKeyFromStorage(s.text(2));
    value.mode=matchModeFromStorage(s.text(3)); value.opponentName=s.text(4); value.profileDisplayName=s.text(5);
    if (!s.isNull(6)) value.profileSideOrMark = s.text(6);
    if (!s.isNull(7)) value.opponentSideOrMark = s.text(7);
    value.result = matchResultFromStorage(s.text(8));
    if (!s.isNull(9)) value.profileScore = static_cast<int>(s.integer(9));
    if (!s.isNull(10)) value.opponentScore = static_cast<int>(s.integer(10));
    if (!s.isNull(11)) value.drawValue = static_cast<int>(s.integer(11));
    value.difficulty = difficultyFromStorage(s.text(12));
    value.matchFormat=s.text(13); value.durationMs=s.integer(14); value.startedAt=s.integer(15); value.completedAt=s.integer(16);
    return value;
}
} // namespace persistence
