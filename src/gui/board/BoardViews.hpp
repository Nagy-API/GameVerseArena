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

} // namespace board_view
