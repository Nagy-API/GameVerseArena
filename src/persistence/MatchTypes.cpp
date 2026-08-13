#include "MatchTypes.hpp"

#include <stdexcept>

namespace persistence {
const char* toStorage(GameKey value) noexcept { return value == GameKey::ClassicTicTacToe ? "classic_tic_tac_toe" : "ping_pong"; }
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
    return "none";
}
GameKey gameKeyFromStorage(const std::string& value)
{
    if (value == "classic_tic_tac_toe") return GameKey::ClassicTicTacToe;
    if (value == "ping_pong") return GameKey::PingPong;
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
    throw std::runtime_error("Unknown stored difficulty");
}
} // namespace persistence
