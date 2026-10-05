#include "BoardViews.hpp"

#include <map>

namespace board_view {

std::unique_ptr<BoardView> createBoardView(const std::string& gameKey)
{
    using Maker = std::unique_ptr<BoardView> (*)();
    static const std::map<std::string, Maker> makers{
        {"numerical_tic_tac_toe", &makeNumericalView},
        {"sus", &makeSusView},
        {"five_by_five_tic_tac_toe", &makeFiveByFiveView},
        {"misere_tic_tac_toe", &makeMisereView},
        {"four_in_a_row", &makeFourInRowView},
        {"four_by_four_tic_tac_toe", &makeFourByFourView},
        {"pyramid_tic_tac_toe", &makePyramidView},
        {"diamond", &makeDiamondView},
        {"word_tic_tac_toe", &makeWordView},
        {"infinity_xo", &makeInfinityView},
        {"memory_xo", &makeMemoryView},
        {"obstacle_tic_tac_toe", &makeObstacleView},
        {"ultimate_xo", &makeUltimateView},
    };
    const auto found = makers.find(gameKey);
    return found == makers.end() ? nullptr : found->second();
}

} // namespace board_view
