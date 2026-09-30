#include "DiamondGame.hpp"
#include "TurnBasedTestSupport.hpp"

#include <algorithm>
#include <utility>

using diamond::DiamondGame;
using diamond::Mark;
using turn_based::Seat;
using turn_based_test::check;

namespace {
using Cell = std::pair<int, int>;

void play(DiamondGame& game, Cell cell)
{
    check(game.play(DiamondGame::moveAt(cell.first, cell.second)),
          "setup move (" + std::to_string(cell.first) + "," + std::to_string(cell.second) + ") is legal");
}

// Alternates X's cells with O's cells, X first.
void playAlternating(DiamondGame& game, const std::vector<Cell>& x, const std::vector<Cell>& o)
{
    for (std::size_t index = 0; index < x.size(); ++index) {
        play(game, x[index]);
        if (index < o.size()) play(game, o[index]);
    }
}

void testBoard()
{
    int cells = 0;
    for (int row = 0; row < 7; ++row) {
        for (int column = 0; column < 7; ++column) {
            if (DiamondGame::onBoard(row, column)) ++cells;
        }
    }
    check(cells == DiamondGame::cellCount, "the diamond has 25 cells");
    check(DiamondGame::onBoard(0, 3) && DiamondGame::onBoard(3, 0) && DiamondGame::onBoard(3, 6) &&
              !DiamondGame::onBoard(0, 0) && !DiamondGame::onBoard(1, 1) && !DiamondGame::onBoard(6, 6),
          "rows hold 1, 3, 5, 7, 5, 3, and 1 cells");
    DiamondGame game;
    check(game.legalMoves().size() == 25, "all 25 cells are open at the start");
    check(!game.play(DiamondGame::moveAt(0, 0)) && !game.play(-1) && !game.play(49), "cells outside the diamond are illegal");
}

void testWinningPair()
{
    // X: a line of four across row 3 (columns 1-4) and a line of three down column 2 (rows 2-4).
    DiamondGame game;
    playAlternating(game, {{3, 1}, {3, 2}, {3, 3}, {2, 2}, {4, 2}}, {{0, 3}, {6, 3}, {5, 3}, {5, 4}, {1, 4}});
    check(!game.outcome().finished(), "two lines of three are not enough");
    play(game, {3, 4});
    check(game.outcome().winner == Seat::First, "a line of four and a line of three in different directions win");
    const auto& lines = game.winningLines();
    check(lines && lines->four.size() == 4 && lines->three.size() == 3, "the winning lines are reported");
    check(!game.play(DiamondGame::moveAt(2, 3)), "no moves are accepted after a win");
}

void testSameDirectionDoesNotWin()
{
    // X: four across row 3 (columns 1-4) and three across row 2 (columns 1-3): both horizontal.
    DiamondGame game;
    playAlternating(game, {{3, 1}, {3, 2}, {3, 3}, {3, 4}, {2, 1}, {2, 3}, {2, 2}},
                    {{0, 3}, {6, 3}, {5, 3}, {5, 4}, {1, 4}, {5, 2}});
    check(!game.outcome().finished(), "a three and a four in the same direction do not win");
}

void testLongLineIsNotAFour()
{
    // X: five across row 3 plus three down column 2: a line of five is not a line of four.
    DiamondGame game;
    playAlternating(game, {{3, 1}, {3, 2}, {3, 4}, {3, 5}, {2, 2}, {4, 2}, {3, 3}},
                    {{0, 3}, {6, 3}, {5, 3}, {5, 4}, {1, 4}, {1, 2}});
    check(!game.outcome().finished(), "a line of five does not count as a line of four");
}

void testConsoleScanOrderCaseWins()
{
    // X: four down column 3 (rows 1-4), three down column 1 (rows 2-4), and three across row 3.
    // The console met the column of four first and then the column-1 three in the same direction,
    // remembered only those two, and missed the valid pair; the graphical version finds it.
    DiamondGame game;
    playAlternating(game, {{2, 1}, {3, 1}, {4, 1}, {3, 2}, {1, 3}, {2, 3}, {4, 3}, {3, 3}},
                    {{0, 3}, {6, 3}, {5, 3}, {2, 5}, {4, 5}, {1, 2}, {5, 2}});
    check(game.outcome().winner == Seat::First, "a valid three-and-four pair wins whatever the scan order");
}

void testDraw()
{
    // A full diamond in which no line of either mark is longer than three: nobody can ever hold a
    // line of four, so every order of these moves fills the board without a winner.
    const std::vector<Cell> x{{1, 3}, {2, 2}, {2, 4}, {3, 0}, {3, 2}, {3, 5}, {3, 6},
                              {4, 3}, {4, 4}, {4, 5}, {5, 2}, {5, 3}, {5, 4}};
    const std::vector<Cell> o{{0, 3}, {1, 2}, {1, 4}, {2, 1}, {2, 3}, {2, 5},
                              {3, 1}, {3, 3}, {3, 4}, {4, 1}, {4, 2}, {6, 3}};
    DiamondGame game;
    playAlternating(game, x, o);
    check(game.outcome().status == turn_based::OutcomeStatus::Draw && game.movesPlayed() == 25 && !game.winningLines(),
          "a full diamond without a winner is a draw");
    game.reset();
    check(game.movesPlayed() == 0 && game.cell(3, 3) == Mark::Empty, "reset empties the diamond");
}

void testComputer()
{
    std::mt19937 random(1);
    turn_based::CancelToken cancel;
    DiamondGame empty;
    check(empty.chooseComputerMove(random, cancel) == DiamondGame::moveAt(3, 3), "the computer opens in the centre");

    // X holds three across row 3 and three down column 2: extending either to four wins, at
    // (1,2), (3,0), (3,4), or (5,2). Row by row, (1,2) comes first.
    DiamondGame win;
    playAlternating(win, {{3, 1}, {3, 2}, {3, 3}, {2, 2}, {4, 2}}, {{0, 3}, {6, 3}, {5, 3}, {5, 4}, {1, 4}});
    check(win.chooseComputerMove(random, cancel) == DiamondGame::moveAt(1, 2),
          "the computer takes the first winning cell in scan order");

    // O must stop one of X's winning cells; the block takes the first in scan order.
    DiamondGame block;
    playAlternating(block, {{3, 1}, {3, 2}, {3, 3}, {2, 2}, {4, 2}}, {{0, 3}, {6, 3}, {5, 3}, {5, 4}});
    check(block.currentSeat() == Seat::Second, "block fixture has O to move");
    check(block.chooseComputerMove(random, cancel) == DiamondGame::moveAt(1, 2), "the computer blocks a winning cell");

    // Positions checked against the console computer (with the every-line win rule).
    DiamondGame reply;
    play(reply, {3, 3});
    check(reply.chooseComputerMove(random, cancel) == DiamondGame::moveAt(2, 3),
          "after X takes the centre, O takes the first best-scoring cell (2,3)");

    // O to move can win at (2,3) or block X's win at (6,3): winning comes first.
    DiamondGame winFirst;
    playAlternating(winFirst, {{1, 3}, {5, 3}, {5, 2}, {4, 3}, {3, 0}, {4, 1}}, {{3, 5}, {3, 6}, {3, 3}, {1, 2}, {3, 4}});
    check(winFirst.currentSeat() == Seat::Second && !winFirst.outcome().finished(), "win-first fixture has O to move");
    check(winFirst.chooseComputerMove(random, cancel) == DiamondGame::moveAt(2, 3), "the computer wins before it blocks");

    DiamondGame scoring;
    play(scoring, {3, 3});
    check(scoring.evaluate(Mark::X) == 0, "a single mark scores nothing");
    play(scoring, {0, 3});
    play(scoring, {3, 4});
    // A line of two is counted from both cells in both directions: 4 x 25.
    check(scoring.evaluate(Mark::X) == 100, "the evaluation counts a pair four times, as the console did");
}
} // namespace

int main()
{
    testBoard();
    testWinningPair();
    testSameDirectionDoesNotWin();
    testLongLineIsNotAFour();
    testConsoleScanOrderCaseWins();
    testDraw();
    testComputer();
    turn_based_test::checkContract("Diamond", [] { return std::make_unique<DiamondGame>(); }, 25);
    return turn_based_test::finish("Diamond");
}
