#include "FourByFourGame.hpp"
#include "TurnBasedTestSupport.hpp"

#include <algorithm>

using four_by_four::FourByFourGame;
using four_by_four::Token;
using turn_based::Seat;
using turn_based_test::check;

namespace {
void slide(FourByFourGame& game, int from, int to)
{
    check(game.play(FourByFourGame::encode(from, to)),
          "setup slide " + std::to_string(from) + " -> " + std::to_string(to) + " is legal");
}

void testOpening()
{
    FourByFourGame game;
    const std::string top{static_cast<char>(game.cell(0)), static_cast<char>(game.cell(1)), static_cast<char>(game.cell(2)),
                          static_cast<char>(game.cell(3))};
    const std::string bottom{static_cast<char>(game.cell(12)), static_cast<char>(game.cell(13)),
                             static_cast<char>(game.cell(14)), static_cast<char>(game.cell(15))};
    check(top == "OXOX" && bottom == "XOXO", "the board opens with O X O X on top and X O X O at the bottom");
    check(game.currentSeat() == Seat::First, "X moves first");
    // Each X token has exactly one open neighbour at the start.
    const auto moves = game.legalMoves();
    check(moves.size() == 4, "X has four opening slides");
    check(moves.front() == FourByFourGame::encode(1, 5), "slides are listed token by token in board order");
    check(game.destinationsFrom(1) == std::vector<int>{5} && game.destinationsFrom(0).empty(),
          "only the player to move's tokens have destinations");
}

void testIllegalSlides()
{
    FourByFourGame game;
    check(!game.play(FourByFourGame::encode(0, 4)), "a player cannot move the opponent's token");
    check(!game.play(FourByFourGame::encode(1, 9)), "a token moves only one cell");
    check(!game.play(FourByFourGame::encode(1, 2)), "a token cannot move onto another token");
    check(!game.play(FourByFourGame::encode(12, 9)), "a token cannot move diagonally");
    check(!game.play(-1) && !game.play(256), "encoded moves outside the board are illegal");
    slide(game, 1, 5);
    check(game.cell(1) == Token::Empty && game.cell(5) == Token::X && game.currentSeat() == Seat::Second,
          "a slide moves the token and passes the turn");
}

void testWin()
{
    FourByFourGame game;
    slide(game, 1, 5);
    slide(game, 13, 9);
    slide(game, 3, 7);
    slide(game, 9, 13);
    slide(game, 14, 10);
    slide(game, 13, 9);
    check(!game.outcome().finished(), "no line yet");
    slide(game, 10, 6);
    check(game.outcome().winner == Seat::First && game.winningLine() &&
              *game.winningLine() == FourByFourGame::Line{5, 6, 7},
          "three X tokens in a row win");
    check(game.legalMoves().empty() && !game.play(FourByFourGame::encode(5, 4)), "the game is over after a win");
    game.reset();
    check(game.cell(1) == Token::X && game.cell(5) == Token::Empty && game.movesPlayed() == 0, "reset restores the opening");
}

void testStalemateIsADraw()
{
    // X's four tokens fill the top-left square and O's tokens wall it in: X cannot slide.
    std::array<Token, 16> layout;
    layout.fill(Token::Empty);
    for (const int cell : {0, 1, 4, 5}) layout[static_cast<std::size_t>(cell)] = Token::X;
    for (const int cell : {2, 6, 8, 9}) layout[static_cast<std::size_t>(cell)] = Token::O;
    FourByFourGame stuck(layout, Seat::First);
    check(stuck.outcome().status == turn_based::OutcomeStatus::Draw && stuck.legalMoves().empty(),
          "a player with no legal slide ends the game in a draw");

    FourByFourGame free(layout, Seat::Second);
    check(!free.outcome().finished() && !free.legalMoves().empty(), "the other player can still move");
    // O slides 6 -> 7 and frees X's token on 5, so the game goes on.
    check(free.play(FourByFourGame::encode(6, 7)) && !free.outcome().finished(),
          "the game continues while the next player can slide");
}

void testComputer()
{
    std::mt19937 random(1);
    turn_based::CancelToken cancel;
    FourByFourGame opening;
    const auto first = opening.chooseComputerMove(random, cancel);
    check(first == FourByFourGame::encode(1, 5), "the computer opens by sliding its token on 1 down to 5, as the console does");

    // X to move with X on 5, 7, 10, 12 and O on 0, 2, 9, 15: sliding 10 -> 6 completes 5-6-7.
    FourByFourGame winning;
    slide(winning, 1, 5);
    slide(winning, 13, 9);
    slide(winning, 3, 7);
    slide(winning, 9, 13);
    slide(winning, 14, 10);
    slide(winning, 13, 9);
    check(winning.evaluate(Token::X) < 100, "the fixture is not yet decided");
    check(winning.chooseComputerMove(random, cancel) == FourByFourGame::encode(10, 6),
          "the computer takes a winning slide");

    // The console's evaluation scores every row and column window but only two diagonal windows.
    std::array<Token, 16> diagonal;
    diagonal.fill(Token::Empty);
    diagonal[1] = diagonal[6] = Token::X;  // a pair on the unscored diagonal 1-6-11
    FourByFourGame unscored(diagonal, Seat::First);
    diagonal.fill(Token::Empty);
    diagonal[0] = diagonal[5] = Token::X;  // a pair on the scored diagonal 0-5-10
    FourByFourGame scored(diagonal, Seat::First);
    check(unscored.evaluate(Token::X) == 0 && scored.evaluate(Token::X) == 10,
          "the evaluation only counts the diagonals the console counted");
}
} // namespace

int main()
{
    testOpening();
    testIllegalSlides();
    testWin();
    testStalemateIsADraw();
    testComputer();
    // Slides can go on forever (the console has no move limit), so games are not required to end.
    turn_based_test::checkContract("4x4", [] { return std::make_unique<FourByFourGame>(); }, 200, 60, false);
    return turn_based_test::finish("4x4 Tic-Tac-Toe");
}
