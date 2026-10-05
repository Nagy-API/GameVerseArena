#include "UltimateGame.hpp"

#include <algorithm>
#include <limits>

namespace ultimate_xo {
namespace {

constexpr std::array<UltimateGame::Line, 8> lines{{
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}}};

using Cells = std::array<Mark, 81>;
using Squares = std::array<Square, 9>;

struct State {
    Cells cells;
    Squares squares;
    int forced;  // the small board the next move must use, or -1 for a free choice
};

Square squareOf(Mark mark)
{
    return mark == Mark::X ? Square::X : Square::O;
}

Mark at(const Cells& cells, int board, int cell)
{
    return cells[static_cast<std::size_t>(board * 9 + cell)];
}

std::optional<UltimateGame::Line> smallLine(const Cells& cells, int board, Mark mark)
{
    for (const auto& line : lines) {
        if (at(cells, board, line[0]) == mark && at(cells, board, line[1]) == mark && at(cells, board, line[2]) == mark) {
            return line;
        }
    }
    return std::nullopt;
}

// The console's update_big_board for one small board: an X line (checked first), an O line,
// a full board (tie), or still open.
Square squareFor(const Cells& cells, int board)
{
    if (smallLine(cells, board, Mark::X)) return Square::X;
    if (smallLine(cells, board, Mark::O)) return Square::O;
    for (int cell = 0; cell < 9; ++cell) {
        if (at(cells, board, cell) == Mark::Empty) return Square::Open;
    }
    return Square::Tie;
}

std::optional<UltimateGame::Line> bigLine(const Squares& squares, Square square)
{
    for (const auto& line : lines) {
        if (squares[static_cast<std::size_t>(line[0])] == square && squares[static_cast<std::size_t>(line[1])] == square &&
            squares[static_cast<std::size_t>(line[2])] == square) {
            return line;
        }
    }
    return std::nullopt;
}

bool allClosed(const Squares& squares)
{
    return std::none_of(squares.begin(), squares.end(), [](Square square) { return square == Square::Open; });
}

// Plays (board, cell) for `mark` and applies the console's rule for the next small board: the
// same one while it stays open, otherwise a free choice.
void apply(State& state, int board, int cell, Mark mark)
{
    state.cells[static_cast<std::size_t>(board * 9 + cell)] = mark;
    state.squares[static_cast<std::size_t>(board)] = squareFor(state.cells, board);
    state.forced = state.squares[static_cast<std::size_t>(board)] == Square::Open ? board : -1;
}

// The console's get_valid_moves: the forced board's empty cells, or the empty cells of every
// open board, boards and cells in order.
std::vector<MoveId> validMoves(const State& state)
{
    std::vector<MoveId> moves;
    const auto addBoard = [&](int board) {
        if (state.squares[static_cast<std::size_t>(board)] != Square::Open) return;
        for (int cell = 0; cell < 9; ++cell) {
            if (at(state.cells, board, cell) == Mark::Empty) moves.push_back(UltimateGame::encode(board, cell));
        }
    };
    if (state.forced == -1) {
        for (int board = 0; board < 9; ++board) addBoard(board);
    } else {
        addBoard(state.forced);
    }
    return moves;
}

class Search {
public:
    Search(Mark computer, Mark human, const turn_based::CancelToken& cancel)
        : computer_(computer), human_(human), computerSquare_(squareOf(computer)), humanSquare_(squareOf(human)),
          cancel_(cancel)
    {
    }

    // The console's minimax: depth counts the replies after the computer's candidate move, and
    // the evaluation is applied at depth 3.
    int minimax(const State& state, int depth, int alpha, int beta, bool maximizing)
    {
        if ((++nodes_ & 1023) == 0 && cancel_.cancelled()) stopped_ = true;
        if (stopped_) return 0;
        if (bigLine(state.squares, computerSquare_)) return 10000 - depth;
        if (bigLine(state.squares, humanSquare_)) return -10000 + depth;
        if (allClosed(state.squares)) return 0;
        if (depth >= maxDepth) return evaluate(state);
        const auto moves = validMoves(state);
        if (moves.empty()) return evaluate(state);
        const Mark mover = maximizing ? computer_ : human_;
        int best = maximizing ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
        for (const MoveId move : moves) {
            State next = state;
            apply(next, UltimateGame::boardOf(move), UltimateGame::cellOf(move), mover);
            const int value = minimax(next, depth + 1, alpha, beta, !maximizing);
            if (maximizing) {
                best = std::max(best, value);
                alpha = std::max(alpha, value);
            } else {
                best = std::min(best, value);
                beta = std::min(beta, value);
            }
            if (beta <= alpha) break;
        }
        return best;
    }

    bool stopped() const noexcept { return stopped_; }

private:
    static constexpr int maxDepth = 3;

    int scoreLine(Square a, Square b, Square c) const
    {
        int own = 0;
        int other = 0;
        int open = 0;
        for (const Square square : {a, b, c}) {
            if (square == computerSquare_) ++own;
            else if (square == humanSquare_) ++other;
            else if (square == Square::Open) ++open;
        }
        if (own == 3) return 1000;
        if (other == 3) return -1000;
        if (own == 2 && open == 1) return 10;
        if (other == 2 && open == 1) return -10;
        if (own == 1 && open == 2) return 1;
        if (other == 1 && open == 2) return -1;
        return 0;
    }

    // Rows and columns with one or two of `mark` (whatever else they hold) and the centre.
    static int smallBoardScore(const State& state, int board, Mark mark)
    {
        int score = 0;
        for (int i = 0; i < 3; ++i) {
            int row = 0;
            int column = 0;
            for (int j = 0; j < 3; ++j) {
                if (at(state.cells, board, i * 3 + j) == mark) ++row;
                if (at(state.cells, board, j * 3 + i) == mark) ++column;
            }
            score += row == 2 ? 3 : row == 1 ? 1 : 0;
            score += column == 2 ? 3 : column == 1 ? 1 : 0;
        }
        if (at(state.cells, board, 4) == mark) score += 2;
        return score;
    }

    int evaluate(const State& state) const
    {
        const auto& squares = state.squares;
        const auto square = [&](int index) { return squares[static_cast<std::size_t>(index)]; };
        int score = 0;
        for (const auto& line : lines) score += scoreLine(square(line[0]), square(line[1]), square(line[2])) * 100;
        if (square(4) == computerSquare_) score += 30;
        else if (square(4) == humanSquare_) score -= 30;
        for (const int corner : {0, 2, 6, 8}) {
            if (square(corner) == computerSquare_) score += 15;
            else if (square(corner) == humanSquare_) score -= 15;
        }
        for (int board = 0; board < 9; ++board) {
            if (square(board) != Square::Open) continue;
            score += smallBoardScore(state, board, computer_) - smallBoardScore(state, board, human_);
        }
        return score;
    }

    Mark computer_;
    Mark human_;
    Square computerSquare_;
    Square humanSquare_;
    const turn_based::CancelToken& cancel_;
    long long nodes_{0};
    bool stopped_{false};
};

} // namespace

UltimateGame::UltimateGame()
{
    reset();
}

std::unique_ptr<turn_based::TurnBasedGame> UltimateGame::clone() const
{
    return std::make_unique<UltimateGame>(*this);
}

void UltimateGame::reset()
{
    cells_.fill(Mark::Empty);
    squares_.fill(Square::Open);
    forced_ = -1;
    turn_ = Seat::First;
    moves_ = 0;
    outcome_ = {};
    winningLine_.reset();
}

std::optional<int> UltimateGame::forcedBoard() const noexcept
{
    if (forced_ < 0) return std::nullopt;
    return forced_;
}

std::optional<UltimateGame::Line> UltimateGame::smallBoardLine(int board) const
{
    if (board < 0 || board >= 9) return std::nullopt;
    const Square square = squares_[static_cast<std::size_t>(board)];
    if (square == Square::X) return smallLine(cells_, board, Mark::X);
    if (square == Square::O) return smallLine(cells_, board, Mark::O);
    return std::nullopt;
}

bool UltimateGame::isLegal(MoveId move) const
{
    if (outcome_.finished() || move < 0 || move >= 81) return false;
    const int board = boardOf(move);
    return squares_[static_cast<std::size_t>(board)] == Square::Open &&
           cells_[static_cast<std::size_t>(move)] == Mark::Empty && (forced_ < 0 || board == forced_);
}

std::vector<MoveId> UltimateGame::legalMoves() const
{
    if (outcome_.finished()) return {};
    return validMoves(State{cells_, squares_, forced_});
}

bool UltimateGame::play(MoveId move)
{
    if (!isLegal(move)) return false;
    const Mark mark = markFor(turn_);
    State state{cells_, squares_, forced_};
    apply(state, boardOf(move), cellOf(move), mark);
    cells_ = state.cells;
    squares_ = state.squares;
    forced_ = state.forced;
    ++moves_;
    if (const auto line = bigLine(squares_, squareOf(mark))) {
        winningLine_ = line;
        outcome_ = turn_based::winFor(turn_, "Three small boards in a line");
        return true;
    }
    if (allClosed(squares_)) {
        outcome_ = turn_based::drawResult("Every small board closed without a line");
        return true;
    }
    turn_ = turn_based::otherSeat(turn_);
    return true;
}

std::optional<MoveId> UltimateGame::chooseComputerMove(std::mt19937&, const turn_based::CancelToken& cancel) const
{
    // The console's get_best_move: every legal move in order, scored by the minimax above, keeping
    // the first best.
    if (outcome_.finished() || cancel.cancelled()) return std::nullopt;
    const State state{cells_, squares_, forced_};
    const Mark computer = markFor(turn_);
    Search search(computer, markFor(turn_based::otherSeat(turn_)), cancel);
    int bestScore = std::numeric_limits<int>::min();
    std::optional<MoveId> best;
    for (const MoveId move : validMoves(state)) {
        State next = state;
        apply(next, boardOf(move), cellOf(move), computer);
        const int score =
            search.minimax(next, 0, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), false);
        if (search.stopped()) return std::nullopt;
        if (score > bestScore) {
            bestScore = score;
            best = move;
        }
    }
    return best;
}

} // namespace ultimate_xo
