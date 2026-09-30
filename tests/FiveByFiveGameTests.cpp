#include "FiveByFiveGame.hpp"
#include "TurnBasedTestSupport.hpp"

using five_by_five::FiveByFiveGame;
using five_by_five::Mark;
using turn_based::Seat;
using turn_based_test::check;

namespace {
void play(FiveByFiveGame& game, int cell)
{
    check(game.play(cell), "setup move at cell " + std::to_string(cell) + " is legal");
}

void testRulesAndCounting()
{
    FiveByFiveGame game;
    check(game.legalMoves().size() == 25, "all 25 cells are open at the start");
    play(game, 0);
    check(game.cell(0) == Mark::X && game.currentSeat() == Seat::Second, "X moves first and the turn passes");
    check(!game.play(0) && !game.play(25) && !game.play(-3), "occupied and out-of-range cells are illegal");

    // X on the top row 0-3 (with O elsewhere): runs 0-1-2 and 1-2-3 overlap and both count.
    for (const int cell : {10, 1, 11, 2, 12, 3}) play(game, cell);
    check(game.triples(Mark::X) == 2, "overlapping runs of three each score");
    check(game.triples(Mark::O) == 1, "O's run 10-11-12 scores one");
    check(game.triplesOf(Mark::X).size() == 2 && game.scores()->first == 2 && game.scores()->second == 1,
          "scores mirror the triple counts");
    check(!game.outcome().finished(), "the game does not end on a triple");
}

void testDiagonalsCount()
{
    FiveByFiveGame game;
    // X: 0, 6, 12 runs down-right.
    for (const int cell : {0, 1, 6, 2, 12, 3}) play(game, cell);
    check(game.triples(Mark::X) == 1, "a down-right diagonal run scores");
    FiveByFiveGame anti;
    for (const int cell : {20, 0, 16, 1, 12, 2}) play(anti, cell);
    check(anti.triples(Mark::X) == 1, "an up-right diagonal run (20-16-12) scores");
}

void testEndAfterTwentyFourMoves()
{
    FiveByFiveGame game;
    for (int move = 0; move < 23; ++move) {
        const auto options = game.legalMoves();
        game.play(options.front());
    }
    check(!game.outcome().finished() && game.movesPlayed() == 23, "the game continues through move 23");
    game.play(game.legalMoves().front());
    check(game.outcome().finished() && game.movesPlayed() == 24, "the game ends after exactly 24 moves");
    int empty = 0;
    for (int cell = 0; cell < 25; ++cell) if (game.cell(cell) == Mark::Empty) ++empty;
    check(empty == 1, "exactly one cell stays empty");
    const int x = game.triples(Mark::X);
    const int o = game.triples(Mark::O);
    if (x > o) check(game.outcome().winner == Seat::First, "more X triples: X wins");
    else if (o > x) check(game.outcome().winner == Seat::Second, "more O triples: O wins");
    else check(game.outcome().status == turn_based::OutcomeStatus::Draw, "equal triples: draw");
    check(game.legalMoves().empty() && !game.play(24), "the finished game refuses moves, even on the empty cell");
    game.reset();
    check(game.movesPlayed() == 0 && game.triples(Mark::X) == 0 && game.cell(0) == Mark::Empty, "reset clears the board");
}

void testComputerPriorities()
{
    std::mt19937 random(3);
    turn_based::CancelToken cancel;
    FiveByFiveGame opening;
    check(opening.chooseComputerMove(random, cancel) == turn_based::MoveId{12}, "the computer opens in the center");

    FiveByFiveGame corner;
    play(corner, 12);
    check(corner.chooseComputerMove(random, cancel) == turn_based::MoveId{0}, "with the center taken, it takes a corner");

    // X (computer) holds 5 and 6: cell 7 completes 5-6-7.
    FiveByFiveGame win;
    for (const int cell : {5, 20, 6, 21}) play(win, cell);
    check(win.chooseComputerMove(random, cancel) == turn_based::MoveId{7}, "it completes its own three first");

    // O (computer) has no three to make, so it blocks X's only threat: 7 completes 5-6-7.
    FiveByFiveGame block;
    for (const int cell : {5, 20, 6}) play(block, cell);
    check(block.chooseComputerMove(random, cancel) == turn_based::MoveId{7}, "it blocks the opponent's three");

    // X already holds a three (0-1-2). O holds 20, 21, and 12, so both 16 (20-16-12, up-right)
    // and 22 (20-21-22) make a new three for O; the scan finds 16 first (as in the console, since
    // O itself holds no three yet).
    FiveByFiveGame established;
    for (const int cell : {0, 20, 1, 21, 2, 12, 24}) play(established, cell);
    check(established.chooseComputerMove(random, cancel) == turn_based::MoveId{16},
          "it still makes a new three after a three already exists");

    // The computer (X) already holds 0-1-2 and can make a new three at 22 (20-21-22). The
    // console's whole-board test passed on the first free cell, 7, instead.
    FiveByFiveGame ownThree;
    for (const int cell : {0, 3, 1, 4, 2, 5, 20, 6, 21, 8}) play(ownThree, cell);
    check(ownThree.chooseComputerMove(random, cancel) == turn_based::MoveId{22},
          "holding a three already, it still makes a new one instead of taking the first free cell");

    // Blocking also still works after a three exists: X holds 20-21-22 and threatens 23
    // (21-22-23); O (0 and 3) has no three to make, so it blocks at 23 rather than taking the
    // first free cell, 1, as the console would once the opponent held a three.
    FiveByFiveGame blockLater;
    for (const int cell : {20, 0, 21, 3, 22}) play(blockLater, cell);
    check(blockLater.chooseComputerMove(random, cancel) == turn_based::MoveId{23},
          "it still blocks a new three after a three already exists");

    // Its own new three comes before blocking: O (10, 11) can make 10-11-12 while X (0-1-2)
    // threatens 3. The console agrees here, because O itself holds no three yet.
    FiveByFiveGame ownFirst;
    for (const int cell : {0, 10, 1, 11, 2, 23, 18}) play(ownFirst, cell);
    check(ownFirst.chooseComputerMove(random, cancel) == turn_based::MoveId{12},
          "its own three still comes before blocking");
}
} // namespace

int main()
{
    testRulesAndCounting();
    testDiagonalsCount();
    testEndAfterTwentyFourMoves();
    testComputerPriorities();
    turn_based_test::checkContract("5x5", [] { return std::make_unique<FiveByFiveGame>(); }, 24);
    return turn_based_test::finish("5x5 Tic-Tac-Toe");
}
