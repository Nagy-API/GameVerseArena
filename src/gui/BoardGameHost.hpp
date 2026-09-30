#pragma once

#include "BoardGameSession.hpp"
#include "BoardView.hpp"
#include "GameCatalogue.hpp"

#include <memory>

// The board game chosen in the library and the game in progress, shared by the board-game setup
// and game scenes. The setup scene starts the session and creates the game's view; the game
// scene plays it.
struct BoardGameHost {
    const catalogue::GameDescriptor* game{nullptr};
    turn_based::BoardGameSession session;
    std::unique_ptr<board_view::BoardView> view;
    // Set when the setup could not arm history recording, so the result can say it was not saved.
    bool recordingUnavailable{false};
};
