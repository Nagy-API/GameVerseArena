#include "PyramidGame.hpp"

namespace pyramid {
namespace {

std::optional<PyramidGame::Line> lineOf(const std::array<Mark, PyramidGame::cellCount>& cells, Mark mark)
{
    for (const auto& line : PyramidGame::lines()) {
        if (cells[static_cast<std::size_t>(line[0])] == mark && cells[static_cast<std::size_t>(line[1])] == mark &&
            cells[static_cast<std::size_t>(line[2])] == mark) {
            return line;
        }
    }
    return std::nullopt;
}

bool wouldWin(std::array<Mark, PyramidGame::cellCount> cells, int cell, Mark mark)
{
    cells[static_cast<std::size_t>(cell)] = mark;
    return lineOf(cells, mark).has_value();
}

} // namespace

const std::array<PyramidGame::Line, 7>& PyramidGame::lines()
{
    // Bottom-row windows, the middle row, the centre column, then the two sloped edges.
    static const std::array<Line, 7> all{{{0, 1, 2}, {1, 2, 3}, {2, 3, 4}, {5, 6, 7}, {2, 6, 8}, {0, 5, 8}, {4, 7, 8}}};
    return all;
}

PyramidGame::PyramidGame()
{
    reset();
}

std::unique_ptr<turn_based::TurnBasedGame> PyramidGame::clone() const
{
    return std::make_unique<PyramidGame>(*this);
}

void PyramidGame::reset()
{
    cells_.fill(Mark::Empty);
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
    winningLine_.reset();
}

bool PyramidGame::isLegal(MoveId move) const
{
    return !outcome_.finished() && move >= 0 && move < cellCount && cells_[static_cast<std::size_t>(move)] == Mark::Empty;
}

std::vector<MoveId> PyramidGame::legalMoves() const
{
    std::vector<MoveId> moves;
    if (outcome_.finished()) return moves;
    for (int index = 0; index < cellCount; ++index) {
        if (cells_[static_cast<std::size_t>(index)] == Mark::Empty) moves.push_back(index);
    }
    return moves;
}

bool PyramidGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    const Mark mark = markFor(turn_);
    cells_[static_cast<std::size_t>(move)] = mark;
    ++moves_;
    if (const auto line = lineOf(cells_, mark)) {
        winningLine_ = line;
        outcome_ = turn_based::winFor(turn_, "Three in a line");
        return true;
    }
    if (moves_ == cellCount) {
        outcome_ = turn_based::drawResult("The pyramid filled without a line");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    return true;
}

std::optional<MoveId> PyramidGame::chooseComputerMove(std::mt19937&, const turn_based::CancelToken& cancel) const
{
    // The console's priorities, each scanning from the bottom row up: win, block, first free cell.
    if (outcome_.finished() || cancel.cancelled()) return std::nullopt;
    const Mark own = markFor(turn_);
    const Mark opponent = markFor(turn_based::otherSeat(turn_));
    for (const Mark mark : {own, opponent}) {
        for (int index = 0; index < cellCount; ++index) {
            if (cells_[static_cast<std::size_t>(index)] == Mark::Empty && wouldWin(cells_, index, mark)) return index;
        }
    }
    for (int index = 0; index < cellCount; ++index) {
        if (cells_[static_cast<std::size_t>(index)] == Mark::Empty) return index;
    }
    return std::nullopt;
}

} // namespace pyramid
