#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

// Obstacle Tic-Tac-Toe, ported from the console game (Obstacle.cpp): X and O alternate on a 6x6
// grid, X first. After every second move, two obstacles appear on random empty cells and can never
// be used. Four in a row horizontally, vertically, or diagonally wins. Moves are cell indices 0-35.
//
// The console only declared a draw after 36 moves, which obstacles make impossible (the game then
// waited for a move that could not exist); the graphical version declares a draw as soon as no
// empty cell is left. The obstacles come from the game's own random generator, seeded when the
// game is created, so a game is reproducible from its seed.
namespace obstacle {

using turn_based::MoveId;
using turn_based::Seat;

enum class Cell : char { Empty = '.', X = 'X', O = 'O', Blocked = '#' };

class ObstacleGame final : public turn_based::TurnBasedGame {
public:
    static constexpr int size = 6;
    using Line = std::vector<int>;

    explicit ObstacleGame(std::uint32_t seed = 1);

    static Cell markFor(Seat seat) noexcept { return seat == Seat::First ? Cell::X : Cell::O; }

    Cell cell(int index) const { return cells_.at(static_cast<std::size_t>(index)); }
    // Cells that became obstacles after the most recent move (for animation).
    const std::vector<int>& lastObstacles() const noexcept { return lastObstacles_; }
    const std::optional<Line>& winningLine() const noexcept { return winningLine_; }

    std::unique_ptr<turn_based::TurnBasedGame> clone() const override;
    // Clears the grid; later obstacles continue the same random sequence, so a rematch differs.
    void reset() override;
    Seat currentSeat() const override { return turn_; }
    turn_based::Outcome outcome() const override { return outcome_; }
    std::vector<MoveId> legalMoves() const override;
    bool isLegal(MoveId move) const override;
    bool play(MoveId move) override;
    int movesPlayed() const override { return moves_; }
    std::optional<MoveId> chooseComputerMove(std::mt19937& random, const turn_based::CancelToken& cancel) const override;

private:
    void addObstacles(int count);

    std::array<Cell, size * size> cells_{};
    std::mt19937 obstacles_;
    std::vector<int> lastObstacles_;
    Seat turn_{Seat::First};
    int moves_{0};
    turn_based::Outcome outcome_{};
    std::optional<Line> winningLine_;
};

} // namespace obstacle
