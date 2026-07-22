#pragma once

#include "TicTacToeBoard.hpp"
#include "TicTacToeTypes.hpp"

#include <optional>
#include <random>

namespace classic_ttt {

class TicTacToeAI {
public:
    TicTacToeAI();
    explicit TicTacToeAI(std::mt19937::result_type seed);

    std::optional<Position> chooseMove(const TicTacToeBoard& board, Cell aiMark, AIDifficulty difficulty);

private:
    std::optional<Position> chooseEasy(const TicTacToeBoard& board);
    std::optional<Position> chooseMedium(const TicTacToeBoard& board, Cell aiMark);
    std::optional<Position> chooseHard(const TicTacToeBoard& board, Cell aiMark);
    int minimax(const TicTacToeBoard& board, Cell turn, Cell aiMark, int depth, int alpha, int beta) const;
    std::optional<Position> findImmediateMove(const TicTacToeBoard& board, Cell mark) const;

    std::mt19937 random_;
};

} // namespace classic_ttt
