#include "DiamondGame.hpp"

#include <cstdlib>
#include <utility>

namespace diamond {
namespace {

constexpr int size = DiamondGame::size;
using Board = std::array<Mark, size * size>;

// The four directions a line can run in: vertical, horizontal, and the two diagonals.
constexpr int axes[4][2]{{1, 0}, {0, 1}, {1, 1}, {1, -1}};

Mark at(const Board& board, int row, int column)
{
    return board[static_cast<std::size_t>(row * size + column)];
}

bool inGrid(int row, int column)
{
    return row >= 0 && row < size && column >= 0 && column < size;
}

// Every maximal line of `mark` along one axis, as grid indices.
std::vector<std::vector<int>> linesAlong(const Board& board, Mark mark, int rowStep, int columnStep)
{
    std::vector<std::vector<int>> result;
    for (int row = 0; row < size; ++row) {
        for (int column = 0; column < size; ++column) {
            if (at(board, row, column) != mark) continue;
            const int previousRow = row - rowStep;
            const int previousColumn = column - columnStep;
            if (inGrid(previousRow, previousColumn) && at(board, previousRow, previousColumn) == mark) continue;
            std::vector<int> line;
            for (int r = row, c = column; inGrid(r, c) && at(board, r, c) == mark; r += rowStep, c += columnStep) {
                line.push_back(r * size + c);
            }
            result.push_back(std::move(line));
        }
    }
    return result;
}

// A line of exactly three and a line of exactly four of `mark`, in different directions.
std::optional<DiamondGame::WinningLines> winningPair(const Board& board, Mark mark)
{
    std::array<std::vector<int>, 4> three;
    std::array<std::vector<int>, 4> four;
    for (std::size_t axis = 0; axis < 4; ++axis) {
        for (auto& line : linesAlong(board, mark, axes[axis][0], axes[axis][1])) {
            if (line.size() == 3 && three[axis].empty()) three[axis] = line;
            if (line.size() == 4 && four[axis].empty()) four[axis] = line;
        }
    }
    for (std::size_t fourAxis = 0; fourAxis < 4; ++fourAxis) {
        if (four[fourAxis].empty()) continue;
        for (std::size_t threeAxis = 0; threeAxis < 4; ++threeAxis) {
            if (threeAxis != fourAxis && !three[threeAxis].empty()) {
                return DiamondGame::WinningLines{three[threeAxis], four[fourAxis]};
            }
        }
    }
    return std::nullopt;
}

// The console's count_line: the length of the run of `mark` through (row, column) along a
// direction, counting that cell.
int countLine(const Board& board, int row, int column, int rowStep, int columnStep, Mark mark)
{
    int total = 1;
    for (int r = row + rowStep, c = column + columnStep; inGrid(r, c) && at(board, r, c) == mark;
         r += rowStep, c += columnStep) {
        ++total;
    }
    for (int r = row - rowStep, c = column - columnStep; inGrid(r, c) && at(board, r, c) == mark;
         r -= rowStep, c -= columnStep) {
        ++total;
    }
    return total;
}

// The console's evaluate(): a win scores 100000; otherwise every mark adds, in each of the eight
// directions, 500 for a line of four or more, 120 for three, and 25 for two.
int evaluateFor(const Board& board, Mark mark)
{
    if (winningPair(board, mark)) return 100000;
    constexpr int rowSteps[8]{1, -1, 0, 0, 1, 1, -1, -1};
    constexpr int columnSteps[8]{0, 0, 1, -1, 1, -1, 1, -1};
    int score = 0;
    for (int row = 0; row < size; ++row) {
        for (int column = 0; column < size; ++column) {
            if (at(board, row, column) != mark) continue;
            for (int direction = 0; direction < 8; ++direction) {
                const int length = countLine(board, row, column, rowSteps[direction], columnSteps[direction], mark);
                if (length >= 4) score += 500;
                else if (length == 3) score += 120;
                else if (length == 2) score += 25;
            }
        }
    }
    return score;
}

} // namespace

bool DiamondGame::onBoard(int row, int column) noexcept
{
    if (row < 0 || row >= size || column < 0 || column >= size) return false;
    const int halfWidth = 3 - std::abs(row - 3);
    return std::abs(column - 3) <= halfWidth;
}

DiamondGame::DiamondGame()
{
    reset();
}

std::unique_ptr<turn_based::TurnBasedGame> DiamondGame::clone() const
{
    return std::make_unique<DiamondGame>(*this);
}

void DiamondGame::reset()
{
    cells_.fill(Mark::Empty);
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
    winningLines_.reset();
}

bool DiamondGame::isLegal(MoveId move) const
{
    return !outcome_.finished() && move >= 0 && move < size * size && onBoard(move / size, move % size) &&
           cells_[static_cast<std::size_t>(move)] == Mark::Empty;
}

std::vector<MoveId> DiamondGame::legalMoves() const
{
    std::vector<MoveId> moves;
    if (outcome_.finished()) return moves;
    for (int index = 0; index < size * size; ++index) {
        if (isLegal(index)) moves.push_back(index);
    }
    return moves;
}

bool DiamondGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    const Mark mark = markFor(turn_);
    cells_[static_cast<std::size_t>(move)] = mark;
    ++moves_;
    if (auto lines = winningPair(cells_, mark)) {
        winningLines_ = std::move(lines);
        outcome_ = turn_based::winFor(turn_, "A line of three and a line of four");
        return true;
    }
    if (moves_ == cellCount) {
        outcome_ = turn_based::drawResult("The diamond filled without a winner");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    return true;
}

int DiamondGame::evaluate(Mark mark) const
{
    return evaluateFor(cells_, mark);
}

std::optional<MoveId> DiamondGame::chooseComputerMove(std::mt19937&, const turn_based::CancelToken& cancel) const
{
    // The console's priorities, scanning rows then columns: win now, block the opponent's win,
    // otherwise the cell that most strengthens its own lines, preferring the centre.
    if (outcome_.finished() || cancel.cancelled()) return std::nullopt;
    const Mark own = markFor(turn_);
    const Mark opponent = markFor(turn_based::otherSeat(turn_));
    Board board = cells_;
    const auto empty = [&](int index) { return onBoard(index / size, index % size) && board[static_cast<std::size_t>(index)] == Mark::Empty; };

    for (const Mark mark : {own, opponent}) {
        for (int index = 0; index < size * size; ++index) {
            if (!empty(index)) continue;
            board[static_cast<std::size_t>(index)] = mark;
            const bool wins = winningPair(board, mark).has_value();
            board[static_cast<std::size_t>(index)] = Mark::Empty;
            if (wins) return index;
        }
    }

    int bestScore = -999999;
    std::optional<MoveId> best;
    for (int index = 0; index < size * size; ++index) {
        if (!empty(index)) continue;
        board[static_cast<std::size_t>(index)] = own;
        const int ownScore = evaluateFor(board, own);
        const int opponentScore = evaluateFor(board, opponent);
        board[static_cast<std::size_t>(index)] = Mark::Empty;
        const int distance = std::abs(index / size - 3) + std::abs(index % size - 3);
        const int centreBonus = distance == 0 ? 80 : distance == 1 ? 50 : distance == 2 ? 25 : 5;
        const int total = ownScore - opponentScore + centreBonus;
        if (total > bestScore) {
            bestScore = total;
            best = index;
        }
    }
    if (cancel.cancelled()) return std::nullopt;
    return best;
}

} // namespace diamond
