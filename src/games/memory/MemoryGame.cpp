#include "MemoryGame.hpp"

namespace memory_xo {
namespace {

constexpr std::array<MemoryGame::Line, 8> lines{{
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}}};

std::optional<MemoryGame::Line> lineOf(const std::array<Mark, 9>& cells, Mark mark)
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

MemoryGame::MemoryGame()
{
    reset();
}

std::unique_ptr<turn_based::TurnBasedGame> MemoryGame::clone() const
{
    return std::make_unique<MemoryGame>(*this);
}

void MemoryGame::reset()
{
    cells_.fill(Mark::Empty);
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
    winningLine_.reset();
}

bool MemoryGame::isLegal(MoveId move) const
{
    return !outcome_.finished() && move >= 0 && move < 9 && cells_[static_cast<std::size_t>(move)] == Mark::Empty;
}

std::vector<MoveId> MemoryGame::legalMoves() const
{
    std::vector<MoveId> moves;
    if (outcome_.finished()) return moves;
    for (int cell = 0; cell < 9; ++cell) {
        if (cells_[static_cast<std::size_t>(cell)] == Mark::Empty) moves.push_back(cell);
    }
    return moves;
}

bool MemoryGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    const Mark mark = markFor(turn_);
    cells_[static_cast<std::size_t>(move)] = mark;
    ++moves_;
    if (const auto line = lineOf(cells_, mark)) {
        winningLine_ = line;
        outcome_ = turn_based::winFor(turn_, "Three in a row");
        return true;
    }
    if (moves_ == 9) {
        outcome_ = turn_based::drawResult("Nine marks without a line");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    return true;
}

std::optional<MoveId> MemoryGame::chooseComputerMove(std::mt19937&, const turn_based::CancelToken& cancel) const
{
    // The console computer reads the hidden board and takes the first free cell, row by row.
    if (outcome_.finished() || cancel.cancelled()) return std::nullopt;
    for (int cell = 0; cell < 9; ++cell) {
        if (cells_[static_cast<std::size_t>(cell)] == Mark::Empty) return cell;
    }
    return std::nullopt;
}

} // namespace memory_xo
