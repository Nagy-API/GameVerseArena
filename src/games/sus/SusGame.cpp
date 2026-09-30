#include "SusGame.hpp"

#include <climits>

namespace sus_game {
namespace {
bool formsSus(char a, char b, char c) noexcept
{
    return a == 'S' && b == 'U' && c == 'S';
}

// Counts S-U-S lines through (row, column) on `board`, exactly as the console's
// count_SUS_last_move and SUS_AI::count_potential_SUS do: the row, the column, and each
// diagonal the cell lies on.
int susThrough(const std::array<char, 9>& board, int row, int column)
{
    const auto at = [&](int r, int c) { return board[static_cast<std::size_t>(r * 3 + c)]; };
    int count = 0;
    if (formsSus(at(row, 0), at(row, 1), at(row, 2))) ++count;
    if (formsSus(at(0, column), at(1, column), at(2, column))) ++count;
    if (row == column && formsSus(at(0, 0), at(1, 1), at(2, 2))) ++count;
    if (row + column == 2 && formsSus(at(0, 2), at(1, 1), at(2, 0))) ++count;
    return count;
}
} // namespace

SusGame::SusGame()
{
    reset();
}

std::unique_ptr<turn_based::TurnBasedGame> SusGame::clone() const
{
    return std::make_unique<SusGame>(*this);
}

void SusGame::reset()
{
    cells_.fill('.');
    scoreS_ = 0;
    scoreU_ = 0;
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
}

bool SusGame::isLegal(MoveId move) const
{
    return !outcome_.finished() && move >= 0 && move < 9 && cells_[static_cast<std::size_t>(move)] == '.';
}

std::vector<MoveId> SusGame::legalMoves() const
{
    std::vector<MoveId> moves;
    if (outcome_.finished()) return moves;
    for (int index = 0; index < 9; ++index) {
        if (cells_[static_cast<std::size_t>(index)] == '.') moves.push_back(index);
    }
    return moves;
}

bool SusGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    const char letter = letterFor(turn_);
    cells_[static_cast<std::size_t>(move)] = letter;
    ++moves_;
    const int gained = susThrough(cells_, move / 3, move % 3);
    if (letter == 'S') scoreS_ += gained;
    else scoreU_ += gained;

    if (moves_ == 9) {
        if (scoreS_ > scoreU_) outcome_ = turn_based::winFor(Seat::First, "More S-U-S lines");
        else if (scoreU_ > scoreS_) outcome_ = turn_based::winFor(Seat::Second, "More S-U-S lines");
        else outcome_ = turn_based::drawResult("Equal S-U-S scores");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    return true;
}

std::optional<turn_based::SeatScores> SusGame::scores() const
{
    return turn_based::SeatScores{scoreS_, scoreU_};
}

std::vector<SusGame::Line> SusGame::susLines() const
{
    static constexpr std::array<Line, 8> lines{{
        {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}}};
    std::vector<Line> result;
    for (const auto& line : lines) {
        if (formsSus(cells_[static_cast<std::size_t>(line[0])], cells_[static_cast<std::size_t>(line[1])],
                     cells_[static_cast<std::size_t>(line[2])])) {
            result.push_back(line);
        }
    }
    return result;
}

int SusGame::countSusThrough(int index, char letter) const
{
    auto board = cells_;
    board[static_cast<std::size_t>(index)] = letter;
    return susThrough(board, index / 3, index % 3);
}

int SusGame::evaluateMove(int index, char letter) const
{
    // SUS_AI::evaluate_move: completed lines, then position, then partial S/U patterns in the
    // placed cell's row and column.
    const int row = index / 3;
    const int column = index % 3;
    int score = countSusThrough(index, letter) * 100;
    if (row == 1 && column == 1) score += 30;
    else if ((row == 0 || row == 2) && (column == 0 || column == 2)) score += 20;
    else score += 10;

    auto board = cells_;
    board[static_cast<std::size_t>(index)] = letter;
    const auto pattern = [&](auto cellAt) {
        int s = 0;
        int u = 0;
        int empty = 0;
        for (int step = 0; step < 3; ++step) {
            const char value = cellAt(step);
            if (value == 'S') ++s;
            else if (value == 'U') ++u;
            else ++empty;
        }
        if (s == 2 && u == 1) return 50;
        if (s == 1 && u == 1 && empty == 1) return 25;
        return 0;
    };
    score += pattern([&](int c) { return board[static_cast<std::size_t>(row * 3 + c)]; });
    score += pattern([&](int r) { return board[static_cast<std::size_t>(r * 3 + column)]; });
    return score;
}

std::optional<MoveId> SusGame::chooseComputerMove(std::mt19937&, const turn_based::CancelToken& cancel) const
{
    // SUS_AI::get_best_move: offensive evaluation plus 80 per S-U-S the opponent's letter would
    // complete in the same cell; the first highest-scoring empty cell in row-major order wins.
    if (outcome_.finished() || cancel.cancelled()) return std::nullopt;
    const char own = letterFor(turn_);
    const char opponent = letterFor(turn_based::otherSeat(turn_));
    std::optional<MoveId> best;
    int bestScore = INT_MIN;
    for (int index = 0; index < 9; ++index) {
        if (cells_[static_cast<std::size_t>(index)] != '.') continue;
        const int total = evaluateMove(index, own) + countSusThrough(index, opponent) * 80;
        if (total > bestScore) {
            bestScore = total;
            best = index;
        }
    }
    return best;
}

} // namespace sus_game
