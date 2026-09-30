#include "FourInRowGame.hpp"
#include "TurnBasedTestSupport.hpp"

#include <algorithm>
#include <chrono>
#include <future>
#include <string>
#include <thread>

using four_in_a_row::Disc;
using four_in_a_row::FourInRowGame;
using turn_based::Seat;
using turn_based_test::check;

namespace {
void play(FourInRowGame& game, int column)
{
    check(game.play(column), "setup drop in column " + std::to_string(column) + " is legal");
}

void playAll(FourInRowGame& game, std::initializer_list<int> columns)
{
    for (const int column : columns) play(game, column);
}

void testRules()
{
    FourInRowGame game;
    check(game.legalMoves().size() == 7 && game.landingRow(3) == 5, "every column is open and discs land on row 5");
    play(game, 3);
    check(game.cell(5, 3) == Disc::X && game.currentSeat() == Seat::Second, "X drops first and the turn passes");
    play(game, 3);
    check(game.cell(4, 3) == Disc::O && game.landingRow(3) == 3, "a second disc stacks on the first");
    for (int drop = 0; drop < 4; ++drop) play(game, 3);
    check(game.landingRow(3) == -1 && !game.isLegal(3) && !game.play(3), "a full column refuses more discs");
    check(game.legalMoves().size() == 6, "a full column is not a legal move");
    check(!game.play(7) && !game.play(-1), "columns outside 0-6 are illegal");
}

void testWins()
{
    FourInRowGame horizontal;
    playAll(horizontal, {0, 0, 1, 1, 2, 2});
    check(!horizontal.outcome().finished(), "three in a row does not end the game");
    play(horizontal, 3);
    check(horizontal.outcome().winner == Seat::First && horizontal.winningLine() &&
              *horizontal.winningLine() == FourInRowGame::Line{35, 36, 37, 38},
          "four across the bottom row wins and is reported");
    check(horizontal.legalMoves().empty() && !horizontal.play(4), "no drops are accepted after a win");

    FourInRowGame vertical;
    playAll(vertical, {0, 1, 0, 1, 0, 1, 0});
    check(vertical.outcome().winner == Seat::First && vertical.winningLine()->size() == 4,
          "four stacked in one column wins");

    // O builds the rising diagonal (5,0) (4,1) (3,2) (2,3).
    FourInRowGame diagonal;
    playAll(diagonal, {1, 0, 2, 1, 2, 2, 3, 3, 3, 4, 6});
    check(!diagonal.outcome().finished() && diagonal.currentSeat() == Seat::Second, "diagonal fixture is in progress");
    play(diagonal, 3);
    check(diagonal.outcome().winner == Seat::Second && diagonal.winningLine() &&
              *diagonal.winningLine() == FourInRowGame::Line{17, 23, 29, 35},
          "four on a rising diagonal wins for O");
}

void testDraw()
{
    // Rows alternate X X O O X X O and O O X X O O X from the bottom up: a full grid without four.
    FourInRowGame game;
    for (int row = 0; row < 6; ++row) playAll(game, {0, 2, 1, 3, 4, 6, 5});
    check(game.movesPlayed() == 42 && game.outcome().status == turn_based::OutcomeStatus::Draw && !game.winningLine(),
          "a full grid without four in a row is a draw");
    game.reset();
    check(game.movesPlayed() == 0 && game.cell(5, 0) == Disc::Empty && game.legalMoves().size() == 7,
          "reset empties the grid");
}

void testComputerPriorities()
{
    turn_based::CancelToken cancel;
    FourInRowGame opening;
    auto report = opening.analyse(cancel);
    check(report.move == 3 && std::string(report.rule) == "opening", "the computer opens in the centre column");
    play(opening, 0);
    report = opening.analyse(cancel);
    check(report.move == 3 && std::string(report.rule) == "opening", "moving second, its first disc also goes in the centre");

    FourInRowGame win;
    playAll(win, {0, 6, 1, 6, 2, 5});
    report = win.analyse(cancel);
    check(report.move == 3 && std::string(report.rule) == "win", "it completes its own four first");

    FourInRowGame block;
    playAll(block, {0, 6, 1, 6, 2});
    report = block.analyse(cancel);
    check(report.move == 3 && std::string(report.rule) == "block", "it blocks the opponent's four");

    // X on 2 and 3 of the bottom row: a drop in column 1 threatens both 0 and 4.
    FourInRowGame fork;
    playAll(fork, {2, 6, 3, 6});
    report = fork.analyse(cancel);
    check(report.move == 1 && std::string(report.rule) == "fork", "it makes a double threat when it can");

    FourInRowGame blockFork;
    playAll(blockFork, {2, 6, 3});
    report = blockFork.analyse(cancel);
    check(report.move == 1 && std::string(report.rule) == "block fork", "it takes the cell the opponent needs for a double threat");
}

void testSearch()
{
    turn_based::CancelToken cancel;
    FourInRowGame quiet;
    playAll(quiet, {3, 3, 2, 4});
    const auto started = std::chrono::steady_clock::now();
    const auto report = quiet.analyse(cancel);
    const auto elapsed = std::chrono::steady_clock::now() - started;
    check(report.move && quiet.isLegal(*report.move) && std::string(report.rule) == "search",
          "a quiet position is decided by the search");
    check(report.completedDepth == 8, "the default budget lets the depth-8 search finish");
    check(report.nodes <= FourInRowGame::defaultSearchBudget + 20000,
          "the search stops close to its fixed budget (" + std::to_string(report.nodes) + " positions)");
    check(elapsed < std::chrono::seconds(10), "one computer move takes well under ten seconds");

    FourInRowGame small(2000);
    playAll(small, {3, 3, 2, 4});
    const auto limited = small.analyse(cancel);
    check(limited.move && small.isLegal(*limited.move) && limited.completedDepth == 4,
          "a tiny budget still finishes depth 4 and plays a legal move");
    check(small.analyse(cancel).move == limited.move, "the budgeted search is deterministic");

    turn_based::CancelToken stop;
    stop.cancel();
    check(!quiet.analyse(stop).move, "a cancelled search returns no move");
}

// Positions whose search result depends on details of the console algorithm, checked against a
// copy of the console search with only the three documented changes applied.
void testSearchDetails()
{
    turn_based::CancelToken cancel;
    const auto expect = [&](std::initializer_list<int> columns, int expected, const std::string& what) {
        FourInRowGame game;
        playAll(game, columns);
        check(game.analyse(cancel).move == expected, what);
    };
    expect({2, 3, 2, 5, 4}, 4, "a block of the opponent's three is not mistaken for the opponent winning");
    expect({5, 3, 4, 2, 2, 4, 0, 6, 1, 1, 4, 4}, 2, "the depth-8 search decides the move");
    expect({0, 0, 4, 5}, 5, "the opponent's threats weigh 1.2 times its own");
    expect({0, 6, 5, 6}, 3, "equal columns are tried in the console's reverse-sorted order");
    expect({0, 1, 2, 0}, 3, "discs in the centre column count in the evaluation");
}

void testCancelDuringSearch()
{
    FourInRowGame heavy;
    playAll(heavy, {5, 3, 4, 2, 2, 4, 0, 6, 1, 1, 4, 4});
    turn_based::CancelToken never;
    const auto started = std::chrono::steady_clock::now();
    const auto full = heavy.analyse(never);
    const auto fullTime = std::chrono::steady_clock::now() - started;
    check(full.nodes > 20000, "the fixture needs a long search (" + std::to_string(full.nodes) + " positions)");

    turn_based::CancelToken token;
    auto search = std::async(std::launch::async, [&] { return heavy.analyse(token); });
    std::this_thread::sleep_for(fullTime / 5);
    const auto cancelledAt = std::chrono::steady_clock::now();
    token.cancel();
    const auto report = search.get();
    const auto stopTime = std::chrono::steady_clock::now() - cancelledAt;
    check(!report.move, "a search cancelled part-way returns no move");
    check(stopTime < fullTime / 2 + std::chrono::milliseconds(50), "cancelling stops the search promptly");
}
} // namespace

int main()
{
    testRules();
    testWins();
    testDraw();
    testComputerPriorities();
    testSearch();
    testSearchDetails();
    testCancelDuringSearch();
    turn_based_test::checkContract("Four-in-a-Row", [] { return std::make_unique<FourInRowGame>(20000); }, 42, 40);
    return turn_based_test::finish("Four-in-a-Row");
}
