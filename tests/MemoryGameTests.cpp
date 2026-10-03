#include "MemoryGame.hpp"
#include "TurnBasedTestSupport.hpp"

using memory_xo::Mark;
using memory_xo::MemoryGame;
using turn_based::Seat;
using turn_based_test::check;

namespace {
void play(MemoryGame& game, int cell)
{
    check(game.play(cell), "setup move at cell " + std::to_string(cell) + " is legal");
}

void testRules()
{
    MemoryGame game;
    play(game, 4);
    check(game.cell(4) == Mark::X && game.currentSeat() == Seat::Second, "X's hidden mark is recorded and the turn passes");
    check(!game.play(4) && game.currentSeat() == Seat::Second && game.movesPlayed() == 1,
          "choosing a taken cell is refused and the same player chooses again");
    check(!game.play(9) && !game.play(-1), "cells outside the grid are illegal");
}

void testResults()
{
    MemoryGame win;
    for (const int cell : {0, 3, 1, 4}) play(win, cell);
    play(win, 2);
    check(win.outcome().winner == Seat::First && win.winningLine() == MemoryGame::Line{0, 1, 2}, "three in a row wins");

    // X O X / X O O / O X X: no line.
    MemoryGame draw;
    for (const int cell : {0, 1, 2, 4, 3, 5, 7, 6, 8}) play(draw, cell);
    check(draw.outcome().status == turn_based::OutcomeStatus::Draw, "nine marks without a line are a draw");
    draw.reset();
    check(draw.movesPlayed() == 0 && draw.cell(0) == Mark::Empty, "reset clears the hidden board");
}

void testComputer()
{
    std::mt19937 random(1);
    turn_based::CancelToken cancel;
    MemoryGame game;
    check(game.chooseComputerMove(random, cancel) == turn_based::MoveId{0}, "the computer takes the first free cell");
    for (const int cell : {0, 1, 4}) play(game, cell);
    check(game.chooseComputerMove(random, cancel) == turn_based::MoveId{2},
          "it reads the hidden board and skips taken cells, row by row");
}
} // namespace

int main()
{
    testRules();
    testResults();
    testComputer();
    turn_based_test::checkContract("Memory", [] { return std::make_unique<MemoryGame>(); }, 9);
    return turn_based_test::finish("Memory XO");
}
