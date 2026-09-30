#include "FourInRowGame.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <utility>

namespace four_in_a_row {
namespace {

using Board = std::array<Disc, FourInRowGame::rows * FourInRowGame::columns>;

constexpr int rows = FourInRowGame::rows;
constexpr int columns = FourInRowGame::columns;
constexpr int winScore = 10000000;
constexpr int loseScore = -10000000;
constexpr int maxDepth = 9;

Disc at(const Board& board, int row, int column)
{
    return board[static_cast<std::size_t>(row * columns + column)];
}

bool validColumn(const Board& board, int column)
{
    return column >= 0 && column < columns && at(board, 0, column) == Disc::Empty;
}

int openRow(const Board& board, int column)
{
    for (int row = rows - 1; row >= 0; --row) {
        if (at(board, row, column) == Disc::Empty) return row;
    }
    return -1;
}

Board drop(const Board& board, int column, Disc disc)
{
    Board next = board;
    const int row = openRow(next, column);
    if (row != -1) next[static_cast<std::size_t>(row * columns + column)] = disc;
    return next;
}

// Four in a row through the top disc of `column` (the disc just dropped). It counts only when
// that disc belongs to `disc`: the console counted from the cell whoever owned it, so another
// player's disc could complete a "four" for this one.
bool winsThrough(const Board& board, Disc disc, int column)
{
    if (column < 0) return false;
    int row = -1;
    for (int r = 0; r < rows; ++r) {
        if (at(board, r, column) != Disc::Empty) {
            row = r;
            break;
        }
    }
    if (row == -1 || at(board, row, column) != disc) return false;
    const auto run = [&](int rowStep, int columnStep) {
        int count = 0;
        for (int r = row + rowStep, c = column + columnStep;
             r >= 0 && r < rows && c >= 0 && c < columns && at(board, r, c) == disc; r += rowStep, c += columnStep) {
            ++count;
        }
        return count;
    };
    return 1 + run(0, -1) + run(0, 1) >= 4 || 1 + run(1, 0) >= 4 || 1 + run(-1, -1) + run(1, 1) >= 4 ||
           1 + run(-1, 1) + run(1, -1) >= 4;
}

bool boardFull(const Board& board)
{
    for (int column = 0; column < columns; ++column) {
        if (at(board, 0, column) == Disc::Empty) return false;
    }
    return true;
}

// The console's threat count: horizontal and vertical windows of four only.
int threats(const Board& board, Disc disc)
{
    int total = 0;
    const auto window = [&](int row, int column, int rowStep, int columnStep) {
        int count = 0;
        int empty = 0;
        for (int k = 0; k < 4; ++k) {
            const Disc value = at(board, row + rowStep * k, column + columnStep * k);
            if (value == disc) ++count;
            else if (value == Disc::Empty) ++empty;
        }
        if (count == 3 && empty == 1) total += 500;
        else if (count == 2 && empty == 2) total += 20;
    };
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < 4; ++column) window(row, column, 0, 1);
    }
    for (int column = 0; column < columns; ++column) {
        for (int row = 0; row < 3; ++row) window(row, column, 1, 0);
    }
    return total;
}

int evaluate(const Board& board, Disc own, Disc other)
{
    int score = 0;
    for (int row = 0; row < rows; ++row) {
        const Disc value = at(board, row, 3);
        if (value == own) score += 20;
        else if (value == other) score -= 18;
    }
    score += threats(board, own);
    // The console's `score -= threats * 1.2` converts the whole result back to an int.
    score = static_cast<int>(static_cast<double>(score) - static_cast<double>(threats(board, other)) * 1.2);
    return score;
}

std::vector<int> validColumns(const Board& board)
{
    std::vector<int> result;
    for (int column = 0; column < columns; ++column) {
        if (validColumn(board, column)) result.push_back(column);
    }
    return result;
}

std::optional<int> immediateWin(const Board& board, Disc disc)
{
    for (int column = 0; column < columns; ++column) {
        if (validColumn(board, column) && winsThrough(drop(board, column, disc), disc, column)) return column;
    }
    return std::nullopt;
}

// A drop that leaves `disc` two different ways to win next turn.
bool isFork(const Board& board, int column, Disc disc)
{
    if (!validColumn(board, column)) return false;
    const Board after = drop(board, column, disc);
    int wins = 0;
    for (int next = 0; next < columns; ++next) {
        if (!validColumn(after, next)) continue;
        if (winsThrough(drop(after, next, disc), disc, next)) ++wins;
        if (wins >= 2) return true;
    }
    return false;
}

class Search {
public:
    Search(Disc own, Disc other, long long budget, const turn_based::CancelToken& cancel)
        : own_(own), other_(other), budget_(budget), cancel_(cancel)
    {
    }

    // Where the console checked its 1.5-second clock. Once spent, the budget stays spent.
    bool spent()
    {
        if (!spent_ && (nodes_ >= budget_ || cancel_.cancelled())) spent_ = true;
        return spent_;
    }

    // The console's move ordering: winning drops, then double threats, then columns nearer the
    // centre. Equal scores put the higher column first, as the console's reverse sort did.
    std::vector<int> ordered(const Board& board) const
    {
        std::vector<std::pair<int, int>> scored;
        for (const int column : validColumns(board)) {
            int score = 0;
            if (winsThrough(drop(board, column, own_), own_, column)) score += 10000000;
            if (isFork(board, column, own_)) score += 1000000;
            score += (4 - std::abs(column - 3)) * 1000;
            scored.emplace_back(score, column);
        }
        std::sort(scored.rbegin(), scored.rend());
        std::vector<int> result;
        result.reserve(scored.size());
        for (const auto& entry : scored) result.push_back(entry.second);
        return result;
    }

    int minimax(const Board& board, int depth, int alpha, int beta, bool maximizing, int lastColumn)
    {
        ++nodes_;
        if (depth % 2 == 0 && spent()) return evaluate(board, own_, other_);
        if (lastColumn >= 0) {
            if (winsThrough(board, own_, lastColumn)) return winScore + depth;
            if (winsThrough(board, other_, lastColumn)) return loseScore - depth;
        }
        if (depth == 0 || boardFull(board)) return evaluate(board, own_, other_);

        const auto columnsToTry = ordered(board);
        if (maximizing) {
            int best = std::numeric_limits<int>::min();
            for (const int column : columnsToTry) {
                const int value = minimax(drop(board, column, own_), depth - 1, alpha, beta, false, column);
                best = std::max(best, value);
                alpha = std::max(alpha, value);
                if (beta <= alpha) break;
            }
            return best;
        }
        int best = std::numeric_limits<int>::max();
        for (const int column : columnsToTry) {
            const int value = minimax(drop(board, column, other_), depth - 1, alpha, beta, true, column);
            best = std::min(best, value);
            beta = std::min(beta, value);
            if (beta <= alpha) break;
        }
        return best;
    }

    // Iterative deepening at depths 4, 6, and 8. A depth that ran out of budget is discarded and
    // the previous depth's choice is kept.
    int bestColumn(const Board& board)
    {
        int best = 3;
        for (int depth = 4; depth <= maxDepth && !spent(); depth += 2) {
            const auto columnsToTry = ordered(board);
            int depthBest = best;
            int depthScore = std::numeric_limits<int>::min();
            for (const int column : columnsToTry) {
                if (spent()) break;
                const int score = minimax(drop(board, column, own_), depth - 1, std::numeric_limits<int>::min(),
                                          std::numeric_limits<int>::max(), false, column);
                if (score > depthScore) {
                    depthScore = score;
                    depthBest = column;
                }
            }
            if (!spent()) {
                best = depthBest;
                completedDepth_ = depth;
            }
        }
        // The console fell back to the centre column even if no depth finished; never pick a full column.
        if (!validColumn(board, best)) {
            const auto columnsToTry = ordered(board);
            if (!columnsToTry.empty()) best = columnsToTry.front();
        }
        return best;
    }

    long long nodes() const noexcept { return nodes_; }
    int completedDepth() const noexcept { return completedDepth_; }

private:
    Disc own_;
    Disc other_;
    long long budget_;
    const turn_based::CancelToken& cancel_;
    long long nodes_{0};
    int completedDepth_{0};
    bool spent_{false};
};

// The cells of the run of four or more through (row, column), or nullopt.
std::optional<FourInRowGame::Line> runThrough(const Board& board, int row, int column, Disc disc)
{
    constexpr int directions[4][2]{{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    for (const auto& direction : directions) {
        FourInRowGame::Line cells{row * columns + column};
        for (const int sign : {-1, 1}) {
            for (int r = row + sign * direction[0], c = column + sign * direction[1];
                 r >= 0 && r < rows && c >= 0 && c < columns && at(board, r, c) == disc;
                 r += sign * direction[0], c += sign * direction[1]) {
                cells.push_back(r * columns + c);
            }
        }
        if (cells.size() >= 4) {
            std::sort(cells.begin(), cells.end());
            return cells;
        }
    }
    return std::nullopt;
}

} // namespace

FourInRowGame::FourInRowGame(long long searchBudget) : searchBudget_(searchBudget)
{
    reset();
}

std::unique_ptr<turn_based::TurnBasedGame> FourInRowGame::clone() const
{
    return std::make_unique<FourInRowGame>(*this);
}

void FourInRowGame::reset()
{
    cells_.fill(Disc::Empty);
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
    winningLine_.reset();
}

int FourInRowGame::landingRow(int column) const
{
    if (column < 0 || column >= columns) return -1;
    return openRow(cells_, column);
}

bool FourInRowGame::isLegal(MoveId move) const
{
    return !outcome_.finished() && validColumn(cells_, move);
}

std::vector<MoveId> FourInRowGame::legalMoves() const
{
    std::vector<MoveId> moves;
    if (outcome_.finished()) return moves;
    for (const int column : validColumns(cells_)) moves.push_back(column);
    return moves;
}

bool FourInRowGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    const int row = openRow(cells_, move);
    const Disc disc = discFor(turn_);
    cells_[static_cast<std::size_t>(indexOf(row, move))] = disc;
    ++moves_;
    if (auto line = runThrough(cells_, row, move, disc)) {
        winningLine_ = std::move(line);
        outcome_ = turn_based::winFor(turn_, "Four in a row");
        return true;
    }
    if (moves_ == rows * columns) {
        outcome_ = turn_based::drawResult("The grid is full");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    return true;
}

FourInRowGame::SearchReport FourInRowGame::analyse(const turn_based::CancelToken& cancel) const
{
    SearchReport report;
    if (outcome_.finished() || cancel.cancelled()) return report;
    const Disc own = discFor(turn_);
    const Disc other = discFor(turn_based::otherSeat(turn_));
    const auto decided = [&report](int column, const char* rule) {
        report.move = column;
        report.rule = rule;
        return report;
    };
    // The console's opening book: each computer player's first disc goes in the centre column.
    if (moves_ < 2) return decided(3, "opening");
    if (const auto column = immediateWin(cells_, own)) return decided(*column, "win");
    for (int column = 0; column < columns; ++column) {
        if (isFork(cells_, column, own)) return decided(column, "fork");
    }
    if (const auto column = immediateWin(cells_, other)) return decided(*column, "block");
    for (int column = 0; column < columns; ++column) {
        if (isFork(cells_, column, other)) return decided(column, "block fork");
    }
    Search search(own, other, searchBudget_, cancel);
    const int column = search.bestColumn(cells_);
    report.nodes = search.nodes();
    report.completedDepth = search.completedDepth();
    if (cancel.cancelled()) return report;
    report.move = column;
    report.rule = "search";
    return report;
}

std::optional<MoveId> FourInRowGame::chooseComputerMove(std::mt19937&, const turn_based::CancelToken& cancel) const
{
    return analyse(cancel).move;
}

} // namespace four_in_a_row
