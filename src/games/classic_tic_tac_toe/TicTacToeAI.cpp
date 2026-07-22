#include "TicTacToeAI.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

namespace classic_ttt {

TicTacToeAI::TicTacToeAI() : random_(std::random_device{}()) {}
TicTacToeAI::TicTacToeAI(std::mt19937::result_type seed) : random_(seed) {}

std::optional<Position> TicTacToeAI::chooseMove(const TicTacToeBoard& board, Cell aiMark, AIDifficulty difficulty)
{
    if (aiMark == Cell::Empty || board.status() != GameStatus::InProgress) {
        return std::nullopt;
    }
    switch (difficulty) {
    case AIDifficulty::Easy: return chooseEasy(board);
    case AIDifficulty::Medium: return chooseMedium(board, aiMark);
    case AIDifficulty::Hard: return chooseHard(board, aiMark);
    }
    return std::nullopt;
}

std::optional<Position> TicTacToeAI::chooseEasy(const TicTacToeBoard& board)
{
    const auto moves = board.legalMoves();
    if (moves.empty()) {
        return std::nullopt;
    }
    std::uniform_int_distribution<std::size_t> choice(0, moves.size() - 1);
    return moves[choice(random_)];
}

std::optional<Position> TicTacToeAI::findImmediateMove(const TicTacToeBoard& board, Cell mark) const
{
    for (const Position move : board.legalMoves()) {
        auto trial = board;
        trial.placeMark(move, mark);
        const auto won = mark == Cell::X ? GameStatus::XWon : GameStatus::OWon;
        if (trial.status() == won) {
            return move;
        }
    }
    return std::nullopt;
}

std::optional<Position> TicTacToeAI::chooseMedium(const TicTacToeBoard& board, Cell aiMark)
{
    if (const auto win = findImmediateMove(board, aiMark)) return win;
    if (const auto block = findImmediateMove(board, opposite(aiMark))) return block;
    if (board.cell({1, 1}) == Cell::Empty) return Position{1, 1};

    constexpr std::array<Position, 4> corners{{{0, 0}, {0, 2}, {2, 0}, {2, 2}}};
    std::vector<Position> availableCorners;
    for (const auto corner : corners) {
        if (board.cell(corner) == Cell::Empty) availableCorners.push_back(corner);
    }
    if (!availableCorners.empty()) {
        std::uniform_int_distribution<std::size_t> choice(0, availableCorners.size() - 1);
        return availableCorners[choice(random_)];
    }
    return chooseEasy(board);
}

std::optional<Position> TicTacToeAI::chooseHard(const TicTacToeBoard& board, Cell aiMark)
{
    const auto moves = board.legalMoves();
    if (moves.empty()) return std::nullopt;

    int bestScore = std::numeric_limits<int>::min();
    std::vector<Position> bestMoves;
    for (const Position move : moves) {
        auto trial = board;
        trial.placeMark(move, aiMark);
        const int score = minimax(trial, opposite(aiMark), aiMark, 1,
                                  std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        if (score > bestScore) {
            bestScore = score;
            bestMoves = {move};
        } else if (score == bestScore) {
            bestMoves.push_back(move);
        }
    }
    std::uniform_int_distribution<std::size_t> choice(0, bestMoves.size() - 1);
    return bestMoves[choice(random_)];
}

int TicTacToeAI::minimax(const TicTacToeBoard& board, Cell turn, Cell aiMark,
                         int depth, int alpha, int beta) const
{
    if (board.status() != GameStatus::InProgress) {
        const auto aiWon = aiMark == Cell::X ? GameStatus::XWon : GameStatus::OWon;
        const auto opponentWon = aiMark == Cell::X ? GameStatus::OWon : GameStatus::XWon;
        if (board.status() == aiWon) return 10 - depth;
        if (board.status() == opponentWon) return depth - 10;
        return 0;
    }

    const bool maximizing = turn == aiMark;
    int best = maximizing ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    for (const Position move : board.legalMoves()) {
        auto trial = board;
        trial.placeMark(move, turn);
        const int score = minimax(trial, opposite(turn), aiMark, depth + 1, alpha, beta);
        if (maximizing) {
            best = std::max(best, score);
            alpha = std::max(alpha, best);
        } else {
            best = std::min(best, score);
            beta = std::min(beta, best);
        }
        if (beta <= alpha) break;
    }
    return best;
}

} // namespace classic_ttt
