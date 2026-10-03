#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <deque>
#include <optional>
#include <vector>

// Infinity XO, ported from the console game (Infinity_XO.cpp): X and O alternate on a 3x3 grid,
// X first. After the 6th move the oldest mark on the board disappears, and after the 9th move the
// next-oldest disappears too; the removal happens before the move is checked for a win. Three in
// a row wins; if the 9th move does not win, the game is a draw. Moves are cell indices 0-8.
namespace infinity_xo {

using turn_based::MoveId;
using turn_based::Seat;

enum class Mark : char { Empty = '.', X = 'X', O = 'O' };

class InfinityGame final : public turn_based::TurnBasedGame {
public:
    using Line = std::array<int, 3>;
    static constexpr int finalMove = 9;

    InfinityGame();

    static Mark markFor(Seat seat) noexcept { return seat == Seat::First ? Mark::X : Mark::O; }
    // Whether playing move number `moveNumber` (1-based) removes the oldest mark afterwards.
    static bool removesOldestAfter(int moveNumber) noexcept { return moveNumber > 3 && (moveNumber - 3) % 3 == 0; }

    Mark cell(int index) const { return cells_.at(static_cast<std::size_t>(index)); }
    // The mark the next move will remove (for highlighting), if the next move removes one.
    std::optional<int> nextToVanish() const;
    // The cell emptied by the most recent move, if it removed a mark, and the mark it held.
    std::optional<int> lastRemoved() const noexcept { return lastRemoved_; }
    Mark lastRemovedMark() const noexcept { return lastRemovedMark_; }
    const std::optional<Line>& winningLine() const noexcept { return winningLine_; }

    std::unique_ptr<turn_based::TurnBasedGame> clone() const override;
    void reset() override;
    Seat currentSeat() const override { return turn_; }
    turn_based::Outcome outcome() const override { return outcome_; }
    std::vector<MoveId> legalMoves() const override;
    bool isLegal(MoveId move) const override;
    bool play(MoveId move) override;
    int movesPlayed() const override { return moves_; }
    std::optional<MoveId> chooseComputerMove(std::mt19937& random, const turn_based::CancelToken& cancel) const override;

private:
    std::array<Mark, 9> cells_{};
    std::deque<int> history_;  // cells of the marks still on the board, oldest first
    Seat turn_{Seat::First};
    int moves_{0};
    turn_based::Outcome outcome_{};
    std::optional<Line> winningLine_;
    std::optional<int> lastRemoved_;
    Mark lastRemovedMark_{Mark::Empty};
};

} // namespace infinity_xo
