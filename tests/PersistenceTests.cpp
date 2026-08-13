#include "Database.hpp"
#include "DatabasePaths.hpp"
#include "ProfileService.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct TemporaryDirectory {
    TemporaryDirectory()
    {
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() /
               ("GameVerseArena-persistence-tests-" + std::to_string(stamp));
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

template <typename Action>
void expectProfileError(Action action, persistence::ProfileErrorCode code, const std::string& message)
{
    try {
        action();
        check(false, message + " (no error was thrown)");
    } catch (const persistence::ProfileError& error) {
        check(error.code() == code, message);
    }
}

void testDatabaseSchema(const std::filesystem::path& directory)
{
    const auto path = directory / "schema.db";
    {
        persistence::Database database(path);
        check(database.userVersion() == 3, "new database creates schema version 3");
        check(database.foreignKeysEnabled(), "foreign key enforcement is enabled");
        database.execute("CREATE TABLE schema_probe(value INTEGER);");
    }
    {
        persistence::Database database(path);
        check(database.userVersion() == 3, "version 3 database reopens");
        auto statement = database.prepare("SELECT COUNT(*) FROM sqlite_master WHERE name = 'schema_probe';");
        check(statement.step() && statement.integer(0) == 1, "clean close and reopen preserves database");
    }

    const auto futurePath = directory / "future.db";
    {
        persistence::Database database(futurePath);
        database.execute("PRAGMA user_version = 4;");
    }
    try {
        persistence::Database unsupported(futurePath);
        check(false, "unsupported future schema fails safely");
    } catch (const std::runtime_error& error) {
        check(std::string(error.what()).find("Unsupported profile database schema version 4") != std::string::npos,
              "unsupported future schema reports its version");
    }
}

void testBootstrapAndCreate(const std::filesystem::path& directory)
{
    const auto path = directory / "profiles.db";
    persistence::Database database(path);
    persistence::ProfileService service(database);
    service.bootstrap();
    auto profiles = service.listProfiles();
    check(profiles.size() == 1 && profiles.front().displayName == "Player 1",
          "empty database bootstraps exactly one Player 1");
    check(service.activeProfile().has_value() && service.activeProfile()->id == profiles.front().id,
          "bootstrapped Player 1 is active");
    service.bootstrap();
    check(service.listProfiles().size() == 1, "repeated bootstrap is idempotent");

    const auto alice = service.createProfile("  Alice  ");
    check(alice.displayName == "Alice", "create trims surrounding whitespace");
    check(service.listProfiles().size() == 2, "valid profile creation succeeds");
    expectProfileError([&] { service.createProfile(""); }, persistence::ProfileErrorCode::InvalidName,
                       "empty name is rejected");
    expectProfileError([&] { service.createProfile(" \t\r\n "); }, persistence::ProfileErrorCode::InvalidName,
                       "whitespace-only name is rejected");
    expectProfileError([&] { service.createProfile("Bad\x01Name"); }, persistence::ProfileErrorCode::InvalidName,
                       "control characters are rejected");
    expectProfileError([&] { service.createProfile("1234567890123456789012345"); },
                       persistence::ProfileErrorCode::InvalidName, "names over 24 text units are rejected");
    const std::string twentyFourUtf8 =
        "\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9"
        "\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9"
        "\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9";
    check(service.createProfile(twentyFourUtf8).displayName == twentyFourUtf8,
          "24 multibyte UTF-8 text units are accepted");
    expectProfileError([&] { service.createProfile(twentyFourUtf8 + "\xC3\xA9"); },
                       persistence::ProfileErrorCode::InvalidName,
                       "25 multibyte UTF-8 text units are rejected");
    expectProfileError([&] { service.createProfile("alice"); }, persistence::ProfileErrorCode::DuplicateName,
                       "case-insensitive duplicate is rejected");
    expectProfileError([&] { service.createProfile(" ALICE "); }, persistence::ProfileErrorCode::DuplicateName,
                       "trimmed duplicate is rejected");

    const auto apostrophe = service.createProfile("King's Profile");
    const auto injection = service.createProfile("A; DROP TABLE profiles;");
    check(apostrophe.displayName == "King's Profile" && injection.displayName == "A; DROP TABLE profiles;",
          "SQL-looking profile names are stored as data");
    auto table = database.prepare("SELECT COUNT(*) FROM profiles;");
    check(table.step() && table.integer(0) == 5, "SQL-looking names leave schema and other rows intact");
}

void testRename(const std::filesystem::path& directory)
{
    persistence::Database database(directory / "rename.db");
    persistence::ProfileService service(database);
    service.bootstrap();
    const auto first = service.createProfile("First");
    service.createProfile("Second");
    const auto renamed = service.renameProfile(first.id, "  Renamed  ");
    check(renamed.displayName == "Renamed", "valid rename succeeds and trims");
    check(renamed.updatedAt > first.updatedAt, "rename advances updated timestamp");
    const auto same = service.renameProfile(first.id, "Renamed");
    check(same.updatedAt == renamed.updatedAt, "same current name is a sensible no-op");
    const auto recased = service.renameProfile(first.id, "RENAMED");
    check(recased.displayName == "RENAMED", "case-only rename preserves requested visible casing");
    expectProfileError([&] { service.renameProfile(first.id, "second"); },
                       persistence::ProfileErrorCode::DuplicateName, "duplicate rename fails");
    expectProfileError([&] { service.renameProfile(first.id, "   "); },
                       persistence::ProfileErrorCode::InvalidName, "invalid rename fails");
    const auto current = service.renameProfile(first.id, "RENAMED");
    check(current.displayName == "RENAMED", "invalid rename leaves original unchanged");
}

void testActiveAndDelete(const std::filesystem::path& directory)
{
    const auto path = directory / "active.db";
    std::int64_t chosenId = 0;
    std::int64_t chosenLastUsed = 0;
    {
        persistence::Database database(path);
        persistence::ProfileService service(database);
        service.bootstrap();
        const auto first = service.activeProfile();
        const auto second = service.createProfile("Second");
        const auto third = service.createProfile("Third");
        const auto before = second.lastUsedAt;
        const auto chosen = service.setActiveProfile(second.id);
        chosenId = chosen.id;
        chosenLastUsed = chosen.lastUsedAt;
        check(chosen.lastUsedAt > before, "setting active advances last-used timestamp");
        check(service.deleteProfile(third.id), "deleting an existing inactive profile succeeds");
        check(service.activeProfile()->id == second.id, "deleting inactive profile preserves active profile");
        service.setActiveProfile(first->id);
        chosenLastUsed = service.setActiveProfile(second.id).lastUsedAt;
        check(!service.deleteProfile(999999), "deleting a missing profile is safe");
    }
    {
        persistence::Database database(path);
        persistence::ProfileService service(database);
        service.bootstrap();
        check(service.activeProfile().has_value() && service.activeProfile()->id == chosenId,
              "active profile persists across close and reopen");
        check(service.activeProfile()->lastUsedAt == chosenLastUsed,
              "persisted active profile retains its last-used timestamp");
        check(service.deleteProfile(chosenId), "deleting active profile succeeds");
        check(service.activeProfile().has_value() && service.activeProfile()->displayName == "Player 1",
              "deleting active chooses the most recently used remaining profile");
        const auto replacementId = service.activeProfile()->id;
        check(service.deleteProfile(replacementId), "deleting final remaining profile succeeds");
        const auto profiles = service.listProfiles();
        check(profiles.size() == 1 && profiles.front().displayName == "Player 1",
              "deleting final profile recreates default");
        check(service.activeProfile().has_value() && service.activeProfile()->id == profiles.front().id,
              "recreated default is active with no orphaned reference");
    }
}

void testInjectedPath(const std::filesystem::path& directory)
{
    const auto injected = directory / "nested" / "injected.db";
    const auto production = persistence::DatabasePaths::productionDatabasePath();
    const bool productionExisted = std::filesystem::exists(production);
    std::filesystem::file_time_type productionWrite{};
    std::uintmax_t productionSize = 0;
    if (productionExisted) {
        productionWrite = std::filesystem::last_write_time(production);
        productionSize = std::filesystem::file_size(production);
    }
    {
        persistence::Database database(injected);
        persistence::ProfileService service(database);
        service.bootstrap();
        check(database.path() == injected, "explicit test database path is honored");
    }
    check(std::filesystem::exists(injected), "injected database is created at the test path");
    check(std::filesystem::exists(production) == productionExisted,
          "path-injected test does not create the production database");
    if (productionExisted) {
        check(std::filesystem::last_write_time(production) == productionWrite &&
              std::filesystem::file_size(production) == productionSize,
              "path-injected test leaves production database unchanged");
    }
}

} // namespace

int main()
{
    TemporaryDirectory temporary;
    testDatabaseSchema(temporary.path);
    testBootstrapAndCreate(temporary.path);
    testRename(temporary.path);
    testActiveAndDelete(temporary.path);
    testInjectedPath(temporary.path);

    if (failures == 0) {
        std::cout << "Persistence tests passed: " << checks << " checks\n";
        return 0;
    }
    std::cerr << failures << " of " << checks << " persistence checks failed\n";
    return 1;
}
