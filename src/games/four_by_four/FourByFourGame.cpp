#include "FourByFourGame.hpp"

#include <algorithm>
#include <cstdlib>

namespace four_by_four {
namespace {

using Board = std::array<Token, 16>;

// The console's slide order: down, up, right, left.
constexpr int directions[4][2]{{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

Token at(const Board& board, int row, int column)
{
    return board[static_cast<std::size_t>(row * 4 + column)];
}

void set(Board& board, int row, int column, Token token)
{
    board[static_cast<std::size_t>(row * 4 + column)] = token;
}

// All 24 lines of three: every row and column window, and every diagonal window.
const std::vector<FourByFourGame::Line>& lines()
{
    static const std::vector<FourByFourGame::Line> all = [] {
        std::vector<FourByFourGame::Line> result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; ++j) {
                result.push_back({i * 4 + j, i * 4 + j + 1, i * 4 + j + 2});
                result.push_back({j * 4 + i, (j + 1) * 4 + i, (j + 2) * 4 + i});
            }
        }
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) result.push_back({i * 4 + j, (i + 1) * 4 + j + 1, (i + 2) * 4 + j + 2});
        }
        for (int i = 0; i < 2; ++i) {
            for (int j = 2; j < 4; ++j) result.push_back({i * 4 + j, (i + 1) * 4 + j - 1, (i + 2) * 4 + j - 2});
        }
        return result;
    }();
    return all;
}

std::optional<FourByFourGame::Line> lineOf(const Board& board, Token token)
{
    for (const auto& line : lines()) {
        if (board[static_cast<std::size_t>(line[0])] == token && board[static_cast<std::size_t>(line[1])] == token &&
            board[static_cast<std::size_t>(line[2])] == token) {
            return line;
        }
    }
    return std::nullopt;
}

bool canSlide(const Board& board, int row, int column, const int (&direction)[2])
{
    const int nextRow = row + direction[0];
    const int nextColumn = column + direction[1];
    return nextRow >= 0 && nextRow < 4 && nextColumn >= 0 && nextColumn < 4 &&
           at(board, nextRow, nextColumn) == Token::Empty;
}

bool hasSlide(const Board& board, Token token)
{
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            if (at(board, row, column) != token) continue;
            for (const auto& direction : directions) {
                if (canSlide(board, row, column, direction)) return true;
            }
        }
    }
    return false;
}

int evaluateLine(Token a, Token b, Token c, Token computer, Token human)
{
    const int computerCount = (a == computer) + (b == computer) + (c == computer);
    const int humanCount = (a == human) + (b == human) + (c == human);
    if (computerCount == 3) return 100;
    if (humanCount == 3) return -100;
    if (computerCount == 2 && humanCount == 0) return 10;
    if (humanCount == 2 && computerCount == 0) return -10;
    return 0;
}

// The console's evaluation: every row and column window, but only two of the eight diagonal windows.
int evaluateBoard(const Board& board, Token computer, Token human)
{
    int score = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 2; ++j) {
            score += evaluateLine(at(board, i, j), at(board, i, j + 1), at(board, i, j + 2), computer, human);
        }
    }
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 2; ++i) {
            score += evaluateLine(at(board, i, j), at(board, i + 1, j), at(board, i + 2, j), computer, human);
        }
    }
    score += evaluateLine(at(board, 0, 0), at(board, 1, 1), at(board, 2, 2), computer, human);
    score += evaluateLine(at(board, 0, 3), at(board, 1, 2), at(board, 2, 1), computer, human);
    return score;
}

int minimax(Board& board, int depth, bool maximizing, Token computer, Token human)
{
    const int score = evaluateBoard(board, computer, human);
    if (depth == 0 || std::abs(score) >= 100) return score;
    const Token mover = maximizing ? computer : human;
    int best = maximizing ? -10000 : 10000;
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            if (at(board, row, column) != mover) continue;
            for (const auto& direction : directions) {
                if (!canSlide(board, row, column, direction)) continue;
                const int nextRow = row + direction[0];
                const int nextColumn = column + direction[1];
                set(board, row, column, Token::Empty);
                set(board, nextRow, nextColumn, mover);
                const int value = minimax(board, depth - 1, !maximizing, computer, human);
                set(board, row, column, mover);
                set(board, nextRow, nextColumn, Token::Empty);
                best = maximizing ? std::max(best, value) : std::min(best, value);
            }
        }
    }
    return best;
}

} // namespace

FourByFourGame::FourByFourGame()
{
    reset();
}

FourByFourGame::FourByFourGame(const std::array<Token, 16>& layout, Seat toMove) : cells_(layout), turn_(toMove)
{
    if (!hasSlide(cells_, tokenFor(turn_))) outcome_ = turn_based::drawResult("The player to move has no legal slide");
}

std::unique_ptr<turn_based::TurnBasedGame> FourByFourGame::clone() const
{
    return std::make_unique<FourByFourGame>(*this);
}

void FourByFourGame::reset()
{
    cells_.fill(Token::Empty);
    constexpr Token top[4]{Token::O, Token::X, Token::O, Token::X};
    constexpr Token bottom[4]{Token::X, Token::O, Token::X, Token::O};
    for (int column = 0; column < 4; ++column) {
        cells_[static_cast<std::size_t>(column)] = top[column];
        cells_[static_cast<std::size_t>(12 + column)] = bottom[column];
    }
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
    winningLine_.reset();
}

bool FourByFourGame::isLegal(MoveId move) const
{
    if (outcome_.finished() || move < 0 || move >= 256) return false;
    const int from = fromOf(move);
    const int to = toOf(move);
    if (cells_[static_cast<std::size_t>(from)] != tokenFor(turn_) || cells_[static_cast<std::size_t>(to)] != Token::Empty) {
        return false;
    }
    return std::abs(from / 4 - to / 4) + std::abs(from % 4 - to % 4) == 1;
}

std::vector<int> FourByFourGame::destinationsFrom(int from) const
{
    std::vector<int> result;
    if (outcome_.finished() || from < 0 || from >= 16 || cells_[static_cast<std::size_t>(from)] != tokenFor(turn_)) {
        return result;
    }
    for (const auto& direction : directions) {
        if (canSlide(cells_, from / 4, from % 4, direction)) {
            result.push_back((from / 4 + direction[0]) * 4 + from % 4 + direction[1]);
        }
    }
    return result;
}

std::vector<MoveId> FourByFourGame::legalMoves() const
{
    std::vector<MoveId> moves;
    for (int from = 0; from < 16; ++from) {
        for (const int to : destinationsFrom(from)) moves.push_back(encode(from, to));
    }
    return moves;
}

bool FourByFourGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    const int from = fromOf(move);
    const int to = toOf(move);
    const Token token = tokenFor(turn_);
    cells_[static_cast<std::size_t>(from)] = Token::Empty;
    cells_[static_cast<std::size_t>(to)] = token;
    ++moves_;
    if (const auto line = lineOf(cells_, token)) {
        winningLine_ = line;
        outcome_ = turn_based::winFor(turn_, "Three in a row");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    if (!hasSlide(cells_, tokenFor(turn_))) {
        outcome_ = turn_based::drawResult("The player to move has no legal slide");
    }
    return true;
}

int FourByFourGame::evaluate(Token computer) const
{
    return evaluateBoard(cells_, computer, computer == Token::X ? Token::O : Token::X);
}

std::optional<MoveId> FourByFourGame::chooseComputerMove(std::mt19937&, const turn_based::CancelToken& cancel) const
{
    // The console's search: try every slide in board order (down, up, right, left for each token)
    // and score it with a two-ply minimax over the console's evaluation; the first best slide wins.
    if (outcome_.finished() || cancel.cancelled()) return std::nullopt;
    const Token computer = tokenFor(turn_);
    const Token human = tokenFor(turn_based::otherSeat(turn_));
    Board board = cells_;
    int bestScore = -10000;
    std::optional<MoveId> best;
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            if (at(board, row, column) != computer) continue;
            for (const auto& direction : directions) {
                if (!canSlide(board, row, column, direction)) continue;
                const int nextRow = row + direction[0];
                const int nextColumn = column + direction[1];
                set(board, row, column, Token::Empty);
                set(board, nextRow, nextColumn, computer);
                const int score = minimax(board, 2, false, computer, human);
                set(board, row, column, computer);
                set(board, nextRow, nextColumn, Token::Empty);
                if (score > bestScore) {
                    bestScore = score;
                    best = encode(row * 4 + column, nextRow * 4 + nextColumn);
                }
            }
        }
    }
    if (cancel.cancelled()) return std::nullopt;
    // Every slide scored the minimum (the opponent could always leave the computer stuck). The
    // console then sent an invalid fallback move; take the first legal slide instead.
    if (!best) {
        const auto moves = legalMoves();
        if (!moves.empty()) best = moves.front();
    }
    return best;
}

} // namespace four_by_four
