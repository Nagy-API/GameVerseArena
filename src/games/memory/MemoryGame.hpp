#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <optional>
#include <vector>

// Memory XO, ported from the console game (Memory_XO.cpp): X and O alternate on a 3x3 grid, X
// first, but every mark is hidden once it is placed. Choosing a cell that is already taken is
// refused and the player must choose again. Three in a row wins; nine marks without a line is a
// draw. Moves are cell indices 0-8. The game itself knows every mark; hiding them is the view's job.
namespace memory_xo {

using turn_based::MoveId;
using turn_based::Seat;

enum class Mark : char { Empty = '.', X = 'X', O = 'O' };

class MemoryGame final : public turn_based::TurnBasedGame {
public:
    using Line = std::array<int, 3>;

    MemoryGame();

    static Mark markFor(Seat seat) noexcept { return seat == Seat::First ? Mark::X : Mark::O; }

    // The hidden mark in a cell (the view reveals it only when the game is over).
    Mark cell(int index) const { return cells_.at(static_cast<std::size_t>(index)); }
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
    Seat turn_{Seat::First};
    int moves_{0};
    turn_based::Outcome outcome_{};
    std::optional<Line> winningLine_;
};

} // namespace memory_xo
