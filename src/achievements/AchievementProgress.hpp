#pragma once

#include <algorithm>
#include <cstdint>

namespace achievements {

struct AchievementProgress {
    std::int64_t current{};
    std::int64_t target{};
};

inline AchievementProgress clampedProgress(std::int64_t current, std::int64_t target) noexcept
{
    return {std::clamp<std::int64_t>(current, 0, target), target};
}

} // namespace achievements
