#include "NumericalGame.hpp"

namespace numerical_ttt {
namespace {
constexpr std::array<NumericalGame::Line, 8> lines{{
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}}};
} // namespace

NumericalGame::NumericalGame()
{
    reset();
}

bool NumericalGame::numberBelongsTo(int number, Seat seat) noexcept
{
    if (number < 1 || number > 9) return false;
    return seat == Seat::First ? number % 2 == 1 : number % 2 == 0;
}

bool NumericalGame::numberUsed(int number) const noexcept
{
    return number >= 1 && number <= 9 && used_[static_cast<std::size_t>(number)];
}

std::vector<int> NumericalGame::availableNumbers(Seat seat) const
{
    std::vector<int> numbers;
    for (int number = 1; number <= 9; ++number) {
        if (numberBelongsTo(number, seat) && !numberUsed(number)) numbers.push_back(number);
    }
    return numbers;
}

std::unique_ptr<turn_based::TurnBasedGame> NumericalGame::clone() const
{
    return std::make_unique<NumericalGame>(*this);
}

void NumericalGame::reset()
{
    cells_.fill(0);
    used_.fill(false);
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
    winningLine_.reset();
}

bool NumericalGame::isLegal(MoveId move) const
{
    if (outcome_.finished() || move < 0) return false;
    const int index = cellOf(move);
    const int number = numberOf(move);
    return index >= 0 && index < 9 && cells_[static_cast<std::size_t>(index)] == 0 &&
           numberBelongsTo(number, turn_) && !numberUsed(number);
}

std::vector<MoveId> NumericalGame::legalMoves() const
{
    std::vector<MoveId> moves;
    if (outcome_.finished()) return moves;
    const auto numbers = availableNumbers(turn_);
    for (int index = 0; index < 9; ++index) {
        if (cells_[static_cast<std::size_t>(index)] != 0) continue;
        for (const int number : numbers) moves.push_back(encode(index, number));
    }
    return moves;
}

bool NumericalGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    const int number = numberOf(move);
    cells_[static_cast<std::size_t>(cellOf(move))] = number;
    used_[static_cast<std::size_t>(number)] = true;
    ++moves_;
    // Legacy rule: any full line summing to 15 ends the game in the mover's favour.
    for (const auto& line : lines) {
        const int a = cells_[static_cast<std::size_t>(line[0])];
        const int b = cells_[static_cast<std::size_t>(line[1])];
        const int c = cells_[static_cast<std::size_t>(line[2])];
        if (a != 0 && b != 0 && c != 0 && a + b + c == 15) {
            winningLine_ = line;
            outcome_ = turn_based::winFor(turn_, "Completed a line that adds up to 15");
            return true;
        }
    }
    if (moves_ == 9) {
        outcome_ = turn_based::drawResult("All nine cells filled without a line of 15");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    return true;
}

std::optional<MoveId> NumericalGame::chooseComputerMove(std::mt19937& random,
                                                       const turn_based::CancelToken& cancel) const
{
    // The console computer retried random cells and random numbers of its parity until the move
    // was valid, which picks uniformly among the legal (cell, number) pairs.
    if (cancel.cancelled()) return std::nullopt;
    const auto moves = legalMoves();
    if (moves.empty()) return std::nullopt;
    std::uniform_int_distribution<std::size_t> pick(0, moves.size() - 1);
    return moves[pick(random)];
}

} // namespace numerical_ttt
