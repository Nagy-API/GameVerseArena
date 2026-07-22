#pragma once

#include "TicTacToeTypes.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace classic_ttt {

class TicTacToeBoard {
public:
    static constexpr std::size_t size = 3;
    using Cells = std::array<std::array<Cell, size>, size>;

    bool placeMark(Position position, Cell mark);
    Cell cell(Position position) const;
    const Cells& cells() const noexcept { return cells_; }
    GameStatus status() const noexcept { return status_; }
    const std::optional<WinningLine>& winningLine() const noexcept { return winningLine_; }
    std::size_t moveCount() const noexcept { return moveCount_; }
    std::vector<Position> legalMoves() const;
    void reset();

private:
    void updateStatus();

    Cells cells_{};
    GameStatus status_{GameStatus::InProgress};
    std::optional<WinningLine> winningLine_;
    std::size_t moveCount_{};
};

} // namespace classic_ttt
