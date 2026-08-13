#pragma once

#include "AchievementProgress.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace achievements {

enum class AchievementCategory {
    General,
    TicTacToe,
    PingPong
};

struct AchievementDefinition {
    std::string key;
    std::string title;
    std::string description;
    AchievementCategory category{AchievementCategory::General};
    bool hidden{false};
    bool numericProgress{false};
    std::int64_t progressTarget{};
};

struct AchievementSnapshot {
    std::int64_t totalMatches{};
    std::int64_t totalWins{};
    std::int64_t bestWinStreak{};
    std::int64_t classicWins{};
    std::int64_t pingPongWins{};
    std::int64_t classicXWins{};
    std::int64_t classicOWins{};
    bool hasPingPongFiveZeroWin{};
    bool hasPingPongFiveFourWin{};
};

struct AchievementEvaluation {
    std::string key;
    bool satisfied{};
    std::optional<AchievementProgress> progress;
};

struct AchievementStatus {
    AchievementDefinition definition;
    std::optional<std::int64_t> unlockedAt;
    std::optional<AchievementProgress> progress;
};

} // namespace achievements
