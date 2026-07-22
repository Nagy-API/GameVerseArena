#include "TicTacToeAI.hpp"
#include "TicTacToeBoard.hpp"
#include "TicTacToeSession.hpp"

#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>

using namespace classic_ttt;

namespace {
int failures = 0;

void check(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++failures;
    }
}

void play(TicTacToeBoard& board, std::size_t row, std::size_t column, Cell mark)
{
    check(board.placeMark({row, column}, mark), "test setup move should be legal");
}

void testBoard()
{
    TicTacToeBoard board;
    check(board.status() == GameStatus::InProgress && board.moveCount() == 0, "new board is empty and active");
    for (const auto& row : board.cells()) for (Cell cell : row) check(cell == Cell::Empty, "new cell is empty");
    check(board.placeMark({0, 0}, Cell::X), "valid move is accepted");
    check(!board.placeMark({0, 0}, Cell::O), "occupied cell is rejected");
    check(!board.placeMark({3, 0}, Cell::O), "out-of-range row is rejected");
    check(!board.placeMark({0, 3}, Cell::O), "out-of-range column is rejected");
    check(board.moveCount() == 1, "rejected moves do not increment move count");

    board.reset();
    play(board, 0, 0, Cell::X); play(board, 0, 1, Cell::X); play(board, 0, 2, Cell::X);
    check(board.status() == GameStatus::XWon, "row win is detected");
    check(board.winningLine() == WinningLine{{{0, 0}, {0, 1}, {0, 2}}}, "row winning coordinates are correct");
    check(!board.placeMark({1, 1}, Cell::O), "moves after completion are rejected");

    board.reset();
    play(board, 0, 2, Cell::O); play(board, 1, 2, Cell::O); play(board, 2, 2, Cell::O);
    check(board.status() == GameStatus::OWon, "column win is detected");
    check(board.winningLine() == WinningLine{{{0, 2}, {1, 2}, {2, 2}}}, "column coordinates are correct");

    board.reset();
    play(board, 0, 0, Cell::X); play(board, 1, 1, Cell::X); play(board, 2, 2, Cell::X);
    check(board.status() == GameStatus::XWon, "main diagonal win is detected");
    check(board.winningLine() == WinningLine{{{0, 0}, {1, 1}, {2, 2}}}, "main diagonal coordinates are correct");

    board.reset();
    play(board, 0, 2, Cell::O); play(board, 1, 1, Cell::O); play(board, 2, 0, Cell::O);
    check(board.status() == GameStatus::OWon, "anti-diagonal win is detected");
    check(board.winningLine() == WinningLine{{{0, 2}, {1, 1}, {2, 0}}}, "anti-diagonal coordinates are correct");

    board.reset();
    const Cell draw[3][3]{{Cell::X, Cell::O, Cell::X}, {Cell::X, Cell::O, Cell::O}, {Cell::O, Cell::X, Cell::X}};
    for (std::size_t row = 0; row < 3; ++row) for (std::size_t column = 0; column < 3; ++column) play(board, row, column, draw[row][column]);
    check(board.status() == GameStatus::Draw && !board.winningLine(), "full non-winning board is a draw");
    board.reset();
    check(board.status() == GameStatus::InProgress && board.moveCount() == 0 && board.legalMoves().size() == 9, "reset clears all board state");
}

void finishXWin(TicTacToeSession& session)
{
    check(session.playMove({0, 0}), "X move 1");
    check(session.playMove({1, 0}), "O move 1");
    check(session.playMove({0, 1}), "X move 2");
    check(session.playMove({1, 1}), "O move 2");
    check(session.playMove({0, 2}), "X winning move");
}

void testSession()
{
    TicTacToeSession session;
    SessionConfig config;
    config.bestOf = BestOf::Three;
    session.startNewMatch(config);
    check(session.currentTurn() == Cell::X, "X begins a session");
    check(!session.playMove({9, 9}) && session.currentTurn() == Cell::X, "illegal session move does not change turn");
    check(session.playMove({0, 0}) && session.currentTurn() == Cell::O, "turn alternates after legal move");
    check(!session.playMove({0, 0}) && session.currentTurn() == Cell::O, "occupied move does not change turn");
    session.restartRound();
    finishXWin(session);
    check(session.score().xWins == 1, "win increments score once");
    check(!session.playMove({2, 2}) && session.score().xWins == 1, "late events cannot duplicate a score");
    session.restartRound();
    check(session.score().xWins == 0 && session.roundNumber() == 1 && session.board().moveCount() == 0,
          "restarting a completed round retracts its result");
    finishXWin(session);
    check(session.nextRound() && session.roundNumber() == 2 && session.score().xWins == 1, "next round preserves score");
    finishXWin(session);
    check(session.matchFinished(), "best-of-3 finishes at two wins");

    config.bestOf = BestOf::Five;
    session.startNewMatch(config);
    for (int round = 0; round < 3; ++round) {
        finishXWin(session);
        if (round < 2) check(session.nextRound(), "best-of-5 advances before third win");
    }
    check(session.matchFinished() && session.score().xWins == 3, "best-of-5 finishes at three wins");
    session.rematch();
    check(session.score().xWins == 0 && session.score().oWins == 0 && session.score().draws == 0 && session.roundNumber() == 1,
          "rematch resets score and round");

    config.bestOf = BestOf::Three;
    session.startNewMatch(config);
    const Position drawMoves[]{{0,0},{0,1},{0,2},{1,1},{1,0},{1,2},{2,1},{2,0},{2,2}};
    for (const auto move : drawMoves) check(session.playMove(move), "draw setup session move");
    check(session.score().draws == 1, "draw increments exactly once");
    check(!session.playMove({2,2}) && session.score().draws == 1, "late event cannot duplicate draw");
    check(session.nextRound() && session.roundNumber() == 2 && session.score().draws == 1,
          "round number advances after a draw while preserving its score");

    SessionConfig replacement;
    replacement.playerOneName = "  Alice  ";
    replacement.playerTwoName = "";
    replacement.bestOf = BestOf::Single;
    session.startNewMatch(replacement);
    check(session.playerName(Cell::X) == "Alice" && session.playerName(Cell::O) == "Player 2", "new match trims names and applies defaults");
    check(session.score().draws == 0 && session.roundNumber() == 1 && session.board().moveCount() == 0, "new match resets all match state");
}

bool humanCanForceWin(TicTacToeBoard board, Cell turn, Cell humanMark, TicTacToeAI& ai)
{
    const auto humanWon = humanMark == Cell::X ? GameStatus::XWon : GameStatus::OWon;
    if (board.status() == humanWon) return true;
    if (board.status() != GameStatus::InProgress) return false;

    if (turn == humanMark) {
        for (const Position move : board.legalMoves()) {
            auto next = board;
            next.placeMark(move, humanMark);
            if (humanCanForceWin(next, opposite(turn), humanMark, ai)) return true;
        }
        return false;
    }

    const auto move = ai.chooseMove(board, turn, AIDifficulty::Hard);
    if (!move) return false;
    board.placeMark(*move, turn);
    return humanCanForceWin(board, opposite(turn), humanMark, ai);
}

void testAI()
{
    TicTacToeAI ai(12345);
    TicTacToeBoard board;
    for (int attempt = 0; attempt < 30; ++attempt) {
        const auto move = ai.chooseMove(board, Cell::X, AIDifficulty::Easy);
        check(move && board.cell(*move) == Cell::Empty, "Easy AI always returns a legal move");
    }

    play(board, 0, 0, Cell::O); play(board, 0, 1, Cell::O);
    auto move = ai.chooseMove(board, Cell::O, AIDifficulty::Medium);
    check(move == Position{0, 2}, "Medium AI takes an immediate win");
    board.reset();
    play(board, 1, 0, Cell::X); play(board, 1, 1, Cell::X);
    move = ai.chooseMove(board, Cell::O, AIDifficulty::Medium);
    check(move == Position{1, 2}, "Medium AI blocks immediate loss");
    board.reset();
    play(board, 0, 0, Cell::X);
    move = ai.chooseMove(board, Cell::O, AIDifficulty::Medium);
    check(move == Position{1, 1}, "Medium AI takes center without urgent move");

    board.reset();
    play(board, 2, 0, Cell::X); play(board, 0, 0, Cell::O); play(board, 2, 1, Cell::X); play(board, 1, 1, Cell::O);
    move = ai.chooseMove(board, Cell::X, AIDifficulty::Hard);
    check(move == Position{2, 2}, "Hard AI takes immediate win");
    board.reset();
    play(board, 0, 0, Cell::X); play(board, 1, 1, Cell::O); play(board, 0, 1, Cell::X);
    move = ai.chooseMove(board, Cell::O, AIDifficulty::Hard);
    check(move == Position{0, 2}, "Hard AI blocks immediate loss");

    TicTacToeAI exhaustiveX(7);
    check(!humanCanForceWin(TicTacToeBoard{}, Cell::X, Cell::O, exhaustiveX), "Hard AI as X cannot be forced to lose");
    TicTacToeAI exhaustiveO(9);
    check(!humanCanForceWin(TicTacToeBoard{}, Cell::X, Cell::X, exhaustiveO), "Hard AI as O cannot be forced to lose");

    TicTacToeBoard finished;
    play(finished, 0, 0, Cell::X); play(finished, 0, 1, Cell::X); play(finished, 0, 2, Cell::X);
    check(!ai.chooseMove(finished, Cell::O, AIDifficulty::Hard), "Hard AI returns no move on finished board");
    TicTacToeBoard full;
    const Cell draw[3][3]{{Cell::X, Cell::O, Cell::X}, {Cell::X, Cell::O, Cell::O}, {Cell::O, Cell::X, Cell::X}};
    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t column = 0; column < 3; ++column) play(full, row, column, draw[row][column]);
    check(!ai.chooseMove(full, Cell::O, AIDifficulty::Hard), "Hard AI returns no move on a full draw board");
}
} // namespace

int main()
{
    testBoard();
    testSession();
    testAI();
    if (failures == 0) {
        std::cout << "All Classic Tic-Tac-Toe tests passed.\n";
        return EXIT_SUCCESS;
    }
    std::cerr << failures << " test assertion(s) failed.\n";
    return EXIT_FAILURE;
}
