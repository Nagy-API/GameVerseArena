#include "SusGame.hpp"
#include "TurnBasedTestSupport.hpp"

using sus_game::SusGame;
using turn_based::Seat;
using turn_based_test::check;

namespace {
void play(SusGame& game, int cell)
{
    check(game.play(cell), "setup move at cell " + std::to_string(cell) + " is legal");
}

void testPlacementAndLetters()
{
    SusGame game;
    check(SusGame::letterFor(Seat::First) == 'S' && SusGame::letterFor(Seat::Second) == 'U',
          "Player 1 places S and Player 2 places U, as in the console game");
    play(game, 0);
    check(game.cell(0) == 'S' && game.currentSeat() == Seat::Second, "Player 1 places an S and the turn passes");
    check(!game.play(0), "an occupied cell is illegal");
    check(!game.play(9) && !game.play(-1), "cells outside the grid are illegal");
    play(game, 1);
    check(game.cell(1) == 'U', "Player 2 places a U");
}

void testScoring()
{
    // S U S along the top row, completed by Player 1's second S: one point for S.
    SusGame game;
    play(game, 0);  // S
    play(game, 1);  // U
    play(game, 2);  // S completes S-U-S
    check(game.score(Seat::First) == 1 && game.score(Seat::Second) == 0, "completing S-U-S scores for its placer");
    check(game.susLines().size() == 1 && game.susLines().front() == SusGame::Line{0, 1, 2},
          "the completed line is reported for highlighting");

    // A U completes S-U-S when placed in the middle: the point goes to U.
    SusGame middle;
    play(middle, 0);  // S
    play(middle, 8);  // U
    play(middle, 2);  // S
    play(middle, 1);  // U completes row S U S
    check(middle.score(Seat::Second) == 1 && middle.score(Seat::First) == 0,
          "the letter that completes the line earns the point, even a U");

    // The center lies on both diagonals: one U there can complete two S-U-S lines at once.
    // S: 0, 2, 6, 8 (scoring the top row and both outer columns); U: 1, 3, 5, then 4.
    SusGame multi;
    for (const int cell : {0, 1, 2, 3, 6, 5, 8}) play(multi, cell);
    check(multi.score(Seat::First) == 3 && multi.score(Seat::Second) == 0,
          "S scored the top row and both outer columns");
    play(multi, 4);
    check(multi.score(Seat::Second) == 2, "one U completing both diagonals earns two points");
}

void testOutcomes()
{
    // Row-major fill: S U S / U S U / S U S. S completes the top row, the left column, and at the
    // last move both the bottom row and the right column: S 4, U 0.
    SusGame game;
    for (const int cell : {0, 1, 2, 3, 4, 5, 6, 7}) play(game, cell);
    check(!game.outcome().finished(), "the game continues until all nine cells are filled");
    play(game, 8);
    check(game.movesPlayed() == 9 && game.outcome().finished(), "the game ends after nine placements");
    check(game.score(Seat::First) == 4 && game.score(Seat::Second) == 0, "S scores four lines, U none");
    check(game.outcome().winner == Seat::First, "the higher score wins");
    check(!game.play(0) && game.legalMoves().empty(), "moves are locked when the game is over");
    check(game.scores() && game.scores()->first == 4 && game.scores()->second == 0, "scores are reported per seat");

    // S S U / S S U / U U S contains no S-U-S at all: 0-0 is a draw.
    SusGame drawn;
    for (const int cell : {0, 2, 1, 5, 3, 6, 4, 7, 8}) play(drawn, cell);
    check(drawn.score(Seat::First) == 0 && drawn.score(Seat::Second) == 0 &&
              drawn.outcome().status == turn_based::OutcomeStatus::Draw,
          "equal scores draw");

    drawn.reset();
    check(drawn.movesPlayed() == 0 && drawn.score(Seat::First) == 0 && drawn.cell(4) == '.',
          "reset clears letters and scores");
}

void testComputerHeuristic()
{
    // With an S at 0 and a U at 1, S completes the row at 2: the offensive +100 dominates.
    SusGame game;
    play(game, 0);
    play(game, 1);
    std::mt19937 random(1);
    turn_based::CancelToken cancel;
    check(game.chooseComputerMove(random, cancel) == turn_based::MoveId{2},
          "the computer (S) completes an S-U-S line when it can");

    // Opening move for S: every cell scores 0 lines; center +30 beats corners +20 and edges +10.
    SusGame opening;
    check(opening.evaluateMove(4, 'S') == 30 && opening.evaluateMove(0, 'S') == 20 && opening.evaluateMove(1, 'S') == 10,
          "position scores follow the console: center 30, corner 20, edge 10");
    check(opening.chooseComputerMove(random, cancel) == turn_based::MoveId{4}, "the computer opens in the center");

    // U to move: S at 0 and S at 2 make the middle of the top row worth a point for U (+100),
    // and the console also adds 80 for a cell where S would score.
    SusGame blocking;
    play(blocking, 0);
    play(blocking, 4);
    play(blocking, 2);
    check(blocking.countSusThrough(1, 'U') == 1, "U at the top middle would complete S-U-S");
    check(blocking.chooseComputerMove(random, cancel) == turn_based::MoveId{1},
          "the computer (U) takes the cell that completes S-U-S");

    // Fixtures checked against the console AI: its defensive term (80 per S-U-S the opponent's
    // letter would complete) and its strict "first best cell" tie-breaking.
    SusGame afterCorner;
    play(afterCorner, 0);
    check(afterCorner.chooseComputerMove(random, cancel) == turn_based::MoveId{2},
          "after an opening S in a corner, U takes the first best-scoring cell (2)");
    SusGame defensive;
    for (const int cell : {3, 1, 0}) play(defensive, cell);
    check(defensive.chooseComputerMove(random, cancel) == turn_based::MoveId{2},
          "U weighs the S-U-S lines S could complete, as the console does");
}
} // namespace

int main()
{
    testPlacementAndLetters();
    testScoring();
    testOutcomes();
    testComputerHeuristic();
    turn_based_test::checkContract("SUS", [] { return std::make_unique<SusGame>(); }, 9);
    return turn_based_test::finish("SUS");
}
