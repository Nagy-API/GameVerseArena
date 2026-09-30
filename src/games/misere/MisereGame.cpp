#include "MisereGame.hpp"

#include <algorithm>

namespace misere {
namespace {
constexpr std::array<MisereGame::Line, 8> lines{{
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}}};

std::optional<MisereGame::Line> lineOf(const std::array<Mark, 9>& cells, Mark mark)
{
    for (const auto& line : lines) {
        if (cells[static_cast<std::size_t>(line[0])] == mark && cells[static_cast<std::size_t>(line[1])] == mark &&
            cells[static_cast<std::size_t>(line[2])] == mark) {
            return line;
        }
    }
    return std::nullopt;
}

// The console's minimax (Misere_Board.cpp): a line for the computer's own mark scores
// -10 + depth, a line for the opponent's mark 10 - depth, a full board 0. Full depth, no
// pruning, row-major move order.
struct Search {
    Mark computer;
    Mark opponent;
    const turn_based::CancelToken& cancel;
    long long nodes{0};
    bool stopped{false};

    int minimax(std::array<Mark, 9>& cells, int filled, bool maximizing, int depth)
    {
        if ((++nodes & 1023) == 0 && cancel.cancelled()) stopped = true;
        if (stopped) return 0;
        if (lineOf(cells, computer)) return -10 + depth;
        if (lineOf(cells, opponent)) return 10 - depth;
        if (filled == 9) return 0;
        int best = maximizing ? -100000 : 100000;
        for (std::size_t index = 0; index < 9; ++index) {
            if (cells[index] != Mark::Empty) continue;
            cells[index] = maximizing ? computer : opponent;
            const int score = minimax(cells, filled + 1, !maximizing, depth + 1);
            cells[index] = Mark::Empty;
            best = maximizing ? std::max(best, score) : std::min(best, score);
        }
        return best;
    }
};
} // namespace

MisereGame::MisereGame()
{
    reset();
}

std::unique_ptr<turn_based::TurnBasedGame> MisereGame::clone() const
{
    return std::make_unique<MisereGame>(*this);
}

void MisereGame::reset()
{
    cells_.fill(Mark::Empty);
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
    losingLine_.reset();
}

bool MisereGame::isLegal(MoveId move) const
{
    return !outcome_.finished() && move >= 0 && move < 9 && cells_[static_cast<std::size_t>(move)] == Mark::Empty;
}

std::vector<MoveId> MisereGame::legalMoves() const
{
    std::vector<MoveId> moves;
    if (outcome_.finished()) return moves;
    for (int index = 0; index < 9; ++index) {
        if (cells_[static_cast<std::size_t>(index)] == Mark::Empty) moves.push_back(index);
    }
    return moves;
}

bool MisereGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    const Mark mark = markFor(turn_);
    cells_[static_cast<std::size_t>(move)] = mark;
    ++moves_;
    if (const auto line = lineOf(cells_, mark)) {
        losingLine_ = line;
        outcome_ = turn_based::winFor(turn_based::otherSeat(turn_), "The opponent completed three in a row");
        return true;
    }
    if (moves_ == 9) {
        outcome_ = turn_based::drawResult("The grid filled without anyone completing a line");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    return true;
}

std::optional<MoveId> MisereGame::chooseComputerMove(std::mt19937&, const turn_based::CancelToken& cancel) const
{
    if (outcome_.finished() || cancel.cancelled()) return std::nullopt;
    Search search{markFor(turn_), markFor(turn_based::otherSeat(turn_)), cancel};
    auto cells = cells_;
    std::optional<MoveId> best;
    int bestScore = -100000;
    for (std::size_t index = 0; index < 9; ++index) {
        if (cells[index] != Mark::Empty) continue;
        cells[index] = search.computer;
        const int score = search.minimax(cells, moves_ + 1, false, 0);
        cells[index] = Mark::Empty;
        if (search.stopped) return std::nullopt;
        if (score > bestScore) {
            bestScore = score;
            best = static_cast<MoveId>(index);
        }
    }
    return best;
}

} // namespace misere
