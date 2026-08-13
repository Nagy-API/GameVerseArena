#include "MatchService.hpp"

#include "Database.hpp"

#include <algorithm>
#include <stdexcept>

namespace persistence {
std::int64_t MatchService::recordCompleted(const CompletedMatch& match)
{
    validate(match);
    auto profile = database_.prepare("SELECT COUNT(*) FROM profiles WHERE id=?1;");
    profile.bind(1, match.profileId);
    if (!profile.step() || profile.integer(0) != 1) throw std::invalid_argument("Match profile does not exist");
    return repository_.insert(match);
}

void MatchService::validate(const CompletedMatch& value)
{
    if (value.profileId <= 0 || value.opponentName.empty() || value.profileDisplayName.empty() ||
        value.matchFormat.empty()) throw std::invalid_argument("Completed match is missing required data");
    if (value.durationMs < 0) throw std::invalid_argument("Match duration cannot be negative");
    if (value.startedAt < 0 || value.completedAt < value.startedAt)
        throw std::invalid_argument("Match timestamps are invalid");
    if (value.mode == MatchMode::HumanVsHuman && value.difficulty != DifficultyKey::None)
        throw std::invalid_argument("Human-vs-human match cannot have AI difficulty");
    if (value.mode == MatchMode::HumanVsComputer && value.difficulty == DifficultyKey::None)
        throw std::invalid_argument("Human-vs-computer match requires AI difficulty");
    if (value.game == GameKey::PingPong && value.result == MatchResult::Draw)
        throw std::invalid_argument("Ping Pong cannot end in a draw");
    if (!value.profileScore || !value.opponentScore)
        throw std::invalid_argument("Completed match requires a final score");
    if (*value.profileScore < 0 || *value.opponentScore < 0)
        throw std::invalid_argument("Completed match scores cannot be negative");
    const bool scoreSaysWin = *value.profileScore > *value.opponentScore;
    const bool scoreSaysLoss = *value.profileScore < *value.opponentScore;
    if ((value.result == MatchResult::Win && !scoreSaysWin) ||
        (value.result == MatchResult::Loss && !scoreSaysLoss) ||
        (value.result == MatchResult::Draw && *value.profileScore != *value.opponentScore))
        throw std::invalid_argument("Match result contradicts the final score");

    if (value.game == GameKey::ClassicTicTacToe) {
        const bool validFormat = value.matchFormat == "single" || value.matchFormat == "best_of_3" ||
                                 value.matchFormat == "best_of_5";
        if (!validFormat || !value.profileSideOrMark || !value.opponentSideOrMark ||
            ((*value.profileSideOrMark != "X" && *value.profileSideOrMark != "O")) ||
            ((*value.opponentSideOrMark != "X" && *value.opponentSideOrMark != "O")) ||
            value.profileSideOrMark == value.opponentSideOrMark || !value.drawValue || *value.drawValue < 0)
            throw std::invalid_argument("Tic-Tac-Toe completion data is invalid");
        const int winsNeeded = value.matchFormat == "single" ? 1 : value.matchFormat == "best_of_3" ? 2 : 3;
        if (value.result == MatchResult::Draw) {
            if (value.matchFormat != "single" || *value.profileScore != 0 || *value.opponentScore != 0 || *value.drawValue < 1)
                throw std::invalid_argument("Tic-Tac-Toe draw is not a completed single game");
        } else if (std::max(*value.profileScore, *value.opponentScore) != winsNeeded) {
            throw std::invalid_argument("Tic-Tac-Toe score has not completed its match format");
        }
    } else {
        if (value.matchFormat != "first_to_5" || value.drawValue ||
            value.profileSideOrMark != std::optional<std::string>{"Left"} ||
            value.opponentSideOrMark != std::optional<std::string>{"Right"} ||
            std::max(*value.profileScore, *value.opponentScore) != 5 ||
            std::min(*value.profileScore, *value.opponentScore) >= 5)
            throw std::invalid_argument("Ping Pong score is not a completed first-to-five match");
    }
}
} // namespace persistence
