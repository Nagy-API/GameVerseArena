#include "MisereGame.hpp"
#include "TurnBasedTestSupport.hpp"

using misere::Mark;
using misere::MisereGame;
using turn_based::Seat;
using turn_based_test::check;

namespace {
void play(MisereGame& game, int cell)
{
    check(game.play(cell), "setup move at cell " + std::to_string(cell) + " is legal");
}

void testRules()
{
    MisereGame game;
    check(game.legalMoves().size() == 9 && game.currentSeat() == Seat::First, "X moves first on an empty grid");
    play(game, 4);
    check(game.cell(4) == Mark::X && game.currentSeat() == Seat::Second, "X is placed and the turn passes to O");
    check(!game.play(4), "an occupied cell is illegal");
    check(!game.play(9) && !game.play(-1), "cells outside the grid are illegal");
}

void testCompletingALineLoses()
{
    // X completes the top row itself: X loses, so O wins.
    MisereGame game;
    for (const int cell : {0, 3, 1, 4}) play(game, cell);
    check(!game.outcome().finished(), "two in a row does not end the game");
    play(game, 2);
    check(game.outcome().status == turn_based::OutcomeStatus::Won && game.outcome().winner == Seat::Second,
          "completing your own line of three loses");
    check(game.losingLine() && *game.losingLine() == MisereGame::Line{0, 1, 2}, "the losing line is reported");
    check(game.legalMoves().empty() && !game.play(5), "no moves are accepted after the game ends");

    // O completes the middle row: X wins.
    MisereGame second;
    for (const int cell : {0, 3, 1, 4, 8, 5}) play(second, cell);
    check(second.outcome().winner == Seat::First && *second.losingLine() == MisereGame::Line{3, 4, 5},
          "O completing a line hands the win to X");
}

void testDraw()
{
    // X O X / X O O / O X X contains no line for either mark.
    MisereGame game;
    for (const int cell : {0, 1, 2, 4, 3, 5, 7, 6}) play(game, cell);
    check(!game.outcome().finished(), "eight marks without a line continue");
    play(game, 8);
    check(game.outcome().status == turn_based::OutcomeStatus::Draw && !game.losingLine(),
          "nine marks without a line are a draw");
    game.reset();
    check(game.movesPlayed() == 0 && game.cell(0) == Mark::Empty && !game.outcome().finished(),
          "reset clears the grid");
}

void testComputer()
{
    std::mt19937 random(1);
    turn_based::CancelToken cancel;

    // X (computer) holds 0 and 1; playing 2 would complete its own line, so it avoids 2.
    MisereGame avoid;
    for (const int cell : {0, 4, 1, 8}) play(avoid, cell);
    const auto choice = avoid.chooseComputerMove(random, cancel);
    check(choice && *choice != 2 && avoid.isLegal(*choice), "the computer avoids completing its own line");

    // When the only empty cell completes its own line, it still has to play it.
    MisereGame forced;
    for (const int cell : {0, 3, 1, 4, 5, 7, 6, 8}) play(forced, cell);
    check(forced.legalMoves() == std::vector<turn_based::MoveId>{2}, "fixture leaves only cell 2 open");
    check(forced.chooseComputerMove(random, cancel) == turn_based::MoveId{2}, "a forced losing move is still played");
    forced.play(2);
    check(forced.outcome().winner == Seat::Second, "the forced move loses for X");

    // Ties keep the first best cell, as the console's strict comparison did.
    MisereGame afterCorner;
    play(afterCorner, 0);
    check(afterCorner.chooseComputerMove(random, cancel) == turn_based::MoveId{1},
          "after X takes a corner, O picks the first of its best cells");
    MisereGame afterCentre;
    play(afterCentre, 4);
    check(afterCentre.chooseComputerMove(random, cancel) == turn_based::MoveId{0},
          "after X takes the centre, O picks the first of its best cells");

    // Both sides searching the full game tree never complete a line: perfect Misere play draws.
    MisereGame perfect;
    while (!perfect.outcome().finished()) {
        const auto move = perfect.chooseComputerMove(random, cancel);
        check(move.has_value(), "the computer always finds a move in an unfinished game");
        if (!move) break;
        perfect.play(*move);
    }
    check(perfect.outcome().status == turn_based::OutcomeStatus::Draw, "computer against computer is a draw");
}
} // namespace

int main()
{
    testRules();
    testCompletingALineLoses();
    testDraw();
    testComputer();
    turn_based_test::checkContract("Misere", [] { return std::make_unique<MisereGame>(); }, 9);
    return turn_based_test::finish("Misere Tic-Tac-Toe");
}
