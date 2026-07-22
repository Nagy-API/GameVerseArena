#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace classic_ttt {

enum class Cell { Empty, X, O };
enum class GameStatus { InProgress, XWon, OWon, Draw };
enum class GameMode { HumanVsHuman, HumanVsComputer };
enum class AIDifficulty { Easy, Medium, Hard };
enum class BestOf { Single = 1, Three = 3, Five = 5 };

struct Position {
    std::size_t row{};
    std::size_t column{};

    friend bool operator==(const Position& left, const Position& right)
    {
        return left.row == right.row && left.column == right.column;
    }
};

using WinningLine = std::array<Position, 3>;

inline Cell opposite(Cell mark)
{
    return mark == Cell::X ? Cell::O : mark == Cell::O ? Cell::X : Cell::Empty;
}

inline char markCharacter(Cell mark)
{
    return mark == Cell::X ? 'X' : mark == Cell::O ? 'O' : '-';
}

struct SessionConfig {
    GameMode mode{GameMode::HumanVsHuman};
    std::string playerOneName{"Player 1"};
    std::string playerTwoName{"Player 2"};
    Cell humanMark{Cell::X};
    AIDifficulty difficulty{AIDifficulty::Medium};
    BestOf bestOf{BestOf::Single};
};

struct MatchScore {
    unsigned int xWins{};
    unsigned int oWins{};
    unsigned int draws{};
};

} // namespace classic_ttt
