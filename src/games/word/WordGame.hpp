#pragma once

#include "TurnBasedGame.hpp"

#include <array>
#include <optional>
#include <set>
#include <string>
#include <vector>

// Word Tic-Tac-Toe, ported from the console game (Word_Tic_Tac_Toe.cpp): players take turns
// writing any letter A-Z in an empty cell of a 3x3 grid. Whoever completes a valid three-letter
// word, read left to right along a row, top to bottom along a column, or downward along either
// diagonal, wins. Nine letters without a word is a draw. Moves are encoded as cell * 26 + letter
// index (A = 0).
//
// The console loads its words from dic.txt at run time; the graphical version builds in the same
// 22 words (a test keeps the two lists identical), so it never depends on the working directory.
namespace word_ttt {

using turn_based::MoveId;
using turn_based::Seat;

class WordGame final : public turn_based::TurnBasedGame {
public:
    using Line = std::array<int, 3>;

    WordGame();

    // The built-in dictionary: the words of the console's dic.txt, in upper case.
    static const std::set<std::string>& dictionary();
    static MoveId encode(int cell, char letter) noexcept { return cell * 26 + (letter - 'A'); }
    static int cellOf(MoveId move) noexcept { return move / 26; }
    static char letterOf(MoveId move) noexcept { return static_cast<char>('A' + move % 26); }

    char cell(int index) const { return cells_.at(static_cast<std::size_t>(index)); }  // '.' when empty
    // The completed word's cells, once a word has been made.
    const std::optional<Line>& winningLine() const noexcept { return winningLine_; }
    std::string winningWord() const;

    std::unique_ptr<turn_based::TurnBasedGame> clone() const override;
    void reset() override;
    Seat currentSeat() const override { return turn_; }
    turn_based::Outcome outcome() const override { return outcome_; }
    std::vector<MoveId> legalMoves() const override;
    bool isLegal(MoveId move) const override;
    bool play(MoveId move) override;
    int movesPlayed() const override { return moves_; }
    std::optional<MoveId> chooseComputerMove(std::mt19937& random, const turn_based::CancelToken& cancel) const override;

private:
    std::array<char, 9> cells_{};
    Seat turn_{Seat::First};
    int moves_{0};
    turn_based::Outcome outcome_{};
    std::optional<Line> winningLine_;
};

} // namespace word_ttt
