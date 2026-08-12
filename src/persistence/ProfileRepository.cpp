#include "ProfileRepository.hpp"

#include "Database.hpp"

#include <stdexcept>

namespace persistence {

std::vector<Profile> ProfileRepository::list() const
{
    auto statement = database_.prepare(
        "SELECT id, display_name, created_at, updated_at, last_used_at "
        "FROM profiles ORDER BY last_used_at DESC, created_at ASC, id ASC;");
    std::vector<Profile> profiles;
    while (statement.step()) profiles.push_back(readProfile(statement));
    return profiles;
}

std::optional<Profile> ProfileRepository::findById(std::int64_t id) const
{
    auto statement = database_.prepare(
        "SELECT id, display_name, created_at, updated_at, last_used_at FROM profiles WHERE id = ?1;");
    statement.bind(1, id);
    if (!statement.step()) return std::nullopt;
    return readProfile(statement);
}

std::optional<Profile> ProfileRepository::findByName(
    const std::string& displayName, std::optional<std::int64_t> excludingId) const
{
    auto statement = database_.prepare(excludingId.has_value()
        ? "SELECT id, display_name, created_at, updated_at, last_used_at FROM profiles "
          "WHERE display_name = ?1 COLLATE NOCASE AND id <> ?2;"
        : "SELECT id, display_name, created_at, updated_at, last_used_at FROM profiles "
          "WHERE display_name = ?1 COLLATE NOCASE;");
    statement.bind(1, displayName);
    if (excludingId.has_value()) statement.bind(2, *excludingId);
    if (!statement.step()) return std::nullopt;
    return readProfile(statement);
}

Profile ProfileRepository::create(const std::string& displayName)
{
    auto statement = database_.prepare(
        "INSERT INTO profiles(display_name, created_at, updated_at, last_used_at) "
        "VALUES(?1, CAST(unixepoch('subsec') * 1000 AS INTEGER), "
        "CAST(unixepoch('subsec') * 1000 AS INTEGER), CAST(unixepoch('subsec') * 1000 AS INTEGER));");
    statement.bind(1, displayName);
    statement.step();
    const auto profile = findById(database_.lastInsertId());
    if (!profile.has_value()) throw std::runtime_error("Created profile could not be read back");
    return *profile;
}

Profile ProfileRepository::rename(std::int64_t id, const std::string& displayName)
{
    auto statement = database_.prepare(
        "UPDATE profiles SET display_name = ?1, "
        "updated_at = MAX(CAST(unixepoch('subsec') * 1000 AS INTEGER), updated_at + 1) WHERE id = ?2;");
    statement.bind(1, displayName);
    statement.bind(2, id);
    statement.step();
    const auto profile = findById(id);
    if (!profile.has_value()) throw ProfileError(ProfileErrorCode::NotFound, "Profile no longer exists");
    return *profile;
}

bool ProfileRepository::remove(std::int64_t id)
{
    if (!findById(id).has_value()) return false;
    auto statement = database_.prepare("DELETE FROM profiles WHERE id = ?1;");
    statement.bind(1, id);
    statement.step();
    return true;
}

std::optional<Profile> ProfileRepository::active() const
{
    auto statement = database_.prepare(
        "SELECT p.id, p.display_name, p.created_at, p.updated_at, p.last_used_at "
        "FROM app_state s JOIN profiles p ON p.id = s.profile_id "
        "WHERE s.key = 'active_profile_id';");
    if (!statement.step()) return std::nullopt;
    return readProfile(statement);
}

void ProfileRepository::setActive(std::int64_t id)
{
    auto updateTimestamp = database_.prepare(
        "UPDATE profiles SET last_used_at = "
        "MAX(CAST(unixepoch('subsec') * 1000 AS INTEGER), "
        "(SELECT COALESCE(MAX(other.last_used_at), 0) + 1 FROM profiles AS other)) WHERE id = ?1;");
    updateTimestamp.bind(1, id);
    updateTimestamp.step();

    auto statement = database_.prepare(
        "INSERT INTO app_state(key, profile_id) VALUES('active_profile_id', ?1) "
        "ON CONFLICT(key) DO UPDATE SET profile_id = excluded.profile_id;");
    statement.bind(1, id);
    statement.step();
}

void ProfileRepository::clearActive()
{
    database_.execute("DELETE FROM app_state WHERE key = 'active_profile_id';");
}

Profile ProfileRepository::readProfile(Statement& statement)
{
    return {statement.integer(0), statement.text(1), statement.integer(2),
            statement.integer(3), statement.integer(4)};
}

} // namespace persistence
