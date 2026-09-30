#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <optional>
#include <vector>

// Four-in-a-Row, ported from the console game (FourInRow.cpp): discs drop into a grid of 6 rows
// and 7 columns, X first; four in a row horizontally, vertically, or diagonally wins, and a full
// grid is a draw. Moves are column numbers 0-6.
//
// The computer keeps the console strategy (centre opening, immediate win, double threat, block,
// block a double threat, then iterative-deepening alpha-beta search at depths 4, 6, and 8 with
// the console's evaluation and move ordering), with three documented differences: the search
// stops after a fixed number of positions instead of 1.5 seconds of wall-clock time (so it plays
// the same way on every computer), it has no transposition cache (the console's reused scores
// across different depths and search windows), and it only counts a four for the player whose
// disc was just dropped (the console could report a false win for the other player).
namespace four_in_a_row {

using turn_based::MoveId;
using turn_based::Seat;

enum class Disc : char { Empty = '.', X = 'X', O = 'O' };

class FourInRowGame final : public turn_based::TurnBasedGame {
public:
    static constexpr int rows = 6;
    static constexpr int columns = 7;
    // Positions the computer may examine per move (see above). Enough for the depth-8 search to
    // finish in practically every position; a search that runs out keeps its depth-6 choice.
    static constexpr long long defaultSearchBudget = 400000;
    using Line = std::vector<int>;  // the winning run's cells, row * columns + column, row 0 at the top

    // How the computer chose its move (for tests).
    struct SearchReport {
        std::optional<MoveId> move;
        const char* rule{""};    // the console priority that decided: "opening", "win", "fork", "block",
                                 // "block fork", or "search"
        long long nodes{0};      // positions the deepening search examined
        int completedDepth{0};   // deepest search depth that finished within the budget
    };

    explicit FourInRowGame(long long searchBudget = defaultSearchBudget);

    static Disc discFor(Seat seat) noexcept { return seat == Seat::First ? Disc::X : Disc::O; }
    static int indexOf(int row, int column) noexcept { return row * columns + column; }

    Disc cell(int row, int column) const { return cells_.at(static_cast<std::size_t>(indexOf(row, column))); }
    // The row a disc dropped into `column` lands in, or -1 when the column is full or invalid.
    int landingRow(int column) const;
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

    SearchReport analyse(const turn_based::CancelToken& cancel) const;

private:
    std::array<Disc, rows * columns> cells_{};
    Seat turn_{Seat::First};
    int moves_{0};
    turn_based::Outcome outcome_{};
    std::optional<Line> winningLine_;
    long long searchBudget_;
};

} // namespace four_in_a_row
