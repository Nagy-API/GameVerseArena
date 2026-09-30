#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <optional>
#include <vector>

// 5x5 Tic-Tac-Toe, ported from the console game ("5x5 Tic Tac Toe.cpp"): X and O alternate
// on a 5x5 grid, X first. The game ends after 24 moves (one cell stays empty). Every run of
// three identical marks in a row, column, or diagonal scores a point, overlapping runs
// included; the higher score wins and equal scores draw. Moves are cell indices 0-24.
namespace five_by_five {

using turn_based::MoveId;
using turn_based::Seat;

enum class Mark : char { Empty = '.', X = 'X', O = 'O' };

class FiveByFiveGame final : public turn_based::TurnBasedGame {
public:
    static constexpr int size = 5;
    static constexpr int finalMoveCount = 24;
    using Triple = std::array<int, 3>;

    FiveByFiveGame();

    static Mark markFor(Seat seat) noexcept { return seat == Seat::First ? Mark::X : Mark::O; }

    Mark cell(int index) const { return cells_.at(static_cast<std::size_t>(index)); }
    int triples(Mark mark) const;
    // Every scoring run of three on the board, for highlighting.
    std::vector<Triple> triplesOf(Mark mark) const;

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

private:
    std::array<Mark, 25> cells_{};
    Seat turn_{Seat::First};
    int moves_{0};
    turn_based::Outcome outcome_{};
};

} // namespace five_by_five
