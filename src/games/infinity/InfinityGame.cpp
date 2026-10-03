#include "InfinityGame.hpp"

namespace infinity_xo {
namespace {

constexpr std::array<InfinityGame::Line, 8> lines{{
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}}};

std::optional<InfinityGame::Line> lineOf(const std::array<Mark, 9>& cells, Mark mark)
{
    for (const auto& line : lines) {
        if (cells[static_cast<std::size_t>(line[0])] == mark && cells[static_cast<std::size_t>(line[1])] == mark &&
            cells[static_cast<std::size_t>(line[2])] == mark) {
            return line;
        }
    }
    return std::nullopt;
}

} // namespace

InfinityGame::InfinityGame()
{
    reset();
}

std::unique_ptr<turn_based::TurnBasedGame> InfinityGame::clone() const
{
    return std::make_unique<InfinityGame>(*this);
}

void InfinityGame::reset()
{
    cells_.fill(Mark::Empty);
    history_.clear();
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
    winningLine_.reset();
    lastRemoved_.reset();
    lastRemovedMark_ = Mark::Empty;
}

std::optional<int> InfinityGame::nextToVanish() const
{
    if (outcome_.finished() || !removesOldestAfter(moves_ + 1) || history_.empty()) return std::nullopt;
    return history_.front();
}

bool InfinityGame::isLegal(MoveId move) const
{
    return !outcome_.finished() && move >= 0 && move < 9 && cells_[static_cast<std::size_t>(move)] == Mark::Empty;
}

std::vector<MoveId> InfinityGame::legalMoves() const
{
    std::vector<MoveId> moves;
    if (outcome_.finished()) return moves;
    for (int cell = 0; cell < 9; ++cell) {
        if (cells_[static_cast<std::size_t>(cell)] == Mark::Empty) moves.push_back(cell);
    }
    return moves;
}

bool InfinityGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    lastRemoved_.reset();
    lastRemovedMark_ = Mark::Empty;
    const Mark mark = markFor(turn_);
    cells_[static_cast<std::size_t>(move)] = mark;
    ++moves_;
    history_.push_back(move);
    // As in the console, the oldest mark goes before the move is checked for a win.
    if (removesOldestAfter(moves_) && history_.size() >= 3) {
        const int oldest = history_.front();
        history_.pop_front();
        lastRemovedMark_ = cells_[static_cast<std::size_t>(oldest)];
        cells_[static_cast<std::size_t>(oldest)] = Mark::Empty;
        lastRemoved_ = oldest;
    }
    if (const auto line = lineOf(cells_, mark)) {
        winningLine_ = line;
        outcome_ = turn_based::winFor(turn_, "Three in a row");
        return true;
    }
    if (moves_ >= finalMove) {
        outcome_ = turn_based::drawResult("The ninth move did not complete a line");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    return true;
}

std::optional<MoveId> InfinityGame::chooseComputerMove(std::mt19937& random, const turn_based::CancelToken& cancel) const
{
    // The console computer tries random cells until one is empty: a uniformly random empty cell.
    if (outcome_.finished() || cancel.cancelled()) return std::nullopt;
    const auto moves = legalMoves();
    if (moves.empty()) return std::nullopt;
    std::uniform_int_distribution<std::size_t> pick(0, moves.size() - 1);
    return moves[pick(random)];
}

} // namespace infinity_xo
