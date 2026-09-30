#include "FiveByFiveGame.hpp"

namespace five_by_five {
namespace {
// All 48 runs of three on a 5x5 board, in the console's counting order: rows, columns,
// down-right diagonals, then up-right diagonals.
const std::vector<FiveByFiveGame::Triple>& allTriples()
{
    static const std::vector<FiveByFiveGame::Triple> triples = [] {
        std::vector<FiveByFiveGame::Triple> result;
        const auto at = [](int row, int column) { return row * 5 + column; };
        for (int row = 0; row < 5; ++row)
            for (int column = 0; column < 3; ++column)
                result.push_back({at(row, column), at(row, column + 1), at(row, column + 2)});
        for (int column = 0; column < 5; ++column)
            for (int row = 0; row < 3; ++row)
                result.push_back({at(row, column), at(row + 1, column), at(row + 2, column)});
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 3; ++column)
                result.push_back({at(row, column), at(row + 1, column + 1), at(row + 2, column + 2)});
        for (int row = 2; row < 5; ++row)
            for (int column = 0; column < 3; ++column)
                result.push_back({at(row, column), at(row - 1, column + 1), at(row - 2, column + 2)});
        return result;
    }();
    return triples;
}

int countTriples(const std::array<Mark, 25>& cells, Mark mark)
{
    int count = 0;
    for (const auto& triple : allTriples()) {
        if (cells[static_cast<std::size_t>(triple[0])] == mark && cells[static_cast<std::size_t>(triple[1])] == mark &&
            cells[static_cast<std::size_t>(triple[2])] == mark) {
            ++count;
        }
    }
    return count;
}
} // namespace

FiveByFiveGame::FiveByFiveGame()
{
    reset();
}

std::unique_ptr<turn_based::TurnBasedGame> FiveByFiveGame::clone() const
{
    return std::make_unique<FiveByFiveGame>(*this);
}

void FiveByFiveGame::reset()
{
    cells_.fill(Mark::Empty);
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
}

int FiveByFiveGame::triples(Mark mark) const
{
    return countTriples(cells_, mark);
}

std::vector<FiveByFiveGame::Triple> FiveByFiveGame::triplesOf(Mark mark) const
{
    std::vector<Triple> result;
    for (const auto& triple : allTriples()) {
        if (cells_[static_cast<std::size_t>(triple[0])] == mark && cells_[static_cast<std::size_t>(triple[1])] == mark &&
            cells_[static_cast<std::size_t>(triple[2])] == mark) {
            result.push_back(triple);
        }
    }
    return result;
}

bool FiveByFiveGame::isLegal(MoveId move) const
{
    return !outcome_.finished() && move >= 0 && move < 25 && cells_[static_cast<std::size_t>(move)] == Mark::Empty;
}

std::vector<MoveId> FiveByFiveGame::legalMoves() const
{
    std::vector<MoveId> moves;
    if (outcome_.finished()) return moves;
    for (int index = 0; index < 25; ++index) {
        if (cells_[static_cast<std::size_t>(index)] == Mark::Empty) moves.push_back(index);
    }
    return moves;
}

bool FiveByFiveGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    cells_[static_cast<std::size_t>(move)] = markFor(turn_);
    ++moves_;
    if (moves_ == finalMoveCount) {
        const int x = triples(Mark::X);
        const int o = triples(Mark::O);
        if (x > o) outcome_ = turn_based::winFor(Seat::First, "More three-in-a-rows");
        else if (o > x) outcome_ = turn_based::winFor(Seat::Second, "More three-in-a-rows");
        else outcome_ = turn_based::drawResult("Equal three-in-a-row counts");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    return true;
}

std::optional<turn_based::SeatScores> FiveByFiveGame::scores() const
{
    return turn_based::SeatScores{triples(Mark::X), triples(Mark::O)};
}

std::optional<MoveId> FiveByFiveGame::chooseComputerMove(std::mt19937&, const turn_based::CancelToken& cancel) const
{
    // The console's priorities (find_best_ai_move): make a three, block the opponent's three,
    // take the center, then a corner, then the first free cell. A cell "makes a three" when it
    // adds to the mark's count. The console tested whole-board totals instead: once the computer
    // held any three its first check passed on the first free cell, and once the opponent held
    // any three the blocking check did the same whenever the computer had no new three to make.
    if (outcome_.finished() || cancel.cancelled()) return std::nullopt;
    const Mark own = markFor(turn_);
    const Mark opponent = markFor(turn_based::otherSeat(turn_));
    const auto gains = [&](int index, Mark mark) {
        auto board = cells_;
        board[static_cast<std::size_t>(index)] = mark;
        return countTriples(board, mark) > countTriples(cells_, mark);
    };
    for (int index = 0; index < 25; ++index)
        if (cells_[static_cast<std::size_t>(index)] == Mark::Empty && gains(index, own)) return index;
    for (int index = 0; index < 25; ++index)
        if (cells_[static_cast<std::size_t>(index)] == Mark::Empty && gains(index, opponent)) return index;
    if (cells_[12] == Mark::Empty) return 12;
    for (const int corner : {0, 4, 20, 24})
        if (cells_[static_cast<std::size_t>(corner)] == Mark::Empty) return corner;
    for (int index = 0; index < 25; ++index)
        if (cells_[static_cast<std::size_t>(index)] == Mark::Empty) return index;
    return std::nullopt;
}

} // namespace five_by_five
