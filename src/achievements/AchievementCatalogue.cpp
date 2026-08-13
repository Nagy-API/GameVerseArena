#include "AchievementCatalogue.hpp"

namespace achievements {

const std::vector<AchievementDefinition>& AchievementCatalogue::all()
{
    static const std::vector<AchievementDefinition> catalogue{
        {"first_victory", "First Victory", "Win your first match.", AchievementCategory::General, false, true, 1},
        {"arena_regular", "Arena Regular", "Complete 10 matches.", AchievementCategory::General, false, true, 10},
        {"dedicated_player", "Dedicated Player", "Complete 25 matches.", AchievementCategory::General, false, true, 25},
        {"on_a_roll", "On a Roll", "Win 3 matches in a row.", AchievementCategory::General, false, true, 3},
        {"unstoppable", "Unstoppable", "Win 5 matches in a row.", AchievementCategory::General, false, true, 5},
        {"versatile_player", "Versatile Player",
         "Win at least one Classic Tic-Tac-Toe match and one Ping Pong match.",
         AchievementCategory::General, false, true, 2},
        {"xo_first_win", "XO Winner", "Win your first Classic Tic-Tac-Toe match.",
         AchievementCategory::TicTacToe, false, true, 1},
        {"x_marks_the_spot", "X Marks the Spot", "Win Classic Tic-Tac-Toe while playing X.",
         AchievementCategory::TicTacToe, false, true, 1},
        {"o_turnaround", "O Takes the Win", "Win Classic Tic-Tac-Toe while playing O.",
         AchievementCategory::TicTacToe, false, true, 1},
        {"pong_first_win", "Pong Winner", "Win your first Ping Pong match.",
         AchievementCategory::PingPong, false, true, 1},
        {"clean_sweep", "Clean Sweep", "Win Ping Pong 5-0.",
         AchievementCategory::PingPong, false, false, 0},
        {"clutch_finish", "Clutch Finish", "Win Ping Pong 5-4.",
         AchievementCategory::PingPong, false, false, 0}
    };
    return catalogue;
}

} // namespace achievements
