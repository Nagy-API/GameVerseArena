#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <optional>
#include <vector>

// SUS, ported from the console game (SUS.cpp / SUS_AI.cpp): Player 1 always places 'S' and
// Player 2 always places 'U'. Every S-U-S line (row, column, or diagonal) through the letter
// just placed scores a point for its placer. After nine placements the higher score wins and
// equal scores draw. Moves are cell indices 0-8 (row * 3 + column).
namespace sus_game {

using turn_based::MoveId;
using turn_based::Seat;

class SusGame final : public turn_based::TurnBasedGame {
public:
    using Line = std::array<int, 3>;

    SusGame();

    static char letterFor(Seat seat) noexcept { return seat == Seat::First ? 'S' : 'U'; }

    char cell(int index) const { return cells_.at(static_cast<std::size_t>(index)); }  // '.', 'S', or 'U'
    int score(Seat seat) const noexcept { return seat == Seat::First ? scoreS_ : scoreU_; }
    // Every S-U-S line currently on the board, for highlighting.
    std::vector<Line> susLines() const;

    std::unique_ptr<turn_based::TurnBasedGame> clone() const override;
    void reset() override;
    Seat currentSeat() const override { return turn_; }
    turn_based::Outcome outcome() const override { return outcome_; }
    std::vector<MoveId> legalMoves() const override;
    bool isLegal(MoveId move) const override;
    bool play(MoveId move) override;
    int movesPlayed() const override { return moves_; }
    std::optional<turn_based::SeatScores> scores() const override;
    std::optional<MoveId> chooseComputerMove(std::mt19937& random, const turn_based::CancelToken& cancel) const override;

    // The console AI's scores for placing `letter` at `index` (exposed for tests).
    int countSusThrough(int index, char letter) const;
    int evaluateMove(int index, char letter) const;

private:
    std::array<char, 9> cells_{};
    int scoreS_{0};
    int scoreU_{0};
    Seat turn_{Seat::First};
    int moves_{0};
    turn_based::Outcome outcome_{};
};

} // namespace sus_game
