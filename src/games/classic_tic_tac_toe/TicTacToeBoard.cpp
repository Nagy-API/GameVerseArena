#include "TicTacToeBoard.hpp"

#include <array>
#include <stdexcept>

namespace classic_ttt {

bool TicTacToeBoard::placeMark(Position position, Cell mark)
{
    if (position.row >= size || position.column >= size || mark == Cell::Empty ||
        status_ != GameStatus::InProgress || cells_[position.row][position.column] != Cell::Empty) {
        return false;
    }

    cells_[position.row][position.column] = mark;
    ++moveCount_;
    updateStatus();
    return true;
}

Cell TicTacToeBoard::cell(Position position) const
{
    if (position.row >= size || position.column >= size) {
        throw std::out_of_range("Tic-Tac-Toe cell is outside the board");
    }
    return cells_[position.row][position.column];
}

std::vector<Position> TicTacToeBoard::legalMoves() const
{
    std::vector<Position> moves;
    if (status_ != GameStatus::InProgress) {
        return moves;
    }

    for (std::size_t row = 0; row < size; ++row) {
        for (std::size_t column = 0; column < size; ++column) {
            if (cells_[row][column] == Cell::Empty) {
                moves.push_back({row, column});
            }
        }
    }
    return moves;
}

void TicTacToeBoard::reset()
{
    cells_ = {};
    status_ = GameStatus::InProgress;
    winningLine_.reset();
    moveCount_ = 0;
}

void TicTacToeBoard::updateStatus()
{
    constexpr std::array<WinningLine, 8> lines{{
        {{{0, 0}, {0, 1}, {0, 2}}},
        {{{1, 0}, {1, 1}, {1, 2}}},
        {{{2, 0}, {2, 1}, {2, 2}}},
        {{{0, 0}, {1, 0}, {2, 0}}},
        {{{0, 1}, {1, 1}, {2, 1}}},
        {{{0, 2}, {1, 2}, {2, 2}}},
        {{{0, 0}, {1, 1}, {2, 2}}},
        {{{0, 2}, {1, 1}, {2, 0}}}
    }};

    for (const auto& line : lines) {
        const Cell mark = cells_[line[0].row][line[0].column];
        if (mark != Cell::Empty &&
            cells_[line[1].row][line[1].column] == mark &&
            cells_[line[2].row][line[2].column] == mark) {
            status_ = mark == Cell::X ? GameStatus::XWon : GameStatus::OWon;
            winningLine_ = line;
            return;
        }
    }

    if (moveCount_ == size * size) {
        status_ = GameStatus::Draw;
    }
}

} // namespace classic_ttt
