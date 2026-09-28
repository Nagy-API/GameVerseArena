#include "SoundTypes.hpp"

#include <algorithm>

namespace audio {

SoundCategory categoryOf(SoundId id) noexcept
{
    switch (id) {
    case SoundId::UiFocus:
    case SoundId::UiConfirm:
    case SoundId::UiBack:
    case SoundId::UiError:
        return SoundCategory::Ui;
    case SoundId::AchievementUnlocked:
        return SoundCategory::Achievement;
    case SoundId::MovePrimary:
    case SoundId::MoveSecondary:
    case SoundId::SpecialEvent:
    case SoundId::RoundWin:
    case SoundId::RoundDraw:
    case SoundId::RoundLoss:
    case SoundId::PaddleHit:
    case SoundId::WallHit:
    case SoundId::PointScored:
    case SoundId::MatchWin:
    case SoundId::Count:
        break;
    }
    return SoundCategory::Gameplay;
}

const char* soundName(SoundId id) noexcept
{
    switch (id) {
    case SoundId::UiFocus: return "ui_focus";
    case SoundId::UiConfirm: return "ui_confirm";
    case SoundId::UiBack: return "ui_back";
    case SoundId::UiError: return "ui_error";
    case SoundId::MovePrimary: return "move_primary";
    case SoundId::MoveSecondary: return "move_secondary";
    case SoundId::SpecialEvent: return "special_event";
    case SoundId::RoundWin: return "round_win";
    case SoundId::RoundDraw: return "round_draw";
    case SoundId::RoundLoss: return "round_loss";
    case SoundId::PaddleHit: return "paddle_hit";
    case SoundId::WallHit: return "wall_hit";
    case SoundId::PointScored: return "point_scored";
    case SoundId::MatchWin: return "match_win";
    case SoundId::AchievementUnlocked: return "achievement_unlocked";
    case SoundId::Count: break;
    }
    return "unknown";
}

float effectiveVolume(const MixLevels& levels, SoundCategory category) noexcept
{
    if (levels.muted) return 0.f;
    const auto clampPercent = [](int value) { return std::clamp(value, 0, 100); };
    int categoryLevel = levels.gameplay;
    if (category == SoundCategory::Ui) categoryLevel = levels.ui;
    else if (category == SoundCategory::Achievement) categoryLevel = levels.achievement;
    return static_cast<float>(clampPercent(levels.master) * clampPercent(categoryLevel)) / 100.f;
}

} // namespace audio
