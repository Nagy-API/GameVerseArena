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
    };
    const auto found = makers.find(gameKey);
    return found == makers.end() ? nullptr : found->second();
}

} // namespace board_view
