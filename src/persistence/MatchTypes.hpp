#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace persistence {

// Every game key the v4 schema's CHECK constraint accepts, so each stored row can be decoded.
enum class GameKey {
    ClassicTicTacToe,
    NumericalTicTacToe,
    Sus,
    FiveByFiveTicTacToe,
    MisereTicTacToe,
    FourInARow,
    FourByFourTicTacToe,
    WordTicTacToe,
    PyramidTicTacToe,
    Diamond,
    InfinityXo,
    UltimateXo,
    MemoryXo,
    ObstacleTicTacToe,
    PingPong
};

const std::vector<GameKey>& allGameKeys();
enum class MatchMode { HumanVsHuman, HumanVsComputer };
enum class MatchResult { Win, Loss, Draw };
// Standard identifies a computer opponent whose game offers one faithful strategy rather
// than selectable Easy/Medium/Hard levels. None is used only for human-vs-human matches.
enum class DifficultyKey { None, Easy, Medium, Hard, Standard };

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
