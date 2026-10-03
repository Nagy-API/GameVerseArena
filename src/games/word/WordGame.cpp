#include "WordGame.hpp"

namespace word_ttt {
namespace {

// Rows (left to right), columns (top to bottom), then the two diagonals (downward), as the
// console reads them.
constexpr std::array<WordGame::Line, 8> lines{{
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}}};

std::optional<WordGame::Line> wordLine(const std::array<char, 9>& cells)
{
    for (const auto& line : lines) {
        std::string word;
        for (const int cell : line) word.push_back(cells[static_cast<std::size_t>(cell)]);
        if (word.find('.') == std::string::npos && WordGame::dictionary().count(word) > 0) return line;
    }
    return std::nullopt;
}

} // namespace

const std::set<std::string>& WordGame::dictionary()
{
    static const std::set<std::string> words{"CAT", "DOG", "SUN", "YES", "YOU", "ONE", "TWO", "CAR",
                                             "BUS", "BOX", "BED", "RED", "BIG", "MAN", "FUN", "RUN",
                                             "WIN", "DAY", "EAT", "TEA", "SEA", "SKY"};
    return words;
}

WordGame::WordGame()
{
    reset();
}

std::unique_ptr<turn_based::TurnBasedGame> WordGame::clone() const
{
    return std::make_unique<WordGame>(*this);
}

void WordGame::reset()
{
    cells_.fill('.');
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
    winningLine_.reset();
}

std::string WordGame::winningWord() const
{
    std::string word;
    if (winningLine_) {
        for (const int cell : *winningLine_) word.push_back(cells_[static_cast<std::size_t>(cell)]);
    }
    return word;
}

bool WordGame::isLegal(MoveId move) const
{
    return !outcome_.finished() && move >= 0 && move < 9 * 26 && cells_[static_cast<std::size_t>(cellOf(move))] == '.';
}

std::vector<MoveId> WordGame::legalMoves() const
{
    std::vector<MoveId> moves;
    if (outcome_.finished()) return moves;
    for (int cell = 0; cell < 9; ++cell) {
        if (cells_[static_cast<std::size_t>(cell)] != '.') continue;
        for (char letter = 'A'; letter <= 'Z'; ++letter) moves.push_back(encode(cell, letter));
    }
    return moves;
}

bool WordGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    cells_[static_cast<std::size_t>(cellOf(move))] = letterOf(move);
    ++moves_;
    if (const auto line = wordLine(cells_)) {
        winningLine_ = line;
        outcome_ = turn_based::winFor(turn_, "Completed the word " + winningWord());
        return true;
    }
    if (moves_ == 9) {
        outcome_ = turn_based::drawResult("Nine letters without a word");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    return true;
}

std::optional<MoveId> WordGame::chooseComputerMove(std::mt19937& random, const turn_based::CancelToken& cancel) const
{
    // The console's strategy: the first (cell, letter) that completes a word, scanning cells row
    // by row and letters A to Z; otherwise a random letter in the centre, then in the first free
    // corner, then in a random free cell.
    if (outcome_.finished() || cancel.cancelled()) return std::nullopt;
    std::vector<int> empty;
    for (int cell = 0; cell < 9; ++cell) {
        if (cells_[static_cast<std::size_t>(cell)] == '.') empty.push_back(cell);
    }
    if (empty.empty()) return std::nullopt;

    auto trial = cells_;
    for (const int cell : empty) {
        for (char letter = 'A'; letter <= 'Z'; ++letter) {
            trial[static_cast<std::size_t>(cell)] = letter;
            if (wordLine(trial)) return encode(cell, letter);
        }
        trial[static_cast<std::size_t>(cell)] = '.';
    }

    std::uniform_int_distribution<int> letters(0, 25);
    const auto randomLetter = [&] { return static_cast<char>('A' + letters(random)); };
    if (cells_[4] == '.') return encode(4, randomLetter());
    for (const int corner : {0, 2, 6, 8}) {
        if (cells_[static_cast<std::size_t>(corner)] == '.') return encode(corner, randomLetter());
    }
    std::uniform_int_distribution<std::size_t> cells(0, empty.size() - 1);
    const int cell = empty[cells(random)];
    return encode(cell, randomLetter());
}

} // namespace word_ttt
