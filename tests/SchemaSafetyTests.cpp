// Database safety: databases the build must not accept are left byte-for-byte unchanged,
// every key the current schema accepts can be read back, and file paths are handled safely.
#include "Database.hpp"
#include "DatabasePaths.hpp"
#include "MatchRepository.hpp"
#include "MatchService.hpp"
#include "MatchTypes.hpp"
#include "ProfileService.hpp"
#include "Schema.hpp"
#include "StatisticsRepository.hpp"
#include "TestSchemaSupport.hpp"
#include "sqlite3.h"

#include <chrono>
#include <climits>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

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
        path = std::filesystem::temp_directory_path() / ("GameVerseArena-schema-safety-" + std::to_string(stamp));
        std::filesystem::create_directories(path);
    }
    ~TemporaryDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
    std::filesystem::path path;
};

std::string fileBytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

// Opens a raw connection (no migration) and runs a statement.
bool rawExecute(const std::filesystem::path& path, const std::string& sql)
{
    sqlite3* raw = nullptr;
    const auto utf8 = path.u8string();
    if (sqlite3_open_v2(utf8.c_str(), &raw, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        sqlite3_close(raw);
        return false;
    }
    const bool ok = sqlite3_exec(raw, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
    sqlite3_close(raw);
    return ok;
}

std::int64_t rawScalar(const std::filesystem::path& path, const char* sql)
{
    sqlite3* raw = nullptr;
    const auto utf8 = path.u8string();
    std::int64_t value = -1;
    if (sqlite3_open_v2(utf8.c_str(), &raw, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK) {
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(raw, sql, -1, &statement, nullptr) == SQLITE_OK && sqlite3_step(statement) == SQLITE_ROW)
            value = sqlite3_column_int64(statement, 0);
        sqlite3_finalize(statement);
    }
    sqlite3_close(raw);
    return value;
}

bool opens(const std::filesystem::path& path)
{
    try {
        persistence::Database database(path);
        return true;
    } catch (const std::runtime_error&) {
        return false;
    }
}

void testUnsupportedVersionsLeaveFileUnchanged(const std::filesystem::path& directory)
{
    const int current = persistence::schema::currentVersion;
    for (const long long version : {static_cast<long long>(current) + 1, -1LL, static_cast<long long>(INT_MAX)}) {
        const auto path = directory / ("unsupported-" + std::to_string(version) + ".db");
        {
            persistence::Database database(path);
            persistence::ProfileService profiles(database);
            profiles.bootstrap();
            database.execute(("PRAGMA user_version = " + std::to_string(version) + ";").c_str());
        }
        const auto before = fileBytes(path);
        check(!opens(path), "schema version " + std::to_string(version) + " is refused");
        check(fileBytes(path) == before, "refusing version " + std::to_string(version) + " leaves the file byte-identical");
    }
}

void testForeignDatabaseIsNotAdopted(const std::filesystem::path& directory)
{
    const auto path = directory / "foreign.db";
    check(rawExecute(path, "CREATE TABLE invoices(id INTEGER PRIMARY KEY, total INTEGER);"
                           "INSERT INTO invoices(total) VALUES (42);"),
          "fixture: another application's SQLite file");
    const auto before = fileBytes(path);
    check(!opens(path), "a non-empty SQLite file without a GameVerseArena version is refused");
    check(fileBytes(path) == before, "the other application's file is left byte-identical");
    check(rawScalar(path, "PRAGMA user_version;") == 0 &&
              rawScalar(path, "SELECT COUNT(*) FROM sqlite_master WHERE name='profiles';") == 0,
          "no GameVerseArena tables or version marker were added");

    const auto fresh = directory / "fresh.db";
    check(opens(fresh) && rawScalar(fresh, "PRAGMA user_version;") == persistence::schema::currentVersion,
          "a brand-new empty file is still initialised normally");
}

void testUnrelatedOrphanDoesNotBlockUpgrade(const std::filesystem::path& directory)
{
    const auto path = directory / "orphan.db";
    {
        persistence::Database database(path);
        persistence::ProfileService profiles(database);
        profiles.bootstrap();
        test_support::downgradeToVersion(database, 3);
    }
    // A tampered file: an unlock row whose profile does not exist (foreign keys off).
    check(rawExecute(path, "PRAGMA foreign_keys = OFF;"
                           "INSERT INTO achievement_unlocks(profile_id, achievement_key, unlocked_at) "
                           "VALUES(987654, 'first_victory', 5);"),
          "fixture: orphan unlock row in an unrelated table");
    check(opens(path), "an orphan row outside the rebuilt table does not block the v4 upgrade");
    check(rawScalar(path, "PRAGMA user_version;") == persistence::schema::currentVersion,
          "the upgrade completed");
    check(rawScalar(path, "SELECT COUNT(*) FROM achievement_unlocks WHERE profile_id=987654;") == 1,
          "the migration does not delete or rewrite unrelated rows");
}

void testEveryAcceptedGameKeyIsReadable(const std::filesystem::path& directory)
{
    const auto& keys = persistence::allGameKeys();
    check(keys.size() == 15, "the typed layer knows all 15 game keys");
    std::set<std::string> storage;
    for (const auto key : keys) {
        const std::string text = persistence::toStorage(key);
        storage.insert(text);
        check(persistence::gameKeyFromStorage(text) == key, "game key '" + text + "' round-trips");
    }
    check(storage.size() == keys.size(), "every game key has a distinct storage value");
    bool rejected = false;
    try { (void)persistence::gameKeyFromStorage("chess"); } catch (const std::runtime_error&) { rejected = true; }
    check(rejected, "an unknown stored key is still rejected by the decoder");

    persistence::Database database(directory / "all-keys.db");
    persistence::ProfileService profiles(database);
    profiles.bootstrap();
    const auto profile = profiles.activeProfile();
    std::int64_t completedAt = 1000;
    for (const auto key : keys) {
        auto insert = database.prepare(
            "INSERT INTO matches(profile_id,game_key,mode_key,opponent_name,profile_display_name,result,"
            "difficulty_key,match_format,duration_ms,started_at,completed_at) "
            "VALUES(?1,?2,'human_vs_computer','Computer','Player 1','win','standard','single',100,?3,?3);");
        insert.bind(1, profile->id);
        insert.bind(2, std::string(persistence::toStorage(key)));
        insert.bind(3, completedAt++);
        insert.step();
    }
    persistence::MatchRepository matches(database);
    bool readable = true;
    std::vector<persistence::CompletedMatch> page;
    try {
        page = matches.recent(profile->id, {}, 100, 0);
    } catch (const std::exception&) {
        readable = false;
    }
    check(readable && page.size() == keys.size(), "history lists a row for every game key the schema accepts");
    persistence::StatisticsRepository statistics(database);
    for (const auto key : keys) {
        persistence::MatchFilter filter;
        filter.game = key;
        check(matches.count(profile->id, filter) == 1 && statistics.forGame(profile->id, key).matches == 1,
              std::string("history filter and statistics work for ") + persistence::toStorage(key));
    }
    check(statistics.overall(profile->id).wins == 15, "overall statistics include every game");

    if (page.empty()) return;
    persistence::MatchService service(database);
    persistence::CompletedMatch unsupported = page.front();
    unsupported.game = persistence::GameKey::Sus;
    unsupported.profileScore = 1;
    unsupported.opponentScore = 0;
    rejected = false;
    try { service.recordCompleted(unsupported); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "this build refuses to record games it cannot validate yet");
}

void testExactTextLength(const std::filesystem::path& directory)
{
    const auto path = directory / "text.db";
    {
        persistence::Database database(path);
        database.execute("CREATE TABLE probe(value TEXT);");
    }
    sqlite3* raw = nullptr;
    const auto utf8 = path.u8string();
    sqlite3_open_v2(utf8.c_str(), &raw, SQLITE_OPEN_READWRITE, nullptr);
    sqlite3_stmt* statement = nullptr;
    sqlite3_prepare_v2(raw, "INSERT INTO probe(value) VALUES(?1);", -1, &statement, nullptr);
    const char embedded[] = {'1', '\0', 'x'};
    sqlite3_bind_text(statement, 1, embedded, 3, SQLITE_TRANSIENT);
    sqlite3_step(statement);
    sqlite3_finalize(statement);
    sqlite3_close(raw);

    persistence::Database database(path);
    auto read = database.prepare("SELECT value FROM probe;");
    check(read.step() && read.text(0).size() == 3, "text values with an embedded NUL are read at full length");
}

void testPathEquivalence(const std::filesystem::path& directory)
{
    using persistence::DatabasePaths;
    const auto file = directory / "same.db";
    std::ofstream(file) << "x";
    check(DatabasePaths::referToSameFile(file, file), "a path refers to itself");
    check(DatabasePaths::referToSameFile(file, directory / "sub" / ".." / "same.db"), "'..' segments are resolved");
    check(!DatabasePaths::referToSameFile(file, directory / "other.db"), "different files are distinguished");
    check(DatabasePaths::referToSameFile(directory / "missing" / "a.db", directory / "missing" / "." / "a.db"),
          "paths to files that do not exist yet are compared after normalisation");
#ifdef _WIN32
    auto upper = file.native();
    for (auto& character : upper) character = static_cast<wchar_t>(std::towupper(character));
    check(DatabasePaths::referToSameFile(file, std::filesystem::path(upper)), "letter case is ignored on Windows");
#endif
}

void testNonAsciiPaths(const std::filesystem::path& directory)
{
    const auto folder = directory / std::filesystem::u8path(u8"Jürgen Été Ω");
    std::filesystem::create_directories(folder);
    const auto path = folder / "profiles.db";
    {
        persistence::Database database(path);
        persistence::ProfileService profiles(database);
        profiles.bootstrap();
        profiles.createProfile(u8"José");
    }
    {
        persistence::Database reopened(path);
        persistence::ProfileService profiles(reopened);
        check(profiles.listProfiles().size() == 2, "a database under a non-ASCII folder opens, saves, and reopens");
    }
#ifdef _WIN32
    const DWORD required = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
    std::wstring previous(required > 0 ? required : 1, L'\0');
    const bool hadPrevious = required > 0 && GetEnvironmentVariableW(L"LOCALAPPDATA", previous.data(), required) > 0;
    if (hadPrevious) previous.resize(required - 1);
    SetEnvironmentVariableW(L"LOCALAPPDATA", folder.native().c_str());
    bool resolved = false;
    try {
        const auto production = persistence::DatabasePaths::productionDatabasePath();
        resolved = production == folder / "GameVerseArena" / "gameverse.db";
    } catch (const std::exception&) {
        resolved = false;
    }
    if (hadPrevious) SetEnvironmentVariableW(L"LOCALAPPDATA", previous.c_str());
    else SetEnvironmentVariableW(L"LOCALAPPDATA", nullptr);
    check(resolved, "a non-ASCII LOCALAPPDATA folder resolves to the correct production path");
#endif
}
void testProductionFolderGuard(const std::filesystem::path& directory)
{
#ifdef _WIN32
    using persistence::DatabasePaths;
    const auto fakeAppData = directory / "fake-local-app-data";
    std::filesystem::create_directories(fakeAppData);
    const DWORD required = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
    std::wstring previous(required > 0 ? required : 1, L'\0');
    const bool hadPrevious = required > 0 && GetEnvironmentVariableW(L"LOCALAPPDATA", previous.data(), required) > 0;
    if (hadPrevious) previous.resize(required - 1);
    SetEnvironmentVariableW(L"LOCALAPPDATA", fakeAppData.native().c_str());

    const auto production = DatabasePaths::productionDatabasePath();
    check(production == fakeAppData / "GameVerseArena" / "gameverse.db", "fixture: production path uses the fake folder");
    // The production folder does not exist yet: spellings are resolved by name.
    check(DatabasePaths::insideProductionFolder(production), "the production database itself is refused");
    check(DatabasePaths::insideProductionFolder(fakeAppData / "GAMEVERSEARENA" / "other.db"),
          "a differently cased production folder is refused before it exists");
    check(DatabasePaths::insideProductionFolder(fakeAppData / "GameVerseArena." / "gameverse.db"),
          "a trailing-dot spelling of the production folder is refused before it exists");
    check(DatabasePaths::insideProductionFolder(fakeAppData / "GameVerseArena " / "gameverse.db"),
          "a trailing-space spelling of the production folder is refused before it exists");
    check(!DatabasePaths::insideProductionFolder(fakeAppData / "GameVerseArena2" / "gameverse.db"),
          "a different sibling folder is allowed");
    check(!DatabasePaths::insideProductionFolder(directory / "scratch" / "smoke.db"), "an unrelated folder is allowed");
    // Once the folder exists, anything inside it (at any depth) is refused.
    std::filesystem::create_directories(fakeAppData / "GameVerseArena" / "nested");
    check(DatabasePaths::insideProductionFolder(fakeAppData / "GameVerseArena" / "nested" / "smoke.db"),
          "a file nested inside the existing production folder is refused");
    check(DatabasePaths::insideProductionFolder(fakeAppData / "sub" / ".." / "GameVerseArena" / "x.db"),
          "'..' spellings of the existing production folder are refused");

    if (hadPrevious) SetEnvironmentVariableW(L"LOCALAPPDATA", previous.c_str());
    else SetEnvironmentVariableW(L"LOCALAPPDATA", nullptr);
#else
    (void)directory;
#endif
}
} // namespace

int main()
{
    TemporaryDirectory temporary;
    testUnsupportedVersionsLeaveFileUnchanged(temporary.path);
    testForeignDatabaseIsNotAdopted(temporary.path);
    testUnrelatedOrphanDoesNotBlockUpgrade(temporary.path);
    testEveryAcceptedGameKeyIsReadable(temporary.path);
    testExactTextLength(temporary.path);
    testPathEquivalence(temporary.path);
    testNonAsciiPaths(temporary.path);
    testProductionFolderGuard(temporary.path);

    if (failures == 0) {
        std::cout << "Schema safety tests passed: " << checks << " checks\n";
        return 0;
    }
    std::cerr << failures << " of " << checks << " schema safety checks failed\n";
    return 1;
}
