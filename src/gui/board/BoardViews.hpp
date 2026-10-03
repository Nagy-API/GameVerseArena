#pragma once

#include "BoardView.hpp"

#include <memory>
#include <string>

namespace board_view {

// The board view for a catalogue board game, or nullptr when the game has no graphical view.
std::unique_ptr<BoardView> createBoardView(const std::string& gameKey);

// One factory per migrated game (each defined next to its view).
std::unique_ptr<BoardView> makeNumericalView();
std::unique_ptr<BoardView> makeSusView();
std::unique_ptr<BoardView> makeFiveByFiveView();
std::unique_ptr<BoardView> makeMisereView();
std::unique_ptr<BoardView> makeFourInRowView();
std::unique_ptr<BoardView> makeFourByFourView();
std::unique_ptr<BoardView> makePyramidView();
std::unique_ptr<BoardView> makeDiamondView();
std::unique_ptr<BoardView> makeWordView();
std::unique_ptr<BoardView> makeInfinityView();
std::unique_ptr<BoardView> makeMemoryView();
std::unique_ptr<BoardView> makeObstacleView();

} // namespace board_view
