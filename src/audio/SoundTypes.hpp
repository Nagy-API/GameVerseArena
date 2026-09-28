#pragma once

#include <cstddef>

namespace audio {

enum class SoundCategory { Ui, Gameplay, Achievement };

// Every sound is generated procedurally in memory; no audio files are shipped.
enum class SoundId : std::size_t {
    UiFocus,
    UiConfirm,
    UiBack,
    UiError,
    MovePrimary,
    MoveSecondary,
    SpecialEvent,
    RoundWin,
    RoundDraw,
    RoundLoss,
    PaddleHit,
    WallHit,
    PointScored,
    MatchWin,
    AchievementUnlocked,
    Count
};

inline constexpr std::size_t soundCount = static_cast<std::size_t>(SoundId::Count);

SoundCategory categoryOf(SoundId id) noexcept;
const char* soundName(SoundId id) noexcept;

// Volumes are percentages in [0, 100]; out-of-range inputs are clamped.
struct MixLevels {
    int master{80};
    int ui{70};
    int gameplay{80};
    int achievement{85};
    bool muted{false};
};

// Returns the SFML-style playback volume in [0, 100]: master * category / 100, or 0 when muted.
float effectiveVolume(const MixLevels& levels, SoundCategory category) noexcept;

} // namespace audio
