#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <optional>
#include <vector>

// Diamond, ported from the console game (Diamond.cpp): X and O alternate on the 25 cells of a
// diamond inside a 7x7 grid (rows of 1, 3, 5, 7, 5, 3, and 1 cells), X first. A player wins by
// holding a line of exactly three and a line of exactly four of their marks at the same time, in
// different directions (horizontal, vertical, or either diagonal); the lines may share a cell. A
// full board without a winner is a draw. Moves are grid indices, row * 7 + column.
//
// The console's win check remembered only the first line of three and the first line of four it
// met while scanning the board, so it could miss a valid pair when those two happened to share a
// direction. The graphical version applies the rule to every line.
namespace diamond {

using turn_based::MoveId;
using turn_based::Seat;

enum class Mark : char { Empty = '.', X = 'X', O = 'O' };

class DiamondGame final : public turn_based::TurnBasedGame {
public:
    static constexpr int size = 7;
    static constexpr int cellCount = 25;

    // The winning pair of lines, as grid indices.
    struct WinningLines {
        std::vector<int> three;
        std::vector<int> four;
    };

    DiamondGame();

    static Mark markFor(Seat seat) noexcept { return seat == Seat::First ? Mark::X : Mark::O; }
    static bool onBoard(int row, int column) noexcept;
    static MoveId moveAt(int row, int column) noexcept { return row * size + column; }

    Mark cell(int row, int column) const { return cells_.at(static_cast<std::size_t>(row * size + column)); }
    const std::optional<WinningLines>& winningLines() const noexcept { return winningLines_; }

    std::unique_ptr<turn_based::TurnBasedGame> clone() const override;
    void reset() override;
    Seat currentSeat() const override { return turn_; }
    turn_based::Outcome outcome() const override { return outcome_; }
    std::vector<MoveId> legalMoves() const override;
    bool isLegal(MoveId move) const override;
    bool play(MoveId move) override;
    int movesPlayed() const override { return moves_; }
    std::optional<MoveId> chooseComputerMove(std::mt19937& random, const turn_based::CancelToken& cancel) const override;

    // The console computer's line-strength score for `mark` (exposed for tests).
    int evaluate(Mark mark) const;

private:
    std::array<Mark, size * size> cells_{};  // cells outside the diamond stay empty and are never legal
    Seat turn_{Seat::First};
    int moves_{0};
    turn_based::Outcome outcome_{};
    std::optional<WinningLines> winningLines_;
};

} // namespace diamond
