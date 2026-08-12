#include "ProfileService.hpp"

#include "Database.hpp"

#include <cstddef>
#include <cstdint>

namespace persistence {
namespace {
bool isTrimWhitespace(unsigned char value)
{
    return value == ' ' || value == '\t' || value == '\n' || value == '\r' || value == '\f' || value == '\v';
}

std::size_t validateUtf8AndCount(const std::string& value)
{
    std::size_t count = 0;
    for (std::size_t index = 0; index < value.size();) {
        const auto lead = static_cast<unsigned char>(value[index]);
        std::size_t length = 0;
        std::uint32_t codepoint = 0;
        if (lead < 0x80) {
            length = 1; codepoint = lead;
        } else if ((lead & 0xE0) == 0xC0) {
            length = 2; codepoint = lead & 0x1F;
        } else if ((lead & 0xF0) == 0xE0) {
            length = 3; codepoint = lead & 0x0F;
        } else if ((lead & 0xF8) == 0xF0) {
            length = 4; codepoint = lead & 0x07;
        } else {
            throw ProfileError(ProfileErrorCode::InvalidName, "Profile name contains invalid text");
        }
        if (index + length > value.size()) {
            throw ProfileError(ProfileErrorCode::InvalidName, "Profile name contains invalid text");
        }
        for (std::size_t offset = 1; offset < length; ++offset) {
            const auto continuation = static_cast<unsigned char>(value[index + offset]);
            if ((continuation & 0xC0) != 0x80) {
                throw ProfileError(ProfileErrorCode::InvalidName, "Profile name contains invalid text");
            }
            codepoint = (codepoint << 6) | (continuation & 0x3F);
        }
        const bool overlong = (length == 2 && codepoint < 0x80) ||
                              (length == 3 && codepoint < 0x800) ||
                              (length == 4 && codepoint < 0x10000);
        if (overlong || codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
            throw ProfileError(ProfileErrorCode::InvalidName, "Profile name contains invalid text");
        }
        if (codepoint < 0x20 || (codepoint >= 0x7F && codepoint <= 0x9F)) {
            throw ProfileError(ProfileErrorCode::InvalidName, "Profile name cannot contain control characters");
        }
        ++count;
        index += length;
    }
    return count;
}
} // namespace

ProfileService::ProfileService(Database& database) : database_(database), repository_(database)
{
}

void ProfileService::bootstrap()
{
    auto change = database_.transaction();
    auto profiles = repository_.list();
    if (profiles.empty()) {
        const auto created = repository_.create("Player 1");
        repository_.setActive(created.id);
    } else if (!repository_.active().has_value()) {
        repository_.setActive(profiles.front().id);
    }
    change.commit();
}

std::vector<Profile> ProfileService::listProfiles() const
{
    return repository_.list();
}

std::optional<Profile> ProfileService::activeProfile() const
{
    return repository_.active();
}

Profile ProfileService::createProfile(const std::string& displayName)
{
    const auto normalized = validateAndNormalizeName(displayName);
    if (repository_.findByName(normalized).has_value()) {
        throw ProfileError(ProfileErrorCode::DuplicateName, "A profile with that name already exists");
    }
    return repository_.create(normalized);
}

Profile ProfileService::renameProfile(std::int64_t id, const std::string& displayName)
{
    const auto existing = repository_.findById(id);
    if (!existing.has_value()) throw ProfileError(ProfileErrorCode::NotFound, "Profile no longer exists");
    const auto normalized = validateAndNormalizeName(displayName);
    if (normalized == existing->displayName) return *existing;
    if (repository_.findByName(normalized, id).has_value()) {
        throw ProfileError(ProfileErrorCode::DuplicateName, "A profile with that name already exists");
    }
    return repository_.rename(id, normalized);
}

bool ProfileService::deleteProfile(std::int64_t id)
{
    if (!repository_.findById(id).has_value()) return false;

    auto change = database_.transaction();
    const auto active = repository_.active();
    if (active.has_value() && active->id == id) repository_.clearActive();
    const bool removed = repository_.remove(id);

    if (active.has_value() && active->id == id) {
        auto remaining = repository_.list();
        if (remaining.empty()) {
            const auto replacement = repository_.create("Player 1");
            repository_.setActive(replacement.id);
        } else {
            repository_.setActive(remaining.front().id);
        }
    }
    change.commit();
    return removed;
}

Profile ProfileService::setActiveProfile(std::int64_t id)
{
    if (!repository_.findById(id).has_value()) {
        throw ProfileError(ProfileErrorCode::NotFound, "Profile no longer exists");
    }
    auto change = database_.transaction();
    repository_.setActive(id);
    change.commit();
    return *repository_.active();
}

std::string ProfileService::validateAndNormalizeName(const std::string& displayName)
{
    std::size_t first = 0;
    while (first < displayName.size() && isTrimWhitespace(static_cast<unsigned char>(displayName[first]))) ++first;
    std::size_t last = displayName.size();
    while (last > first && isTrimWhitespace(static_cast<unsigned char>(displayName[last - 1]))) --last;

    const std::string normalized = displayName.substr(first, last - first);
    if (normalized.empty()) {
        throw ProfileError(ProfileErrorCode::InvalidName, "Profile name cannot be empty");
    }
    if (validateUtf8AndCount(normalized) > 24) {
        throw ProfileError(ProfileErrorCode::InvalidName, "Profile name can contain at most 24 characters");
    }
    return normalized;
}

} // namespace persistence
