#include "PyramidGame.hpp"
#include "TurnBasedTestSupport.hpp"

#include <set>

using pyramid::Mark;
using pyramid::PyramidGame;
using turn_based::Seat;
using turn_based_test::check;

namespace {
void play(PyramidGame& game, int cell)
{
    check(game.play(cell), "setup move at cell " + std::to_string(cell) + " is legal");
}

// X takes `line`; O answers on `oCells` (which must not complete a line first).
void expectLineWins(const PyramidGame::Line& line, std::initializer_list<int> oCells, const std::string& name)
{
    PyramidGame game;
    auto o = oCells.begin();
    for (std::size_t index = 0; index < line.size(); ++index) {
        play(game, line[index]);
        if (index + 1 < line.size() && o != oCells.end()) play(game, *o++);
    }
    check(game.outcome().winner == Seat::First && game.winningLine() == line, name + " wins");
}

void testLines()
{
    const auto& lines = PyramidGame::lines();
    check(lines.size() == 7, "the pyramid has seven winning lines");
    std::set<int> used;
    for (const auto& line : lines) used.insert(line.begin(), line.end());
    check(used.size() == 9, "every cell lies on at least one line");

    expectLineWins({0, 1, 2}, {5, 7}, "the left bottom-row line");
    expectLineWins({1, 2, 3}, {5, 7}, "the middle bottom-row line");
    expectLineWins({2, 3, 4}, {5, 7}, "the right bottom-row line");
    expectLineWins({5, 6, 7}, {0, 4}, "the middle row");
    expectLineWins({2, 6, 8}, {0, 4}, "the centre column");
    expectLineWins({0, 5, 8}, {1, 3}, "the left sloped edge");
    expectLineWins({4, 7, 8}, {1, 3}, "the right sloped edge");

    // 3, 7, 8 would be a sloped line on a regular triangle but is not one of the console's lines.
    PyramidGame notALine;
    for (const int cell : {3, 0, 7, 1, 8}) play(notALine, cell);
    check(!notALine.outcome().finished(), "cells 3, 7, and 8 do not form a line");
}

void testDrawAndRules()
{
    PyramidGame game;
    check(game.legalMoves().size() == 9 && game.currentSeat() == Seat::First, "X opens on an empty pyramid");
    check(!game.play(9) && !game.play(-1), "cells outside the pyramid are illegal");
    // X: 0, 1, 3, 4, 6 and O: 2, 5, 7, 8 leave no line for either player.
    for (const int cell : {0, 2, 1, 5, 3, 7, 4, 8}) play(game, cell);
    check(!game.outcome().finished(), "eight marks without a line continue");
    check(!game.play(0), "an occupied cell is illegal");
    play(game, 6);
    check(game.outcome().status == turn_based::OutcomeStatus::Draw && !game.winningLine(), "a full pyramid without a line is a draw");
    game.reset();
    check(game.movesPlayed() == 0 && game.cell(8) == Mark::Empty, "reset empties the pyramid");
    check(PyramidGame::rowOf(0) == 0 && PyramidGame::rowOf(5) == 1 && PyramidGame::rowOf(8) == 2, "cells know their row");
}

void testComputer()
{
    std::mt19937 random(1);
    turn_based::CancelToken cancel;
    PyramidGame empty;
    check(empty.chooseComputerMove(random, cancel) == turn_based::MoveId{0},
          "with nothing to win or block, the computer takes the first free cell from the bottom row");

    PyramidGame win;
    for (const int cell : {0, 3, 5, 4}) play(win, cell);  // X: 0, 5 (the left edge); O: 3, 4
    check(win.chooseComputerMove(random, cancel) == turn_based::MoveId{8}, "the computer completes its own line first");

    PyramidGame block;
    for (const int cell : {2, 0, 6}) play(block, cell);  // X: 2, 6 (the centre column); O: 0
    check(block.chooseComputerMove(random, cancel) == turn_based::MoveId{8}, "the computer blocks the opponent's line");

    // Both a win and a block exist: the win comes first.
    PyramidGame both;
    for (const int cell : {5, 0, 6, 1}) play(both, cell);  // X: 5, 6 (middle row); O: 0, 1 (bottom row)
    check(both.chooseComputerMove(random, cancel) == turn_based::MoveId{7}, "winning beats blocking");
}
} // namespace

int main()
{
    testLines();
    testDrawAndRules();
    testComputer();
    turn_based_test::checkContract("Pyramid", [] { return std::make_unique<PyramidGame>(); }, 9);
    return turn_based_test::finish("Pyramid Tic-Tac-Toe");
}
