#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <optional>
#include <vector>

// Misere Tic-Tac-Toe, ported from the console game (Misere_Board.cpp): X and O alternate on a
// 3x3 grid, X first, and whoever completes a line of three of their own marks LOSES. Nine
// marks without a line are a draw. Moves are cell indices 0-8.
namespace misere {

using turn_based::MoveId;
using turn_based::Seat;

enum class Mark : char { Empty = '.', X = 'X', O = 'O' };

class MisereGame final : public turn_based::TurnBasedGame {
public:
    using Line = std::array<int, 3>;

    MisereGame();

    static Mark markFor(Seat seat) noexcept { return seat == Seat::First ? Mark::X : Mark::O; }

    Mark cell(int index) const { return cells_.at(static_cast<std::size_t>(index)); }
    // The completed (losing) line, once the game has ended that way.
    const std::optional<Line>& losingLine() const noexcept { return losingLine_; }

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
    std::optional<Line> losingLine_;
};

} // namespace misere
