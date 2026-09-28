#pragma once

#include "AchievementTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <vector>

namespace achievements {

class AchievementNotificationQueue {
public:
    explicit AchievementNotificationQueue(float displaySeconds = 3.5f);

    void enqueue(const std::vector<AchievementDefinition>& achievements);
    void update(float deltaSeconds);
    void dismiss();
    const AchievementDefinition* current() const noexcept;
    std::size_t pendingCount() const noexcept { return pending_.size(); }
    // Increases by one each time a different notification becomes the current one, so
    // presentation can start effects (sound, slide-in) exactly once per notification.
    std::uint64_t sequence() const noexcept { return sequence_; }

private:
    bool contains(const std::string& key) const;

    std::deque<AchievementDefinition> pending_;
    float displaySeconds_;
    float elapsed_{};
    std::uint64_t sequence_{};
};

} // namespace achievements
