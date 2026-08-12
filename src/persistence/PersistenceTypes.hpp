#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace persistence {

struct Profile {
    std::int64_t id{};
    std::string displayName;
    std::int64_t createdAt{};
    std::int64_t updatedAt{};
    std::int64_t lastUsedAt{};
};

enum class ProfileErrorCode {
    InvalidName,
    DuplicateName,
    NotFound
};

class ProfileError : public std::runtime_error {
public:
    ProfileError(ProfileErrorCode code, const std::string& message)
        : std::runtime_error(message), code_(code)
    {
    }

    ProfileErrorCode code() const noexcept { return code_; }

private:
    ProfileErrorCode code_;
};

} // namespace persistence
