#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <optional>
#include <vector>

// 4x4 Tic-Tac-Toe, ported from the console game (T4x4_Classes.cpp): each player starts with four
// tokens (top row O X O X, bottom row X O X O) and, X first, slides one of their tokens one cell
// up, down, left, or right into an empty cell. Three of your tokens in a row, column, or diagonal
// win. The console has no draw; the graphical version declares a draw when the player to move has
// no legal slide, a position the console never handled. Moves are encoded as from * 16 + to
// (cells numbered row * 4 + column).
namespace four_by_four {

using turn_based::MoveId;
using turn_based::Seat;

enum class Token : char { Empty = '.', X = 'X', O = 'O' };

class FourByFourGame final : public turn_based::TurnBasedGame {
public:
    static constexpr int size = 4;
    using Line = std::array<int, 3>;

    FourByFourGame();
    // A game starting from `layout` with `toMove` to move (for tests); reset() still restores the
    // standard opening.
    FourByFourGame(const std::array<Token, 16>& layout, Seat toMove);

    static Token tokenFor(Seat seat) noexcept { return seat == Seat::First ? Token::X : Token::O; }
    static MoveId encode(int from, int to) noexcept { return from * 16 + to; }
    static int fromOf(MoveId move) noexcept { return move / 16; }
    static int toOf(MoveId move) noexcept { return move % 16; }

    Token cell(int index) const { return cells_.at(static_cast<std::size_t>(index)); }
    // Empty cells the token at `from` may slide to, when it belongs to the player to move.
    std::vector<int> destinationsFrom(int from) const;
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

    // The console's static evaluation from `computer`'s point of view (exposed for tests).
    int evaluate(Token computer) const;

private:
    std::array<Token, 16> cells_{};
    Seat turn_{Seat::First};
    int moves_{0};
    turn_based::Outcome outcome_{};
    std::optional<Line> winningLine_;
};

} // namespace four_by_four
