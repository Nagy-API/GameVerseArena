#pragma once

#include "TicTacToeBoard.hpp"
#include "TicTacToeTypes.hpp"

#include <string>

namespace classic_ttt {

class TicTacToeSession {
public:
    void startNewMatch(SessionConfig config);
    bool playMove(Position position);
    void restartRound();
    bool nextRound();
    void rematch();

    const TicTacToeBoard& board() const noexcept { return board_; }
    const SessionConfig& config() const noexcept { return config_; }
    const MatchScore& score() const noexcept { return score_; }
    Cell currentTurn() const noexcept { return currentTurn_; }
    unsigned int roundNumber() const noexcept { return roundNumber_; }
    bool matchFinished() const noexcept;
    bool isComputerTurn() const noexcept;
    Cell computerMark() const noexcept;
    std::string playerName(Cell mark) const;
    unsigned int winsNeeded() const noexcept;

private:
    void recordRoundOnce();
    void clearBoard();

    SessionConfig config_{};
    TicTacToeBoard board_{};
    MatchScore score_{};
    Cell currentTurn_{Cell::X};
    unsigned int roundNumber_{1};
    bool roundRecorded_{false};
};

} // namespace classic_ttt
