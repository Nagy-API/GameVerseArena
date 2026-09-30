#include "NumericalGame.hpp"
#include "TurnBasedTestSupport.hpp"

#include <algorithm>
#include <set>

using numerical_ttt::NumericalGame;
using turn_based::Seat;
using turn_based_test::check;

namespace {
void play(NumericalGame& game, int cell, int number)
{
    check(game.play(NumericalGame::encode(cell, number)),
          "setup move cell " + std::to_string(cell) + " number " + std::to_string(number) + " is legal");
}

void testRules()
{
    NumericalGame game;
    check(game.availableNumbers(Seat::First) == std::vector<int>{1, 3, 5, 7, 9}, "Player 1 owns the odd numbers");
    check(game.availableNumbers(Seat::Second) == std::vector<int>{2, 4, 6, 8}, "Player 2 owns the even numbers");
    check(game.legalMoves().size() == 45, "the opening has 9 cells x 5 odd numbers");
    check(!game.play(NumericalGame::encode(0, 2)), "Player 1 cannot place an even number");
    check(!game.play(NumericalGame::encode(9, 1)) && !game.play(NumericalGame::encode(0, 0)) &&
              !game.play(NumericalGame::encode(0, 10)),
          "cells outside 0-8 and numbers outside 1-9 are illegal");
    play(game, 4, 5);
    check(game.currentSeat() == Seat::Second && game.movesPlayed() == 1, "turn passes to Player 2");
    check(!game.play(NumericalGame::encode(4, 2)), "an occupied cell is illegal");
    check(!game.play(NumericalGame::encode(0, 5)) && !game.play(NumericalGame::encode(0, 3)),
          "used and wrong-parity numbers are illegal for Player 2");
    play(game, 0, 2);
    check(!game.play(NumericalGame::encode(1, 5)), "a number can be used only once");
    check(game.availableNumbers(Seat::First) == std::vector<int>{1, 3, 7, 9}, "used numbers leave the pool");
}

void testWinsAndDraws()
{
    // Player 2 completes 1 + 6 + 8 = 15 on the top row: the mover wins (the console's GameManager
    // credits the player who just moved).
    NumericalGame game;
    play(game, 0, 1);
    play(game, 1, 6);
    play(game, 3, 3);
    play(game, 2, 8);
    check(game.outcome().status == turn_based::OutcomeStatus::Won && game.outcome().winner == Seat::Second,
          "the player who completes a 15-line wins, even if it is Player 2");
    check(game.winningLine() && *game.winningLine() == NumericalGame::Line{0, 1, 2}, "the winning line is reported");
    check(!game.play(NumericalGame::encode(4, 5)) && game.legalMoves().empty(), "moves are locked after a win");

    // 9 2 6 / . 5 . / . . 1: the top row sums to 17, so only the diagonal 9 + 5 + 1 makes 15.
    NumericalGame diagonal;
    play(diagonal, 0, 9);
    play(diagonal, 1, 2);
    play(diagonal, 4, 5);
    play(diagonal, 2, 6);
    check(!diagonal.outcome().finished(), "a full row that sums to 17 does not win");
    play(diagonal, 8, 1);
    check(diagonal.outcome().winner == Seat::First && *diagonal.winningLine() == NumericalGame::Line{0, 4, 8},
          "a diagonal summing to 15 wins");

    // A full line that does not add up to 15 does not win.
    NumericalGame full;
    play(full, 0, 1);
    play(full, 1, 2);
    play(full, 2, 3);
    check(!full.outcome().finished() && full.currentSeat() == Seat::Second,
          "a full line that does not sum to 15 does not end the game");
}

void testTrueDraw()
{
    // Find a genuine draw by search: play fixed orders until nine placements end without 15.
    const std::vector<int> odd{1, 3, 5, 7, 9};
    const std::vector<int> even{2, 4, 6, 8};
    // Layout (row-major): 1 2 3 / 4 5 7 / 6 9 8 -> rows 6, 16, 23; cols 11, 16, 18; diags 14, 14.
    const std::array<int, 9> layout{1, 2, 3, 4, 5, 7, 6, 9, 8};
    NumericalGame game;
    // Alternate placements: Player 1 needs odd numbers and Player 2 even numbers.
    std::vector<int> oddCells;
    std::vector<int> evenCells;
    for (int cell = 0; cell < 9; ++cell) (layout[static_cast<std::size_t>(cell)] % 2 ? oddCells : evenCells).push_back(cell);
    check(oddCells.size() == 5 && evenCells.size() == 4, "fixture layout uses five odd and four even numbers");
    for (std::size_t step = 0; step < 9; ++step) {
        const int cell = step % 2 == 0 ? oddCells[step / 2] : evenCells[step / 2];
        play(game, cell, layout[static_cast<std::size_t>(cell)]);
    }
    check(game.outcome().status == turn_based::OutcomeStatus::Draw && !game.winningLine(),
          "nine placements without a 15-line are a draw");
    check(game.movesPlayed() == 9 && game.legalMoves().empty(), "a drawn board has no legal moves");
    game.reset();
    check(game.movesPlayed() == 0 && game.availableNumbers(Seat::First).size() == 5 && game.cell(4) == 0,
          "reset clears cells and returns every number");
}

void testComputer()
{
    std::set<turn_based::MoveId> seen;
    for (std::uint32_t seed = 1; seed <= 200; ++seed) {
        NumericalGame game;
        std::mt19937 random(seed);
        turn_based::CancelToken cancel;
        const auto move = game.chooseComputerMove(random, cancel);
        check(move && game.isLegal(*move), "the computer's random move is legal");
        if (move) seen.insert(*move);
    }
    check(seen.size() > 20, "the computer's random choice covers many (cell, number) pairs");
    NumericalGame a;
    NumericalGame b;
    std::mt19937 first(42);
    std::mt19937 second(42);
    turn_based::CancelToken cancel;
    check(a.chooseComputerMove(first, cancel) == b.chooseComputerMove(second, cancel),
          "the same seed produces the same computer move");
}
} // namespace

int main()
{
    testRules();
    testWinsAndDraws();
    testTrueDraw();
    testComputer();
    turn_based_test::checkContract("Numerical", [] { return std::make_unique<NumericalGame>(); }, 9);
    return turn_based_test::finish("Numerical Tic-Tac-Toe");
}
