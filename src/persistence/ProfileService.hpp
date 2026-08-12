#pragma once

#include "PersistenceTypes.hpp"
#include "ProfileRepository.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace persistence {

class Database;

class ProfileService {
public:
    explicit ProfileService(Database& database);

    void bootstrap();
    std::vector<Profile> listProfiles() const;
    std::optional<Profile> activeProfile() const;
    Profile createProfile(const std::string& displayName);
    Profile renameProfile(std::int64_t id, const std::string& displayName);
    bool deleteProfile(std::int64_t id);
    Profile setActiveProfile(std::int64_t id);

    static std::string validateAndNormalizeName(const std::string& displayName);

private:
    Database& database_;
    ProfileRepository repository_;
};

} // namespace persistence
