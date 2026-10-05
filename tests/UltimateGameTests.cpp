#include "TurnBasedTestSupport.hpp"
#include "UltimateGame.hpp"

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>
#include <vector>

using turn_based::Seat;
using turn_based_test::check;
using ultimate_xo::Mark;
using ultimate_xo::Square;
using ultimate_xo::UltimateGame;

namespace {
void play(UltimateGame& game, int board, int cell)
{
    check(game.play(UltimateGame::encode(board, cell)),
          "setup move board " + std::to_string(board) + " cell " + std::to_string(cell) + " is legal");
}

Square squareFor(Seat seat)
{
    return seat == Seat::First ? Square::X : Square::O;
}

// Plays one small board, starting with the player to move, until it closes with `result`.
void playBoard(UltimateGame& game, int board, Square result)
{
    const Square starter = squareFor(game.currentSeat());
    std::vector<int> cells;
    if (result == Square::Tie) cells = {0, 1, 2, 4, 3, 5, 7, 6, 8};  // X O X / X O O / O X X
    else if (result == starter) cells = {0, 3, 1, 4, 2};            // the starter takes 0-1-2
    else cells = {0, 3, 1, 4, 8, 5};                                 // the other player takes 3-4-5
    for (const int cell : cells) play(game, board, cell);
    check(game.square(board) == result, "board " + std::to_string(board) + " closes as planned");
}

void testForcedBoard()
{
    UltimateGame game;
    check(game.legalMoves().size() == 81 && !game.forcedBoard(), "the first move may go anywhere");
    play(game, 4, 4);
    check(game.forcedBoard() == 4, "the next player must play in the same small board while it is open");
    const auto moves = game.legalMoves();
    check(moves.size() == 8 && std::all_of(moves.begin(), moves.end(), [](auto move) { return UltimateGame::boardOf(move) == 4; }),
          "only the open cells of that board are legal");
    check(!game.isLegal(UltimateGame::encode(0, 0)) && !game.play(UltimateGame::encode(0, 0)),
          "a move in another board is refused");
    check(!game.play(UltimateGame::encode(4, 4)) && !game.play(-1) && !game.play(81), "taken and invalid cells are refused");
}

void testClosingBoards()
{
    UltimateGame game;
    playBoard(game, 4, Square::X);
    check(!game.forcedBoard() && game.currentSeat() == Seat::Second, "after a small board is won the choice is free");
    check(game.smallBoardLine(4) == UltimateGame::Line{0, 1, 2}, "the line that won the small board is reported");
    check(!game.isLegal(UltimateGame::encode(4, 8)), "a won board takes no more moves");
    check(game.legalMoves().size() == 72, "every open cell of the other eight boards is legal");
    playBoard(game, 0, Square::Tie);
    check(game.square(0) == Square::Tie && !game.forcedBoard(), "a full board without a line becomes a tie square");
    check(!game.outcome().finished(), "the game goes on");
}

void testBigWin()
{
    UltimateGame game;
    playBoard(game, 0, Square::X);
    playBoard(game, 1, Square::X);
    check(!game.outcome().finished(), "two claimed squares are not enough");
    playBoard(game, 2, Square::X);
    check(game.outcome().winner == Seat::First && game.winningLine() == UltimateGame::Line{0, 1, 2},
          "three claimed squares in a line win the game");
    check(game.legalMoves().empty(), "no moves after the game is won");
}

void testSecondPlayerWins()
{
    // X takes boards 0, 2, and 3; O takes 1, 4, and 7, the middle column.
    UltimateGame game;
    playBoard(game, 0, Square::X);
    playBoard(game, 1, Square::O);
    playBoard(game, 2, Square::X);
    playBoard(game, 4, Square::O);
    playBoard(game, 3, Square::X);
    check(!game.outcome().finished(), "no big line yet");
    playBoard(game, 7, Square::O);
    check(game.outcome().winner == Seat::Second && game.winningLine() == UltimateGame::Line{1, 4, 7},
          "O's three small boards in a line win the game for the second player");
}

void testTieSquareInALine()
{
    UltimateGame game;
    playBoard(game, 2, Square::Tie);
    playBoard(game, 0, Square::X);
    playBoard(game, 1, Square::X);
    check(!game.outcome().finished() && game.square(2) == Square::Tie,
          "a tied small board counts for nobody: X X tie in a row is not a line");
}

void testLastBoardWins()
{
    // The last open board is won by X and completes the top row: a win, even though every board is closed.
    UltimateGame game;
    const std::pair<int, Square> order[]{{0, Square::X}, {3, Square::O}, {1, Square::X}, {4, Square::O}, {5, Square::X},
                                         {7, Square::O}, {6, Square::X}, {8, Square::O}, {2, Square::X}};
    for (const auto& [board, result] : order) playBoard(game, board, result);
    check(game.movesPlayed() == 45 && game.outcome().winner == Seat::First &&
              game.winningLine() == UltimateGame::Line{0, 1, 2},
          "a move that closes the last board and completes a line wins instead of drawing");
}

void testDraw()
{
    // Squares X O X / X O O / O X X: every board closes and no big line exists.
    UltimateGame game;
    const Square plan[9]{Square::X, Square::O, Square::X, Square::X, Square::O, Square::O, Square::O, Square::X, Square::X};
    for (int board = 0; board < 9; ++board) playBoard(game, board, plan[board]);
    check(game.outcome().status == turn_based::OutcomeStatus::Draw && !game.winningLine(),
          "closing every board without a big line is a draw");
    game.reset();
    check(game.movesPlayed() == 0 && game.square(4) == Square::Open && game.legalMoves().size() == 81,
          "reset clears every board");
}

void testComputer()
{
    std::mt19937 random(1);
    turn_based::CancelToken cancel;
    UltimateGame opening;
    const auto start = std::chrono::steady_clock::now();
    const auto first = opening.chooseComputerMove(random, cancel);
    check(first && opening.isLegal(*first), "the computer's opening move is legal");
    check(std::chrono::steady_clock::now() - start < std::chrono::seconds(10), "the opening search finishes quickly");

    // X owns boards 0 and 1 and holds cells 0 and 1 of board 2, where it must play: cell 2 wins.
    UltimateGame win;
    playBoard(win, 0, Square::X);
    playBoard(win, 1, Square::X);
    for (const int cell : {3, 0, 4, 1, 8}) play(win, 2, cell);  // O 3, X 0, O 4, X 1, O 8
    check(win.currentSeat() == Seat::First && win.forcedBoard() == 2, "win fixture has X to move in board 2");
    check(win.chooseComputerMove(random, cancel) == UltimateGame::encode(2, 2), "the computer wins the game when it can");

    // The same position one move earlier, with O to move: O must stop X (block 2 or win the board at 5).
    UltimateGame defend;
    playBoard(defend, 0, Square::X);
    playBoard(defend, 1, Square::X);
    for (const int cell : {3, 0, 4, 1}) play(defend, 2, cell);
    const auto reply = defend.chooseComputerMove(random, cancel);
    check(reply == UltimateGame::encode(2, 2) || reply == UltimateGame::encode(2, 5),
          "the computer stops an immediate loss");

    turn_based::CancelToken stop;
    stop.cancel();
    check(!opening.chooseComputerMove(random, stop), "a cancelled search returns no move");
}

// The computer's move for the position reached by `history`.
int computerChoice(const std::vector<int>& history)
{
    UltimateGame game;
    for (const int move : history) {
        if (!game.play(move)) return -1;  // never compare against a position the history did not reach
    }
    std::mt19937 random(1);
    turn_based::CancelToken cancel;
    return game.chooseComputerMove(random, cancel).value_or(-1);
}

void testConsoleChoices()
{
    // Moves chosen by the console's Ultimate_TTT_AI (depth 3), recorded by running the unmodified
    // console sources. They pin the search depth, every evaluation term, and the tie-break order
    // (the first best move in board, row, column order).
    check(computerChoice({}) == UltimateGame::encode(0, 4), "the opening move is the console's: board 0, cell 4");
    check(computerChoice({40}) == UltimateGame::encode(4, 0), "O answers board 4 cell 4 with cell 0, as the console does");
    check(computerChoice({36}) == UltimateGame::encode(4, 4), "O answers board 4 cell 0 with cell 4, as the console does");
    check(computerChoice({36, 39, 37, 40, 38}) == UltimateGame::encode(0, 4),
          "with a free choice after X wins board 4, O plays the console's move");
    // Found by searching random games: here a win scored without its depth (a slow win valued like a
    // quick one) would change the move.
    check(computerChoice({42, 36, 43, 44, 38, 39, 37, 40, 24, 19, 26, 18, 21, 23, 22, 20, 45, 49, 52, 53, 50, 46, 48, 47,
                          51, 61, 54, 60}) == UltimateGame::encode(6, 8),
          "a quicker win is preferred exactly as in the console");

    // The console's whole computer-vs-computer game from the empty board: 81 moves and a draw.
    const std::vector<int> consoleGame{
        4,  0,  1,  7,  3,  5,  2,  6,  8,  13, 9,  10, 16, 12, 14, 11, 15, 17, 22, 18, 19, 25, 21, 23, 20, 24, 26,
        31, 27, 28, 34, 30, 32, 29, 33, 35, 40, 36, 37, 43, 39, 41, 38, 42, 44, 49, 45, 46, 47, 48, 50, 51, 52, 53,
        58, 54, 55, 61, 57, 59, 56, 60, 62, 67, 63, 64, 65, 66, 68, 69, 70, 71, 76, 72, 73, 74, 75, 77, 78, 79, 80};
    UltimateGame game;
    std::mt19937 random(1);
    turn_based::CancelToken cancel;
    std::size_t matched = 0;
    while (matched < consoleGame.size() && !game.outcome().finished()) {
        const auto move = game.chooseComputerMove(random, cancel);
        if (move != consoleGame[matched] || !game.play(*move)) break;
        ++matched;
    }
    check(matched == consoleGame.size() && game.outcome().status == turn_based::OutcomeStatus::Draw,
          "computer-vs-computer play repeats the console's 81-move drawn game (matched " + std::to_string(matched) +
              " moves)");
}
} // namespace

int main()
{
    testForcedBoard();
    testClosingBoards();
    testBigWin();
    testSecondPlayerWins();
    testTieSquareInALine();
    testLastBoardWins();
    testDraw();
    testComputer();
    testConsoleChoices();
    turn_based_test::checkContract("Ultimate", [] { return std::make_unique<UltimateGame>(); }, 81, 30);
    return turn_based_test::finish("Ultimate XO");
}
