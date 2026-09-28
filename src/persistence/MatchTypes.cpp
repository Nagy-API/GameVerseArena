#include "MatchTypes.hpp"

#include <stdexcept>

namespace persistence {
const std::vector<GameKey>& allGameKeys()
{
    static const std::vector<GameKey> keys{
        GameKey::ClassicTicTacToe, GameKey::NumericalTicTacToe, GameKey::Sus, GameKey::FiveByFiveTicTacToe,
        GameKey::MisereTicTacToe, GameKey::FourInARow, GameKey::FourByFourTicTacToe, GameKey::WordTicTacToe,
        GameKey::PyramidTicTacToe, GameKey::Diamond, GameKey::InfinityXo, GameKey::UltimateXo, GameKey::MemoryXo,
        GameKey::ObstacleTicTacToe, GameKey::PingPong};
    return keys;
}

const char* toStorage(GameKey value) noexcept
{
    switch (value) {
    case GameKey::ClassicTicTacToe: return "classic_tic_tac_toe";
    case GameKey::NumericalTicTacToe: return "numerical_tic_tac_toe";
    case GameKey::Sus: return "sus";
    case GameKey::FiveByFiveTicTacToe: return "five_by_five_tic_tac_toe";
    case GameKey::MisereTicTacToe: return "misere_tic_tac_toe";
    case GameKey::FourInARow: return "four_in_a_row";
    case GameKey::FourByFourTicTacToe: return "four_by_four_tic_tac_toe";
    case GameKey::WordTicTacToe: return "word_tic_tac_toe";
    case GameKey::PyramidTicTacToe: return "pyramid_tic_tac_toe";
    case GameKey::Diamond: return "diamond";
    case GameKey::InfinityXo: return "infinity_xo";
    case GameKey::UltimateXo: return "ultimate_xo";
    case GameKey::MemoryXo: return "memory_xo";
    case GameKey::ObstacleTicTacToe: return "obstacle_tic_tac_toe";
    case GameKey::PingPong: return "ping_pong";
    }
    return "classic_tic_tac_toe";
}
const char* toStorage(MatchMode value) noexcept { return value == MatchMode::HumanVsHuman ? "human_vs_human" : "human_vs_computer"; }
const char* toStorage(MatchResult value) noexcept
{
    if (value == MatchResult::Win) return "win";
    if (value == MatchResult::Loss) return "loss";
    return "draw";
}
const char* toStorage(DifficultyKey value) noexcept
{
    if (value == DifficultyKey::Easy) return "easy";
    if (value == DifficultyKey::Medium) return "medium";
    if (value == DifficultyKey::Hard) return "hard";
    if (value == DifficultyKey::Standard) return "standard";
    return "none";
}
GameKey gameKeyFromStorage(const std::string& value)
{
    for (const auto key : allGameKeys()) {
        if (value == toStorage(key)) return key;
    }
    throw std::runtime_error("Unknown stored game key");
}
MatchMode matchModeFromStorage(const std::string& value)
{
    if (value == "human_vs_human") return MatchMode::HumanVsHuman;
    if (value == "human_vs_computer") return MatchMode::HumanVsComputer;
    throw std::runtime_error("Unknown stored match mode");
}
MatchResult matchResultFromStorage(const std::string& value)
{
    if (value == "win") return MatchResult::Win;
    if (value == "loss") return MatchResult::Loss;
    if (value == "draw") return MatchResult::Draw;
    throw std::runtime_error("Unknown stored match result");
}
DifficultyKey difficultyFromStorage(const std::string& value)
{
    if (value == "none") return DifficultyKey::None;
    if (value == "easy") return DifficultyKey::Easy;
    if (value == "medium") return DifficultyKey::Medium;
    if (value == "hard") return DifficultyKey::Hard;
    if (value == "standard") return DifficultyKey::Standard;
    throw std::runtime_error("Unknown stored difficulty");
}
} // namespace persistence
