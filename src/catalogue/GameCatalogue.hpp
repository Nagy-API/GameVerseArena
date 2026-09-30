#pragma once

#include "TurnBasedGame.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The graphical game catalogue: immutable, project-owned metadata for every game the GUI
// knows about. It is SFML-independent (so it can be tested without a window) and contains no
// SQL. The GUI binds each entry to a launcher; persistence uses the same stable keys.
namespace catalogue {

enum class GameCategory { Board, Arcade };

// How the GUI starts a game:
// - DedicatedScene: its own setup/game scenes (Classic Tic-Tac-Toe, Ping Pong).
// - BoardGame: a board game created through `createGame` and played in shared board-game scenes.
// - ConsoleOnly: not yet available in the GUI (still fully playable in the console app).
enum class LaunchKind { DedicatedScene, BoardGame, ConsoleOnly };

using GameFactory = std::function<std::unique_ptr<turn_based::TurnBasedGame>(std::uint32_t seed)>;

struct GameDescriptor {
    std::string key;               // stable identifier; equals the persistence storage key
    std::string displayName;
    std::string shortDescription;  // one line for the library card
    std::string rules;             // full rules text
    std::string boardSummary;      // e.g. "3x3", "6x7 gravity"
    GameCategory category{GameCategory::Board};
    bool humanVsHuman{true};
    bool humanVsComputer{true};
    std::string computerStrategy;  // truthful description of the computer opponent
    bool selectableDifficulty{false};
    std::string firstSeatLabel;    // what the first player uses, e.g. "X", "S", "Odd numbers"
    std::string secondSeatLabel;
    bool tournamentEligible{true};
    bool historyEligible{true};
    LaunchKind launch{LaunchKind::ConsoleOnly};
    GameFactory createGame;        // set only for LaunchKind::BoardGame
    int consoleMenuNumber{0};      // 1..14 in XO_Demo.cpp's games menu; 0 for GUI-only games
    std::string consoleMenuLabel;  // exact menu text after "N.  ", e.g. "Play X-O (Tic-Tac-Toe)"
    std::string consoleBoardClass; // legacy console board class, e.g. "X_O_Board"

    bool playableInGui() const noexcept { return launch != LaunchKind::ConsoleOnly; }
};

// All games in library order: the 14 original board games in console-menu order, then Ping Pong.
const std::vector<GameDescriptor>& all();
const GameDescriptor* find(std::string_view key);

// Case-insensitive search over display names, optionally restricted to one category.
bool matchesSearch(const GameDescriptor& game, std::string_view query);
std::vector<const GameDescriptor*> filter(std::string_view query, std::optional<GameCategory> category);

std::size_t playableCount();

} // namespace catalogue
