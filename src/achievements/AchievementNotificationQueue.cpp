#include "AchievementNotificationQueue.hpp"

#include <algorithm>
#include <stdexcept>

namespace achievements {

AchievementNotificationQueue::AchievementNotificationQueue(float displaySeconds)
    : displaySeconds_(displaySeconds)
{
    if (displaySeconds_ <= 0.f) throw std::invalid_argument("Achievement toast duration must be positive");
}

void AchievementNotificationQueue::enqueue(const std::vector<AchievementDefinition>& achievements)
{
    for (const auto& achievement : achievements) {
        if (!contains(achievement.key)) pending_.push_back(achievement);
    }
}

void AchievementNotificationQueue::update(float deltaSeconds)
{
    if (pending_.empty() || deltaSeconds <= 0.f) return;
    elapsed_ += deltaSeconds;
    if (elapsed_ >= displaySeconds_) dismiss();
}

void AchievementNotificationQueue::dismiss()
{
    if (!pending_.empty()) pending_.pop_front();
    elapsed_ = 0.f;
}

const AchievementDefinition* AchievementNotificationQueue::current() const noexcept
{
    return pending_.empty() ? nullptr : &pending_.front();
}

bool AchievementNotificationQueue::contains(const std::string& key) const
{
    return std::any_of(pending_.begin(), pending_.end(),
                       [&](const auto& item) { return item.key == key; });
}

} // namespace achievements
