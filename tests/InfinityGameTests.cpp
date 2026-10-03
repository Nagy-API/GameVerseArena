#include "InfinityGame.hpp"
#include "TurnBasedTestSupport.hpp"

#include <set>

using infinity_xo::InfinityGame;
using infinity_xo::Mark;
using turn_based::Seat;
using turn_based_test::check;

namespace {
void play(InfinityGame& game, int cell)
{
    check(game.play(cell), "setup move at cell " + std::to_string(cell) + " is legal");
}

void testVanishingMarks()
{
    InfinityGame game;
    check(InfinityGame::removesOldestAfter(6) && InfinityGame::removesOldestAfter(9) &&
              !InfinityGame::removesOldestAfter(3) && !InfinityGame::removesOldestAfter(5) &&
              !InfinityGame::removesOldestAfter(7),
          "the oldest mark vanishes after the 6th and 9th moves only");
    // X 4, O 0, X 2, O 6, X 3: no line yet.
    for (const int cell : {4, 0, 2, 6, 3}) play(game, cell);
    check(game.nextToVanish() == 4, "before the 6th move, the first mark is marked to vanish");
    play(game, 5);  // O's 6th move
    check(game.cell(4) == Mark::Empty && game.lastRemoved() == 4 && game.cell(5) == Mark::O,
          "the 6th move removes the oldest mark (the first X)");
    check(!game.nextToVanish(), "the 7th and 8th moves remove nothing");
    check(game.isLegal(4), "a vanished mark's cell can be used again");
    play(game, 1);  // X
    play(game, 8);  // O
    check(!game.outcome().finished() && game.nextToVanish() == 0, "before the 9th move, the second mark is marked to vanish");
    play(game, 7);  // X's 9th move: X 2, 3, 1, 7 -- no line
    check(game.cell(0) == Mark::Empty && game.lastRemoved() == 0, "the 9th move removes the next-oldest mark");
    check(game.outcome().status == turn_based::OutcomeStatus::Draw && game.movesPlayed() == 9,
          "a 9th move that does not win ends the game in a draw");
    game.reset();
    check(game.movesPlayed() == 0 && game.cell(2) == Mark::Empty && !game.lastRemoved(), "reset clears the grid and history");
}

void testWins()
{
    InfinityGame early;
    for (const int cell : {0, 3, 1, 4}) play(early, cell);
    play(early, 2);
    check(early.outcome().winner == Seat::First && early.winningLine() == InfinityGame::Line{0, 1, 2},
          "three in a row wins before any mark vanishes");

    // O wins on move 8: O holds 3, 4 (moves 2, 4) and completes 3-4-5 after X's first mark vanished.
    InfinityGame late;
    for (const int cell : {0, 3, 8, 4, 2, 6, 1}) play(late, cell);
    check(!late.outcome().finished() && late.cell(0) == Mark::Empty, "X's first mark vanished at move 6");
    play(late, 5);
    check(late.outcome().winner == Seat::Second && late.winningLine() == InfinityGame::Line{3, 4, 5},
          "O can win on a later move");

    // At move 9 the mark that vanishes is O's (the second one placed), so X can still win there:
    // X holds 2, 3, 5 (its first mark, 4, vanished at move 6) and completes 2-5-8.
    InfinityGame ninth;
    for (const int cell : {4, 0, 2, 6, 3, 1, 5, 7}) play(ninth, cell);
    check(!ninth.outcome().finished() && ninth.cell(4) == Mark::Empty, "ninth-move fixture is in progress");
    play(ninth, 8);
    check(ninth.outcome().winner == Seat::First && ninth.winningLine() == InfinityGame::Line{2, 5, 8} &&
              ninth.cell(0) == Mark::Empty,
          "the 9th move can still win; the mark that vanishes then is the opponent's");
}

void testComputer()
{
    std::set<turn_based::MoveId> seen;
    turn_based::CancelToken cancel;
    for (std::uint32_t seed = 1; seed <= 100; ++seed) {
        InfinityGame game;
        std::mt19937 random(seed);
        const auto move = game.chooseComputerMove(random, cancel);
        check(move && game.isLegal(*move), "the computer plays a legal cell");
        if (move) seen.insert(*move);
    }
    check(seen.size() == 9, "the computer's random choice covers every cell");
}
} // namespace

int main()
{
    testVanishingMarks();
    testWins();
    testComputer();
    turn_based_test::checkContract("Infinity", [] { return std::make_unique<InfinityGame>(); }, 9);
    return turn_based_test::finish("Infinity XO");
}
