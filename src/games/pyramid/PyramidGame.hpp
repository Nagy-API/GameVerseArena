#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <optional>
#include <vector>

// Pyramid Tic-Tac-Toe, ported from the console game (pyramid.cpp): a pyramid of 5, 3, and 1 cells,
// X first. Three marks in a line win: along the bottom row (three possible lines), across the middle
// row, straight up the centre, or along either sloped edge. A full pyramid without a line is a draw.
// Cells (and moves) are numbered from the bottom row up: 0-4 bottom (left to right), 5-7 middle,
// 8 the top.
namespace pyramid {

using turn_based::MoveId;
using turn_based::Seat;

enum class Mark : char { Empty = '.', X = 'X', O = 'O' };

class PyramidGame final : public turn_based::TurnBasedGame {
public:
    static constexpr int cellCount = 9;
    using Line = std::array<int, 3>;

    PyramidGame();

    static Mark markFor(Seat seat) noexcept { return seat == Seat::First ? Mark::X : Mark::O; }
    // The seven winning lines, in the console's checking order.
    static const std::array<Line, 7>& lines();
    // Row (0 = bottom) and column within that row for a cell.
    static int rowOf(int cell) noexcept { return cell < 5 ? 0 : cell < 8 ? 1 : 2; }

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
    std::array<Mark, cellCount> cells_{};
    Seat turn_{Seat::First};
    int moves_{0};
    turn_based::Outcome outcome_{};
    std::optional<Line> winningLine_;
};

} // namespace pyramid
