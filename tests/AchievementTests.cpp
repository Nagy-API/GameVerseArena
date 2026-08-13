#include "AchievementCatalogue.hpp"
#include "AchievementEvaluator.hpp"
#include "AchievementNotificationQueue.hpp"
#include "AchievementRepository.hpp"
#include "AchievementService.hpp"
#include "AchievementToastText.hpp"
#include "Database.hpp"
#include "MatchService.hpp"
#include "MatchRepository.hpp"
#include "ProfileService.hpp"
#include "StatisticsRepository.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
int failures = 0;
int checks = 0;

void check(bool condition, const std::string& message)
{
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

struct TemporaryDirectory {
    TemporaryDirectory()
    {
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() /
               ("GameVerseArena-achievement-tests-" + std::to_string(stamp));
        std::filesystem::create_directories(path);
    }
    ~TemporaryDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
    std::filesystem::path path;
};

persistence::CompletedMatch match(std::int64_t profileId, persistence::GameKey game,
                                  persistence::MatchResult result, std::int64_t completedAt,
                                  std::string mark = "X", int profileScore = -1, int opponentScore = -1)
{
    persistence::CompletedMatch value;
    value.profileId = profileId;
    value.game = game;
    value.mode = persistence::MatchMode::HumanVsHuman;
    value.opponentName = "Guest";
    value.profileDisplayName = "Owner";
    value.profileSideOrMark = game == persistence::GameKey::ClassicTicTacToe ? mark : "Left";
    value.opponentSideOrMark = game == persistence::GameKey::ClassicTicTacToe
        ? (mark == "X" ? "O" : "X") : "Right";
    value.result = result;
    if (game == persistence::GameKey::ClassicTicTacToe) {
        value.profileScore = result == persistence::MatchResult::Win ? 1 : 0;
        value.opponentScore = result == persistence::MatchResult::Loss ? 1 : 0;
        value.drawValue = result == persistence::MatchResult::Draw ? 1 : 0;
        value.matchFormat = "single";
    } else {
        value.profileScore = profileScore >= 0 ? profileScore : result == persistence::MatchResult::Win ? 5 : 2;
        value.opponentScore = opponentScore >= 0 ? opponentScore : result == persistence::MatchResult::Loss ? 5 : 1;
        value.matchFormat = "first_to_5";
    }
    value.difficulty = persistence::DifficultyKey::None;
    value.durationMs = 1000;
    value.startedAt = completedAt - 100;
    value.completedAt = completedAt;
    return value;
}

const achievements::AchievementDefinition& definition(const std::string& key)
{
    const auto& all = achievements::AchievementCatalogue::all();
    const auto found = std::find_if(all.begin(), all.end(), [&](const auto& item) { return item.key == key; });
    if (found == all.end()) throw std::runtime_error("Missing catalogue entry " + key);
    return *found;
}

const achievements::AchievementEvaluation& evaluation(
    const std::vector<achievements::AchievementEvaluation>& all, const std::string& key)
{
    const auto found = std::find_if(all.begin(), all.end(), [&](const auto& item) { return item.key == key; });
    if (found == all.end()) throw std::runtime_error("Missing evaluation " + key);
    return *found;
}

void testCatalogue()
{
    const auto& all = achievements::AchievementCatalogue::all();
    check(all.size() == 12, "catalogue contains exactly 12 achievements");
    const std::vector<std::string> expected{
        "first_victory", "arena_regular", "dedicated_player", "on_a_roll", "unstoppable",
        "versatile_player", "xo_first_win", "x_marks_the_spot", "o_turnaround", "pong_first_win",
        "clean_sweep", "clutch_finish"};
    for (std::size_t index = 0; index < expected.size() && index < all.size(); ++index) {
        check(all[index].key == expected[index], "catalogue order/key matches specification: " + expected[index]);
        check(!all[index].hidden, "achievement is visible: " + expected[index]);
        check(!all[index].title.empty() && !all[index].description.empty(),
              "achievement has title and description: " + expected[index]);
    }
    check(definition("first_victory").title == "First Victory" &&
          definition("first_victory").description == "Win your first match.",
          "First Victory copy is exact");
    check(definition("arena_regular").progressTarget == 10 &&
          definition("dedicated_player").progressTarget == 25,
          "match-completion progress targets are exact");
    check(definition("versatile_player").category == achievements::AchievementCategory::General &&
          definition("versatile_player").progressTarget == 2,
          "Versatile Player is General with a two-game target");
    check(definition("xo_first_win").category == achievements::AchievementCategory::TicTacToe &&
          definition("pong_first_win").category == achievements::AchievementCategory::PingPong,
          "game-specific categories are correct");
    check(!definition("clean_sweep").numericProgress && !definition("clutch_finish").numericProgress,
          "score-pattern achievements do not fabricate numeric progress");
}

void testEvaluatorAndProgress()
{
    achievements::AchievementSnapshot snapshot;
    auto result = achievements::AchievementEvaluator::evaluate(snapshot);
    for (const auto& item : result) check(!item.satisfied, "zero history leaves " + item.key + " locked");
    check(evaluation(result, "first_victory").progress->current == 0,
          "zero history reports 0/1 wins");
    check(!evaluation(result, "clean_sweep").progress.has_value(),
          "Clean Sweep has no fake fractional progress");

    snapshot.totalMatches = 9;
    snapshot.totalWins = 0;
    snapshot.bestWinStreak = 2;
    snapshot.classicWins = 1;
    result = achievements::AchievementEvaluator::evaluate(snapshot);
    check(!evaluation(result, "arena_regular").satisfied &&
          evaluation(result, "arena_regular").progress->current == 9,
          "Arena Regular is false at 9 and reports 9/10");
    check(!evaluation(result, "dedicated_player").satisfied,
          "Dedicated Player is false before 25 matches");
    check(!evaluation(result, "on_a_roll").satisfied && !evaluation(result, "unstoppable").satisfied,
          "streak achievements are false below their thresholds");
    check(!evaluation(result, "versatile_player").satisfied &&
          evaluation(result, "versatile_player").progress->current == 1,
          "Versatile Player requires wins in both games and reports 1/2");

    snapshot.totalMatches = 25;
    snapshot.totalWins = 8;
    snapshot.bestWinStreak = 5;
    snapshot.pingPongWins = 1;
    snapshot.classicXWins = 1;
    snapshot.classicOWins = 1;
    snapshot.hasPingPongFiveZeroWin = true;
    snapshot.hasPingPongFiveFourWin = true;
    result = achievements::AchievementEvaluator::evaluate(snapshot);
    for (const auto& item : result) check(item.satisfied, "threshold snapshot unlocks " + item.key);
    check(evaluation(result, "first_victory").progress->current == 1 &&
          evaluation(result, "arena_regular").progress->current == 10 &&
          evaluation(result, "dedicated_player").progress->current == 25 &&
          evaluation(result, "on_a_roll").progress->current == 3 &&
          evaluation(result, "unstoppable").progress->current == 5 &&
          evaluation(result, "versatile_player").progress->current == 2 &&
          evaluation(result, "xo_first_win").progress->current == 1 &&
          evaluation(result, "x_marks_the_spot").progress->current == 1 &&
          evaluation(result, "o_turnaround").progress->current == 1 &&
          evaluation(result, "pong_first_win").progress->current == 1,
          "all numeric progress values clamp to their targets");
    check(!evaluation(result, "clean_sweep").progress && !evaluation(result, "clutch_finish").progress,
          "both score-pattern achievements omit numeric progress");

    achievements::AchievementSnapshot boundary;
    boundary.totalWins = 1;
    boundary.totalMatches = 10;
    boundary.bestWinStreak = 3;
    boundary.classicWins = 1;
    boundary.pingPongWins = 1;
    boundary.classicXWins = 1;
    boundary.classicOWins = 0;
    boundary.hasPingPongFiveZeroWin = true;
    boundary.hasPingPongFiveFourWin = false;
    result = achievements::AchievementEvaluator::evaluate(boundary);
    check(evaluation(result, "first_victory").satisfied && evaluation(result, "arena_regular").satisfied &&
          evaluation(result, "on_a_roll").satisfied && evaluation(result, "versatile_player").satisfied,
          "win, match, streak, and cross-game achievements unlock at exact thresholds");
    check(evaluation(result, "x_marks_the_spot").satisfied && !evaluation(result, "o_turnaround").satisfied,
          "X win does not unlock O achievement");
    boundary.classicXWins = 0;
    boundary.classicOWins = 1;
    result = achievements::AchievementEvaluator::evaluate(boundary);
    check(!evaluation(result, "x_marks_the_spot").satisfied && evaluation(result, "o_turnaround").satisfied,
          "O win does not unlock X achievement");
}

void testMigrationAndRepository(const std::filesystem::path& directory)
{
    const auto path = directory / "migration-v2.db";
    std::int64_t profileId = 0;
    {
        persistence::Database database(path);
        persistence::ProfileService profiles(database);
        profiles.bootstrap();
        const auto profile = profiles.createProfile("Migration Survivor");
        profiles.setActiveProfile(profile.id);
        profileId = profile.id;
        persistence::MatchService matches(database);
        matches.recordCompleted(match(profile.id, persistence::GameKey::ClassicTicTacToe,
                                      persistence::MatchResult::Win, 100));
        database.execute("DROP TABLE IF EXISTS achievement_unlocks;");
        database.execute("PRAGMA user_version = 2;");
    }
    {
        persistence::Database migrated(path);
        check(migrated.userVersion() == 3, "v2 database migrates transactionally to v3");
        persistence::ProfileService profiles(migrated);
        check(profiles.activeProfile().has_value() && profiles.activeProfile()->id == profileId,
              "active profile survives v2-to-v3 migration");
        auto profileCount = migrated.prepare("SELECT COUNT(*) FROM profiles WHERE id=?1;");
        profileCount.bind(1, profileId);
        check(profileCount.step() && profileCount.integer(0) == 1, "profiles survive v2-to-v3 migration");
        auto matchCount = migrated.prepare("SELECT COUNT(*) FROM matches WHERE profile_id=?1;");
        matchCount.bind(1, profileId);
        check(matchCount.step() && matchCount.integer(0) == 1, "matches survive v2-to-v3 migration");
        auto table = migrated.prepare(
            "SELECT sql FROM sqlite_master WHERE type='table' AND name='achievement_unlocks';");
        check(table.step() && table.text(0).find("PRIMARY KEY (profile_id, achievement_key)") != std::string::npos,
              "v3 table has the required composite primary key");
        auto foreignKey = migrated.prepare("PRAGMA foreign_key_list(achievement_unlocks);");
        check(foreignKey.step() && foreignKey.text(2) == "profiles" && foreignKey.text(3) == "profile_id" &&
              foreignKey.text(4) == "id" && foreignKey.text(6) == "CASCADE",
              "v3 unlock table references profiles with ON DELETE CASCADE");
    }
    {
        persistence::Database reopened(path);
        check(reopened.userVersion() == 3, "repeated v3 open is idempotent");
    }

    const auto futurePath = directory / "future-v4.db";
    {
        persistence::Database database(futurePath);
        database.execute("PRAGMA user_version = 4;");
    }
    try {
        persistence::Database unsupported(futurePath);
        check(false, "unsupported future v4 must fail safely");
    } catch (const std::runtime_error& error) {
        check(std::string(error.what()).find("version 4") != std::string::npos,
              "future-version rejection identifies v4");
    }

    persistence::Database database(directory / "repository.db");
    persistence::ProfileService profiles(database);
    profiles.bootstrap();
    const auto first = profiles.activeProfile().value();
    const auto second = profiles.createProfile("Second");
    persistence::AchievementRepository unlocks(database);
    check(unlocks.insert(first.id, "first_victory", 111), "first unlock insert reports new row");
    check(!unlocks.insert(first.id, "first_victory", 222), "duplicate unlock insert is ignored");
    check(unlocks.insert(first.id, "arena_regular", 333), "second key inserts for same profile");
    check(unlocks.insert(second.id, "first_victory", 444), "same key inserts for another profile");
    const auto firstRows = unlocks.listByProfile(first.id);
    check(firstRows.size() == 2, "list by profile returns only that profile's unlocks");
    const auto firstVictory = std::find_if(firstRows.begin(), firstRows.end(),
        [](const auto& item) { return item.achievementKey == "first_victory"; });
    check(firstVictory != firstRows.end() && firstVictory->unlockedAt == 111,
          "duplicate insert preserves first unlock timestamp");
    check(unlocks.listByProfile(second.id).size() == 1,
          "achievement unlocks are profile-isolated");
    persistence::MatchService matchService(database);
    matchService.recordCompleted(match(first.id, persistence::GameKey::ClassicTicTacToe,
                                       persistence::MatchResult::Win, 500));
    check(profiles.deleteProfile(first.id), "profile with unlocks can be deleted");
    check(unlocks.listByProfile(first.id).empty(), "profile deletion cascades achievement unlocks");
    persistence::MatchRepository history(database);
    check(history.count(first.id, {}) == 0, "same profile deletion also cascades match history");
}

void testSnapshotQueries(const std::filesystem::path& directory)
{
    persistence::Database database(directory / "snapshots.db");
    persistence::ProfileService profiles(database);
    profiles.bootstrap();
    persistence::MatchService matches(database);
    persistence::StatisticsRepository statistics(database);

    const auto cleanWinner = profiles.activeProfile().value();
    matches.recordCompleted(match(cleanWinner.id, persistence::GameKey::PingPong,
                                  persistence::MatchResult::Win, 100, "", 5, 0));
    auto snapshot = statistics.achievementSnapshot(cleanWinner.id);
    check(snapshot.hasPingPongFiveZeroWin && !snapshot.hasPingPongFiveFourWin,
          "5-0 win unlock condition is recognized");

    const auto cleanLoser = profiles.createProfile("Clean Loser");
    matches.recordCompleted(match(cleanLoser.id, persistence::GameKey::PingPong,
                                  persistence::MatchResult::Loss, 200, "", 0, 5));
    snapshot = statistics.achievementSnapshot(cleanLoser.id);
    check(!snapshot.hasPingPongFiveZeroWin, "0-5 loss does not satisfy Clean Sweep");

    const auto clutchWinner = profiles.createProfile("Clutch Winner");
    matches.recordCompleted(match(clutchWinner.id, persistence::GameKey::PingPong,
                                  persistence::MatchResult::Win, 300, "", 5, 4));
    snapshot = statistics.achievementSnapshot(clutchWinner.id);
    check(snapshot.hasPingPongFiveFourWin, "5-4 win unlock condition is recognized");

    const auto clutchLoser = profiles.createProfile("Clutch Loser");
    matches.recordCompleted(match(clutchLoser.id, persistence::GameKey::PingPong,
                                  persistence::MatchResult::Loss, 400, "", 4, 5));
    snapshot = statistics.achievementSnapshot(clutchLoser.id);
    check(!snapshot.hasPingPongFiveFourWin, "4-5 loss does not satisfy Clutch Finish");

    const auto streak = profiles.createProfile("Streak");
    matches.recordCompleted(match(streak.id, persistence::GameKey::ClassicTicTacToe,
                                  persistence::MatchResult::Win, 500));
    matches.recordCompleted(match(streak.id, persistence::GameKey::ClassicTicTacToe,
                                  persistence::MatchResult::Win, 600));
    matches.recordCompleted(match(streak.id, persistence::GameKey::ClassicTicTacToe,
                                  persistence::MatchResult::Draw, 700));
    matches.recordCompleted(match(streak.id, persistence::GameKey::ClassicTicTacToe,
                                  persistence::MatchResult::Win, 800, "O"));
    snapshot = statistics.achievementSnapshot(streak.id);
    check(snapshot.bestWinStreak == 2, "draw breaks achievement win streak using Task 08 semantics");
    check(snapshot.classicXWins == 2 && snapshot.classicOWins == 1,
          "focused mark-win query counts only wins for each mark");

    const auto batch = statistics.achievementSnapshotsForAllProfiles();
    check(batch.size() == profiles.listProfiles().size() && batch.at(streak.id).bestWinStreak == 2,
          "batched snapshots include every profile and reuse one history scan");
}

void testEvaluationIdempotencyAndBackfill(const std::filesystem::path& directory)
{
    persistence::Database database(directory / "service.db");
    persistence::ProfileService profiles(database);
    profiles.bootstrap();
    persistence::MatchService matches(database);
    persistence::AchievementRepository repository(database);
    const auto profile = profiles.activeProfile().value();
    matches.recordCompleted(match(profile.id, persistence::GameKey::ClassicTicTacToe,
                                  persistence::MatchResult::Win, 100, "X"));

    persistence::AchievementService service(database, [] { return 9000; });
    const auto first = service.evaluateAndUnlock(profile.id);
    check(first.size() == 3, "first X victory recognizes First Victory, XO Winner, and X Marks the Spot");
    check(service.evaluateAndUnlock(profile.id).empty(),
          "already unlocked conditions return no new toast items");
    check(repository.listByProfile(profile.id).size() == 3,
          "repeated evaluation persists each achievement exactly once");

    const auto legacy = profiles.createProfile("Legacy Winner");
    matches.recordCompleted(match(legacy.id, persistence::GameKey::PingPong,
                                  persistence::MatchResult::Win, 200, "", 5, 0));
    check(service.backfillAll() == 3,
          "startup backfill recognizes missing First Victory, Pong Winner, and Clean Sweep");
    check(service.backfillAll() == 0, "repeated startup backfill is idempotent");
    const auto rows = repository.listByProfile(legacy.id);
    check(rows.size() == 3 && std::all_of(rows.begin(), rows.end(),
        [](const auto& item) { return item.unlockedAt == 9000; }),
        "backfill stores current recognition time without fabricating historical time");

    const auto statuses = service.statuses(legacy.id);
    check(statuses.size() == 12 &&
          std::count_if(statuses.begin(), statuses.end(), [](const auto& item) { return item.unlockedAt.has_value(); }) == 3,
          "status view combines all catalogue entries with durable unlock facts");
}

void testUnlockBatchIsAtomic(const std::filesystem::path& directory)
{
    persistence::Database database(directory / "atomic-unlocks.db");
    persistence::ProfileService profiles(database);
    profiles.bootstrap();
    const auto profile = profiles.activeProfile().value();
    persistence::MatchService matches(database);
    matches.recordCompleted(match(profile.id, persistence::GameKey::ClassicTicTacToe,
                                  persistence::MatchResult::Win, 100, "X"));
    database.execute(
        "CREATE TRIGGER reject_x_achievement BEFORE INSERT ON achievement_unlocks "
        "WHEN NEW.achievement_key='x_marks_the_spot' BEGIN "
        "SELECT RAISE(ABORT, 'forced unlock failure'); END;");
    persistence::AchievementService achievements(database, [] { return 9000; });
    try {
        achievements.evaluateAndUnlock(profile.id);
        check(false, "forced unlock failure must propagate");
    } catch (const std::runtime_error&) {
        check(true, "forced unlock failure propagates to the caller");
    }
    persistence::AchievementRepository repository(database);
    check(repository.listByProfile(profile.id).empty(),
          "failed multi-achievement recognition rolls back the entire batch");
}

void testFailedMatchDoesNotUnlock(const std::filesystem::path& directory)
{
    persistence::Database database(directory / "failed-match.db");
    persistence::ProfileService profiles(database);
    profiles.bootstrap();
    const auto profile = profiles.activeProfile().value();
    persistence::MatchService matches(database);
    auto incomplete = match(profile.id, persistence::GameKey::PingPong,
                            persistence::MatchResult::Win, 100, "", 4, 0);
    try {
        matches.recordCompleted(incomplete);
        check(false, "unfinished match persistence must fail");
    } catch (const std::invalid_argument&) {
        check(true, "unfinished match persistence fails before achievement evaluation");
    }
    persistence::AchievementService achievements(database, [] { return 9000; });
    check(achievements.evaluateAndUnlock(profile.id).empty(),
          "unpersisted match cannot satisfy or unlock an achievement");
    persistence::AchievementRepository unlocks(database);
    check(unlocks.listByProfile(profile.id).empty(),
          "failed match persistence leaves no unlock facts");
}

void testNotificationQueue()
{
    achievements::AchievementNotificationQueue queue(3.5f);
    queue.enqueue({definition("first_victory"), definition("xo_first_win")});
    check(queue.current() && queue.current()->key == "first_victory",
          "toast queue starts with first unlock");
    queue.update(3.4f);
    check(queue.current() && queue.current()->key == "first_victory",
          "toast remains visible before duration elapses");
    queue.update(0.2f);
    check(queue.current() && queue.current()->key == "xo_first_win",
          "multiple unlocks preserve sequential queue order");
    queue.dismiss();
    check(!queue.current(), "early dismissal advances and each toast displays once");

    queue.enqueue({definition("first_victory"), definition("first_victory")});
    check(queue.pendingCount() == 1, "queue suppresses duplicate pending toast keys");

    const auto wrapped = wrapAchievementToastDescription(definition("versatile_player").description, 42);
    check(wrapped.find('\n') != std::string::npos,
          "long achievement toast descriptions wrap to multiple lines");
    std::istringstream lines(wrapped);
    std::string line;
    bool bounded = true;
    while (std::getline(lines, line)) bounded = bounded && line.size() <= 42;
    check(bounded, "each wrapped toast-description line stays within the panel limit");
}
} // namespace

int main()
{
    TemporaryDirectory temporary;
    testCatalogue();
    testEvaluatorAndProgress();
    testMigrationAndRepository(temporary.path);
    testSnapshotQueries(temporary.path);
    testEvaluationIdempotencyAndBackfill(temporary.path);
    testUnlockBatchIsAtomic(temporary.path);
    testFailedMatchDoesNotUnlock(temporary.path);
    testNotificationQueue();

    if (failures == 0) {
        std::cout << "Achievement tests passed: " << checks << " checks\n";
        return 0;
    }
    std::cerr << failures << " of " << checks << " achievement checks failed\n";
    return 1;
}
