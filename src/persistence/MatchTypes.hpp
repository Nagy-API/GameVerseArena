#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace persistence {

enum class GameKey { ClassicTicTacToe, PingPong };
enum class MatchMode { HumanVsHuman, HumanVsComputer };
enum class MatchResult { Win, Loss, Draw };
enum class DifficultyKey { None, Easy, Medium, Hard };

struct CompletedMatch {
    std::int64_t id{};
    std::int64_t profileId{};
    GameKey game{GameKey::ClassicTicTacToe};
    MatchMode mode{MatchMode::HumanVsHuman};
    std::string opponentName;
    std::string profileDisplayName;
    std::optional<std::string> profileSideOrMark;
    std::optional<std::string> opponentSideOrMark;
    MatchResult result{MatchResult::Draw};
    std::optional<int> profileScore;
    std::optional<int> opponentScore;
    std::optional<int> drawValue;
    DifficultyKey difficulty{DifficultyKey::None};
    std::string matchFormat;
    std::int64_t durationMs{};
    std::int64_t startedAt{};
    std::int64_t completedAt{};
};

struct MatchFilter {
    std::optional<GameKey> game;
    std::optional<MatchResult> result;
};

struct OverallStatistics {
    std::int64_t matches{};
    std::int64_t wins{};
    std::int64_t losses{};
    std::int64_t draws{};
    double winRate{};
    std::int64_t totalDurationMs{};
    std::optional<std::int64_t> lastPlayedAt;
    std::int64_t currentWinStreak{};
    std::int64_t bestWinStreak{};
};

struct GameStatistics : OverallStatistics {
    std::int64_t pointsScored{};
    std::int64_t pointsConceded{};
    std::int64_t bestFinalMargin{};
    std::int64_t ticTacToeAsX{};
    std::int64_t ticTacToeAsO{};
    std::int64_t singleMatches{};
    std::int64_t bestOfThreeMatches{};
    std::int64_t bestOfFiveMatches{};
};

const char* toStorage(GameKey value) noexcept;
const char* toStorage(MatchMode value) noexcept;
const char* toStorage(MatchResult value) noexcept;
const char* toStorage(DifficultyKey value) noexcept;
GameKey gameKeyFromStorage(const std::string& value);
MatchMode matchModeFromStorage(const std::string& value);
MatchResult matchResultFromStorage(const std::string& value);
DifficultyKey difficultyFromStorage(const std::string& value);

} // namespace persistence
