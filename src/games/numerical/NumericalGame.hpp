#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <optional>
#include <vector>

// Numerical Tic-Tac-Toe, ported from the console game (NumericalTicTacToe.cpp): Player 1
// places the odd numbers 1-9 and Player 2 the even numbers 2-8, each number at most once.
// The player whose placement completes a full line summing to exactly 15 wins; nine
// placements without such a line are a draw. Moves are encoded as cellIndex * 10 + number.
namespace numerical_ttt {

using turn_based::MoveId;
using turn_based::Seat;

class NumericalGame final : public turn_based::TurnBasedGame {
public:
    static constexpr int size = 3;
    using Line = std::array<int, 3>;

    NumericalGame();

    static MoveId encode(int cellIndex, int number) noexcept { return cellIndex * 10 + number; }
    static int cellOf(MoveId move) noexcept { return move / 10; }
    static int numberOf(MoveId move) noexcept { return move % 10; }
    static bool numberBelongsTo(int number, Seat seat) noexcept;

    int cell(int index) const { return cells_.at(static_cast<std::size_t>(index)); }  // 0 = empty
    bool numberUsed(int number) const noexcept;
    std::vector<int> availableNumbers(Seat seat) const;
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
    std::array<int, 9> cells_{};
    std::array<bool, 10> used_{};
    Seat turn_{Seat::First};
    int moves_{0};
    turn_based::Outcome outcome_{};
    std::optional<Line> winningLine_;
};

} // namespace numerical_ttt
