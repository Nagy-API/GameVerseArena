#include "ObstacleGame.hpp"

#include <algorithm>
#include <utility>

namespace obstacle {
namespace {

constexpr int size = ObstacleGame::size;

// The run of four or more of `mark` through (row, column), or nullopt.
std::optional<ObstacleGame::Line> runThrough(const std::array<Cell, size * size>& cells, int row, int column, Cell mark)
{
    constexpr int directions[4][2]{{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    for (const auto& direction : directions) {
        ObstacleGame::Line run{row * size + column};
        for (const int sign : {-1, 1}) {
            for (int r = row + sign * direction[0], c = column + sign * direction[1];
                 r >= 0 && r < size && c >= 0 && c < size && cells[static_cast<std::size_t>(r * size + c)] == mark;
                 r += sign * direction[0], c += sign * direction[1]) {
                run.push_back(r * size + c);
            }
        }
        if (run.size() >= 4) {
            std::sort(run.begin(), run.end());
            return run;
        }
    }
    return std::nullopt;
}

} // namespace

ObstacleGame::ObstacleGame(std::uint32_t seed) : obstacles_(seed)
{
    reset();
}

std::unique_ptr<turn_based::TurnBasedGame> ObstacleGame::clone() const
{
    return std::make_unique<ObstacleGame>(*this);
}

void ObstacleGame::reset()
{
    cells_.fill(Cell::Empty);
    lastObstacles_.clear();
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
    winningLine_.reset();
}

bool ObstacleGame::isLegal(MoveId move) const
{
    return !outcome_.finished() && move >= 0 && move < size * size && cells_[static_cast<std::size_t>(move)] == Cell::Empty;
}

std::vector<MoveId> ObstacleGame::legalMoves() const
{
    std::vector<MoveId> moves;
    if (outcome_.finished()) return moves;
    for (int index = 0; index < size * size; ++index) {
        if (cells_[static_cast<std::size_t>(index)] == Cell::Empty) moves.push_back(index);
    }
    return moves;
}

void ObstacleGame::addObstacles(int count)
{
    // As in the console: pick among the empty cells (in board order) one at a time.
    std::vector<int> empty;
    for (int index = 0; index < size * size; ++index) {
        if (cells_[static_cast<std::size_t>(index)] == Cell::Empty) empty.push_back(index);
    }
    for (int placed = 0; placed < count && !empty.empty(); ++placed) {
        std::uniform_int_distribution<std::size_t> pick(0, empty.size() - 1);
        const auto chosen = static_cast<std::ptrdiff_t>(pick(obstacles_));
        const int cell = empty[static_cast<std::size_t>(chosen)];
        cells_[static_cast<std::size_t>(cell)] = Cell::Blocked;
        lastObstacles_.push_back(cell);
        empty.erase(empty.begin() + chosen);
    }
}

bool ObstacleGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    lastObstacles_.clear();
    const Cell mark = markFor(turn_);
    cells_[static_cast<std::size_t>(move)] = mark;
    ++moves_;
    // The console adds the obstacles before checking the move for a win; they never touch a mark.
    if (moves_ % 2 == 0) addObstacles(2);
    if (auto line = runThrough(cells_, move / size, move % size, mark)) {
        winningLine_ = std::move(line);
        outcome_ = turn_based::winFor(turn_, "Four in a row");
        return true;
    }
    if (std::none_of(cells_.begin(), cells_.end(), [](Cell value) { return value == Cell::Empty; })) {
        outcome_ = turn_based::drawResult("No empty cell is left");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    return true;
}

std::optional<MoveId> ObstacleGame::chooseComputerMove(std::mt19937& random, const turn_based::CancelToken& cancel) const
{
    // The console computer tries random cells until one is empty: a uniformly random empty cell.
    if (outcome_.finished() || cancel.cancelled()) return std::nullopt;
    const auto moves = legalMoves();
    if (moves.empty()) return std::nullopt;
    std::uniform_int_distribution<std::size_t> pick(0, moves.size() - 1);
    return moves[pick(random)];
}

} // namespace obstacle
