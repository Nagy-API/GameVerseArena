#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <optional>
#include <vector>

// Ultimate XO, ported from the console game (Ultimate_TTT.cpp and Ultimate_TTT_AI.cpp): the board
// holds nine small tic-tac-toe boards, X first. Winning a small board claims that square of the big
// board; a full small board without a line becomes a tie square. After a move, the next player
// must play in the same small board while it is still open; once it is won or full, the next
// player may choose any open board. Three claimed squares in a line on the big board win, and if
// every small board closes without such a line the game is a draw. Moves are encoded as
// board * 9 + cell, both numbered row by row from 0.
//
// The computer is the console's: alpha-beta minimax four moves deep (its own move plus three
// replies) with the console's evaluation of the big board and of the open small boards.
namespace ultimate_xo {

using turn_based::MoveId;
using turn_based::Seat;

enum class Mark : char { Empty = '.', X = 'X', O = 'O' };
enum class Square : char { Open = '.', X = 'X', O = 'O', Tie = 'T' };

class UltimateGame final : public turn_based::TurnBasedGame {
public:
    using Line = std::array<int, 3>;

    UltimateGame();

    static Mark markFor(Seat seat) noexcept { return seat == Seat::First ? Mark::X : Mark::O; }
    static MoveId encode(int board, int cell) noexcept { return board * 9 + cell; }
    static int boardOf(MoveId move) noexcept { return move / 9; }
    static int cellOf(MoveId move) noexcept { return move % 9; }

    Mark cell(int board, int cell) const { return cells_.at(static_cast<std::size_t>(board * 9 + cell)); }
    Square square(int board) const { return squares_.at(static_cast<std::size_t>(board)); }
    // The small board the player to move must use, or nullopt for a free choice.
    std::optional<int> forcedBoard() const noexcept;
    // The winning line of small-board squares on the big board.
    const std::optional<Line>& winningLine() const noexcept { return winningLine_; }
    // The line that won a small board, for drawing.
    std::optional<Line> smallBoardLine(int board) const;

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
    std::array<Mark, 81> cells_{};
    std::array<Square, 9> squares_{};
    int forced_{-1};
    Seat turn_{Seat::First};
    int moves_{0};
    turn_based::Outcome outcome_{};
    std::optional<Line> winningLine_;
};

} // namespace ultimate_xo
