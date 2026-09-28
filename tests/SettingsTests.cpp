#include "Database.hpp"
#include "MatchService.hpp"
#include "MatchTypes.hpp"
#include "ProfileService.hpp"
#include "Schema.hpp"
#include "SettingsService.hpp"
#include "TestSchemaSupport.hpp"
#include "sqlite3.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct TemporaryDirectory {
    TemporaryDirectory()
    {
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() / ("GameVerseArena-settings-tests-" + std::to_string(stamp));
        std::filesystem::create_directories(path);
    }
    ~TemporaryDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
    std::filesystem::path path;
};

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

std::int64_t scalar(persistence::Database& database, const std::string& sql)
{
    auto statement = database.prepare(sql.c_str());
    return statement.step() ? statement.integer(0) : -1;
}

void setRaw(persistence::Database& database, const std::string& key, const std::string& value)
{
    auto statement = database.prepare(
        "INSERT INTO app_settings(key, value, updated_at) VALUES(?1, ?2, 1) "
        "ON CONFLICT(key) DO UPDATE SET value = excluded.value;");
    statement.bind(1, key);
    statement.bind(2, value);
    statement.step();
}

persistence::CompletedMatch classicWin(std::int64_t profileId, std::int64_t completedAt)
{
    persistence::CompletedMatch match;
    match.profileId = profileId;
    match.game = persistence::GameKey::ClassicTicTacToe;
    match.mode = persistence::MatchMode::HumanVsComputer;
    match.opponentName = "Computer";
    match.profileDisplayName = "Keeper";
    match.profileSideOrMark = "O";
    match.opponentSideOrMark = "X";
    match.result = persistence::MatchResult::Win;
    match.profileScore = 2;
    match.opponentScore = 1;
    match.drawValue = 1;
    match.difficulty = persistence::DifficultyKey::Hard;
    match.matchFormat = "best_of_3";
    match.durationMs = 4321;
    match.startedAt = completedAt / 2;
    match.completedAt = completedAt;
    return match;
}

persistence::CompletedMatch pongLoss(std::int64_t profileId, std::int64_t completedAt)
{
    persistence::CompletedMatch match;
    match.profileId = profileId;
    match.game = persistence::GameKey::PingPong;
    match.mode = persistence::MatchMode::HumanVsHuman;
    match.opponentName = "Rival";
    match.profileDisplayName = "Keeper";
    match.profileSideOrMark = "Left";
    match.opponentSideOrMark = "Right";
    match.result = persistence::MatchResult::Loss;
    match.profileScore = 3;
    match.opponentScore = 5;
    match.difficulty = persistence::DifficultyKey::None;
    match.matchFormat = "first_to_5";
    match.durationMs = 99000;
    match.startedAt = completedAt / 2;
    match.completedAt = completedAt;
    return match;
}

std::string matchRowSignature(persistence::Database& database)
{
    auto statement = database.prepare(
        "SELECT id,profile_id,game_key,mode_key,opponent_name,profile_display_name,"
        "COALESCE(profile_side_or_mark,'<null>'),COALESCE(opponent_side_or_mark,'<null>'),result,"
        "COALESCE(profile_score,-1),COALESCE(opponent_score,-1),COALESCE(draw_value,-1),difficulty_key,"
        "match_format,duration_ms,started_at,completed_at FROM matches ORDER BY id;");
    std::string signature;
    while (statement.step()) {
        for (int column = 0; column < 17; ++column) signature += statement.text(column) + "|";
        signature += "\n";
    }
    return signature;
}

void testDefaultsAndRoundTrip(const std::filesystem::path& directory)
{
    const auto path = directory / "roundtrip.db";
    {
        persistence::Database database(path);
        persistence::SettingsService service(database);
        const auto loaded = service.load();
        check(loaded.issues.empty(), "a fresh database loads settings without issues");
        const auto& settings = loaded.settings;
        check(settings.masterVolume == 80 && settings.uiVolume == 70 && settings.gameplayVolume == 80 &&
                  settings.achievementVolume == 85 && !settings.audioMuted && !settings.reducedMotion,
              "missing settings use the documented defaults 80/70/80/85, unmuted, full motion");
        check(scalar(database, "SELECT COUNT(*) FROM app_settings;") == 0,
              "loading defaults does not write rows");

        persistence::AppSettings custom;
        custom.masterVolume = 0;
        custom.uiVolume = 100;
        custom.gameplayVolume = 35;
        custom.achievementVolume = 1;
        custom.audioMuted = true;
        custom.reducedMotion = true;
        service.save(custom);
        check(scalar(database, "SELECT COUNT(*) FROM app_settings;") == 6, "saving writes one row per setting");
        check(service.load().settings == custom, "saved settings load back identically in the same session");
    }
    {
        persistence::Database reopened(path);
        persistence::SettingsService service(reopened);
        const auto loaded = service.load();
        check(loaded.issues.empty(), "reopened settings have no issues");
        check(loaded.settings.masterVolume == 0 && loaded.settings.uiVolume == 100 &&
                  loaded.settings.gameplayVolume == 35 && loaded.settings.achievementVolume == 1 &&
                  loaded.settings.audioMuted && loaded.settings.reducedMotion,
              "settings persist across database reopen, including 0 and 100 boundaries");

        const auto defaults = service.resetToDefaults();
        check(defaults == persistence::defaultSettings(), "reset returns the defaults");
    }
    {
        persistence::Database reopened(path);
        persistence::SettingsService service(reopened);
        check(service.load().settings == persistence::defaultSettings(), "reset to defaults is persisted");
    }
}

void testValidationRejectsWithoutWriting(const std::filesystem::path& directory)
{
    persistence::Database database(directory / "validation.db");
    persistence::SettingsService service(database);
    persistence::AppSettings saved;
    saved.masterVolume = 40;
    service.save(saved);

    const auto expectRejected = [&](persistence::AppSettings candidate, const std::string& message) {
        try {
            service.save(candidate);
            check(false, message + " (no error was thrown)");
        } catch (const std::invalid_argument&) {
            check(service.load().settings == saved, message);
        }
    };
    auto invalid = saved;
    invalid.masterVolume = 101;
    expectRejected(invalid, "master volume above 100 is rejected and nothing is written");
    invalid = saved;
    invalid.uiVolume = -1;
    expectRejected(invalid, "negative UI volume is rejected and nothing is written");
    invalid = saved;
    invalid.gameplayVolume = 1000;
    expectRejected(invalid, "gameplay volume far out of range is rejected");
    invalid = saved;
    invalid.achievementVolume = -50;
    expectRejected(invalid, "negative achievement volume is rejected");
}

void testMalformedStoredValues(const std::filesystem::path& directory)
{
    persistence::Database database(directory / "malformed.db");
    setRaw(database, persistence::settings_keys::masterVolume, "abc");
    setRaw(database, persistence::settings_keys::uiVolume, "150");
    setRaw(database, persistence::settings_keys::gameplayVolume, " 50");
    setRaw(database, persistence::settings_keys::achievementVolume, "42");
    setRaw(database, persistence::settings_keys::audioMuted, "true");
    setRaw(database, persistence::settings_keys::reducedMotion, "1");
    setRaw(database, "future.unknown_setting", "whatever");

    persistence::SettingsService service(database);
    const auto loaded = service.load();
    check(loaded.settings.masterVolume == 80, "non-numeric master volume falls back to its default");
    check(loaded.settings.uiVolume == 70, "out-of-range UI volume falls back to its default");
    check(loaded.settings.gameplayVolume == 80, "padded gameplay volume is treated as malformed");
    check(loaded.settings.achievementVolume == 42, "valid neighbouring values are still read");
    check(!loaded.settings.audioMuted, "non-0/1 mute flag falls back to Off");
    check(loaded.settings.reducedMotion, "valid reduced-motion flag is read");
    check(loaded.issues.size() == 4, "each malformed value is reported exactly once; unknown keys are ignored");

    for (const auto& text : {"-5", "5e1", "", "1000", "+7", "7.5"}) {
        auto parsed = persistence::SettingsService::parse({{persistence::settings_keys::masterVolume, text}});
        check(parsed.settings.masterVolume == 80 && parsed.issues.size() == 1,
              std::string("malformed volume text '") + text + "' is rejected");
    }
    auto boundary = persistence::SettingsService::parse({{persistence::settings_keys::masterVolume, "100"},
                                                         {persistence::settings_keys::uiVolume, "0"},
                                                         {persistence::settings_keys::gameplayVolume, "007"}});
    check(boundary.issues.empty() && boundary.settings.masterVolume == 100 && boundary.settings.uiVolume == 0 &&
              boundary.settings.gameplayVolume == 7,
          "0, 100, and leading zeros are accepted");

    service.save(loaded.settings);
    check(service.load().issues.empty(), "saving repairs previously malformed stored values");
}

void testSettingsAreAppWide(const std::filesystem::path& directory)
{
    persistence::Database database(directory / "appwide.db");
    persistence::ProfileService profiles(database);
    profiles.bootstrap();
    persistence::SettingsService settings(database);
    persistence::AppSettings custom;
    custom.uiVolume = 12;
    custom.reducedMotion = true;
    settings.save(custom);

    const auto other = profiles.createProfile("Second");
    profiles.setActiveProfile(other.id);
    check(settings.load().settings == custom, "switching the active profile does not change app settings");
    for (const auto& profile : profiles.listProfiles()) profiles.deleteProfile(profile.id);
    check(settings.load().settings == custom, "deleting every profile keeps app settings");
}

void testMigrationFromVersionThree(const std::filesystem::path& directory)
{
    const auto path = directory / "v3.db";
    std::int64_t keeperId = 0;
    std::int64_t otherId = 0;
    std::string before;
    {
        persistence::Database database(path);
        persistence::ProfileService profiles(database);
        profiles.bootstrap();
        const auto keeper = profiles.createProfile("Keeper");
        const auto other = profiles.createProfile("Other");
        profiles.setActiveProfile(keeper.id);
        keeperId = keeper.id;
        otherId = other.id;
        test_support::downgradeToVersion(database, 3);
        check(database.userVersion() == 3, "fixture is a genuine v3 database");
        check(scalar(database, "SELECT COUNT(*) FROM sqlite_master WHERE name='app_settings';") == 0,
              "v3 fixture has no settings table");

        // Insert through the v3 schema directly, as a v3 build would have done.
        persistence::MatchService matches(database);
        matches.recordCompleted(classicWin(keeperId, 1000));
        matches.recordCompleted(pongLoss(keeperId, 2000));
        matches.recordCompleted(classicWin(otherId, 3000));
        auto unlock = database.prepare(
            "INSERT INTO achievement_unlocks(profile_id, achievement_key, unlocked_at) VALUES(?1, 'first_victory', 777);");
        unlock.bind(1, keeperId);
        unlock.step();
        before = matchRowSignature(database);
        bool rejected = false;
        try {
            database.execute("INSERT INTO matches(profile_id,game_key,mode_key,opponent_name,profile_display_name,result,"
                             "difficulty_key,match_format,duration_ms,started_at,completed_at) "
                             "VALUES(1,'sus','human_vs_human','B','A','win','none','single',1,1,1);");
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        check(rejected, "the v3 schema rejects game keys beyond Tic-Tac-Toe and Ping Pong");
    }
    {
        persistence::Database migrated(path);
        check(migrated.userVersion() == persistence::schema::currentVersion, "v3 database migrates to the current schema");
        check(matchRowSignature(migrated) == before, "every v3 match row is preserved exactly, including ids");
        check(scalar(migrated, "SELECT COUNT(*) FROM achievement_unlocks WHERE achievement_key='first_victory' "
                               "AND unlocked_at=777;") == 1,
              "achievement unlock facts survive the v4 migration");
        persistence::ProfileService profiles(migrated);
        check(profiles.activeProfile() && profiles.activeProfile()->id == keeperId,
              "active profile survives the v4 migration");
        check(scalar(migrated, "SELECT COUNT(*) FROM sqlite_master WHERE type='index' AND name IN "
                               "('matches_profile_completed','matches_profile_game_completed',"
                               "'matches_profile_result_completed');") == 3,
              "all three history indexes are recreated");
        check(scalar(migrated, "SELECT COUNT(*) FROM sqlite_master WHERE name='matches_v4';") == 0,
              "no temporary migration table remains");
        auto foreignKey = migrated.prepare("PRAGMA foreign_key_list(matches);");
        check(foreignKey.step() && foreignKey.text(2) == "profiles" && foreignKey.text(6) == "CASCADE",
              "rebuilt matches table still cascades from profiles");
        check(scalar(migrated, "SELECT COUNT(*) FROM pragma_foreign_key_check;") == 0,
              "migration leaves no foreign-key violations");

        const std::vector<std::string> keys{
            "classic_tic_tac_toe", "numerical_tic_tac_toe", "sus", "five_by_five_tic_tac_toe",
            "misere_tic_tac_toe", "four_in_a_row", "four_by_four_tic_tac_toe", "word_tic_tac_toe",
            "pyramid_tic_tac_toe", "diamond", "infinity_xo", "ultimate_xo", "memory_xo",
            "obstacle_tic_tac_toe", "ping_pong"};
        int accepted = 0;
        for (const auto& key : keys) {
            auto insert = migrated.prepare(
                "INSERT INTO matches(profile_id,game_key,mode_key,opponent_name,profile_display_name,result,"
                "difficulty_key,match_format,duration_ms,started_at,completed_at) "
                "VALUES(?1,?2,'human_vs_computer','Computer','Keeper','draw','standard','single',1,5,6);");
            insert.bind(1, otherId);
            insert.bind(2, key);
            insert.step();
            ++accepted;
        }
        check(accepted == 15, "v4 accepts all 15 graphical game keys and the standard difficulty");
        bool rejected = false;
        try {
            migrated.execute("INSERT INTO matches(profile_id,game_key,mode_key,opponent_name,profile_display_name,result,"
                             "difficulty_key,match_format,duration_ms,started_at,completed_at) "
                             "VALUES(1,'chess','human_vs_human','B','A','win','none','single',1,1,1);");
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        check(rejected, "v4 still rejects unknown game keys");
        rejected = false;
        try {
            migrated.execute("INSERT INTO matches(profile_id,game_key,mode_key,opponent_name,profile_display_name,result,"
                             "difficulty_key,match_format,duration_ms,started_at,completed_at) "
                             "VALUES(1,'sus','human_vs_human','B','A','win','impossible','single',1,1,1);");
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        check(rejected, "v4 still rejects unknown difficulty keys");

        profiles.deleteProfile(keeperId);
        check(scalar(migrated, "SELECT COUNT(*) FROM matches WHERE profile_id=" + std::to_string(keeperId) + ";") == 0 &&
                  scalar(migrated, "SELECT COUNT(*) FROM achievement_unlocks WHERE profile_id=" +
                                       std::to_string(keeperId) + ";") == 0,
              "profile deletion still cascades history and unlocks after the rebuild");
        check(scalar(migrated, "SELECT COUNT(*) FROM matches WHERE profile_id=" + std::to_string(otherId) + ";") == 16,
              "other profiles' history is untouched by the cascade");
    }
    {
        persistence::Database reopened(path);
        check(reopened.userVersion() == persistence::schema::currentVersion, "repeated open after v4 migration is idempotent");
        check(scalar(reopened, "SELECT COUNT(*) FROM sqlite_master WHERE name='app_settings';") == 1,
              "settings table exists exactly once");
    }
}

void testFailedMigrationRollsBack(const std::filesystem::path& directory)
{
    const auto path = directory / "rollback.db";
    std::string before;
    {
        persistence::Database database(path);
        persistence::ProfileService profiles(database);
        profiles.bootstrap();
        const auto profile = profiles.createProfile("Keeper");
        test_support::downgradeToVersion(database, 3);
        persistence::MatchService matches(database);
        matches.recordCompleted(classicWin(profile.id, 1000));
        // A conflicting object makes the v3-to-v4 migration fail part-way through.
        database.execute("CREATE TABLE matches_v4(conflict INTEGER);");
        before = matchRowSignature(database);
    }
    bool failed = false;
    try {
        persistence::Database migrated(path);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    check(failed, "a failing v3-to-v4 migration reports an error instead of opening");

    // Inspect the file without the migrating Database wrapper: the failed transaction must
    // have been rolled back completely, leaving an untouched v3 database.
    sqlite3* raw = nullptr;
    const auto utf8Path = path.u8string();
    check(sqlite3_open_v2(utf8Path.c_str(), &raw, SQLITE_OPEN_READWRITE, nullptr) == SQLITE_OK,
          "rolled-back database can be opened directly");
    const auto rawScalar = [raw](const char* sql) {
        sqlite3_stmt* statement = nullptr;
        std::int64_t value = -1;
        if (sqlite3_prepare_v2(raw, sql, -1, &statement, nullptr) == SQLITE_OK && sqlite3_step(statement) == SQLITE_ROW)
            value = sqlite3_column_int64(statement, 0);
        sqlite3_finalize(statement);
        return value;
    };
    check(rawScalar("PRAGMA user_version;") == 3, "failed migration leaves user_version at 3");
    check(rawScalar("SELECT COUNT(*) FROM sqlite_master WHERE name='app_settings';") == 0,
          "failed migration leaves no settings table behind");
    check(rawScalar("SELECT COUNT(*) FROM sqlite_master WHERE name='matches' AND "
                    "sql LIKE '%IN (''classic_tic_tac_toe'', ''ping_pong'')%';") == 1,
          "failed migration keeps the original v3 matches definition");
    check(rawScalar("SELECT COUNT(*) FROM matches;") == 1, "failed migration keeps every v3 match row");
    check(sqlite3_exec(raw, "DROP TABLE matches_v4;", nullptr, nullptr, nullptr) == SQLITE_OK,
          "conflict can be removed");
    sqlite3_close(raw);

    persistence::Database repaired(path);
    check(repaired.userVersion() == persistence::schema::currentVersion &&
              matchRowSignature(repaired) == before,
          "after the conflict is removed the migration succeeds and preserves the rows");
}

void testFutureVersionRejected(const std::filesystem::path& directory)
{
    const auto path = directory / "future.db";
    const int future = persistence::schema::currentVersion + 1;
    {
        persistence::Database database(path);
        persistence::SettingsService settings(database);
        persistence::AppSettings custom;
        custom.masterVolume = 33;
        settings.save(custom);
        database.execute(("PRAGMA user_version = " + std::to_string(future) + ";").c_str());
    }
    try {
        persistence::Database unsupported(path);
        check(false, "a future schema version must not open");
    } catch (const std::runtime_error& error) {
        check(std::string(error.what()).find("version " + std::to_string(future)) != std::string::npos,
              "future schema rejection names the unsupported version");
    }
}

} // namespace

int main()
{
    TemporaryDirectory temporary;
    testDefaultsAndRoundTrip(temporary.path);
    testValidationRejectsWithoutWriting(temporary.path);
    testMalformedStoredValues(temporary.path);
    testSettingsAreAppWide(temporary.path);
    testMigrationFromVersionThree(temporary.path);
    testFailedMigrationRollsBack(temporary.path);
    testFutureVersionRejected(temporary.path);

    if (failures == 0) {
        std::cout << "Settings tests passed: " << checks << " checks\n";
        return 0;
    }
    std::cerr << failures << " of " << checks << " settings checks failed\n";
    return 1;
}
