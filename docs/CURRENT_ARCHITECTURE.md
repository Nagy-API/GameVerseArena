# Current Architecture

## Application entry points

`XO_Demo.cpp` contains `main()` for the existing `GameVerseArena` console application. It initializes the process-wide random number generator, collects the two players' names and Human/Computer types, displays the main and game-selection menus, starts the selected game, detects its result, and updates the session scoreboard.

`src/gui/main.cpp` contains `main()` for the separate `GameVerseArenaGUI` executable. `Application` owns the SFML 3.1.0 window, runtime asset manager, responsive view, event/update/render loop, and scene manager. The GUI runs at a 1280 x 720 logical size with a 60 FPS limit and clamps long frame times before updating animations.

## Graphical shell

`SceneManager` owns the scenes and changes the active scene without transferring or exposing ownership. Its activation callback lets stateful scenes safely initialize transient work such as an AI-turn timer. The shell contains:

- `MainMenuScene`: Play, Settings, About, and Exit navigation with keyboard and mouse input.
- `GameLibraryScene`: launches graphical Classic Tic-Tac-Toe, identifies the other 13 games as console-only, and keeps Ping Pong labeled as planned.
- `TicTacToeSetupScene`: keyboard- and mouse-accessible mode, name, mark, AI, and match-length configuration.
- `TicTacToeGameScene`: event-driven board input, score display, mark animation, non-blocking AI turns, and safe navigation.
- `TicTacToeResultOverlay`: round and match results with next-round, restart, rematch, setup, and library actions.
- `SettingsScene`: non-functional placeholders for Display, Audio, Controls, and Theme.
- `AboutScene`: technology and current-milestone information.

`UiButton` provides common bounds, label rendering, hover and selected states, click hit-testing, and delta-time-based visual transitions. `Theme.hpp` centralizes the shell's colors, spacing, type sizes, and animation speed. `AssetManager` loads each required Inter font once, and CMake copies the assets beside the GUI executable.

## Graphical Classic Tic-Tac-Toe module

The first migrated game is isolated under `src/games/classic_tic_tac_toe` and links to the GUI as the `GameVerseArenaTicTacToe` library:

- `TicTacToeBoard` is a pure C++ 3×3 board with legal-move validation, status detection, move count, reset, and winning-line coordinates.
- `TicTacToeAI` is SFML-independent. Easy chooses a random legal move, Medium uses tactical priorities, and Hard uses depth-aware Minimax with alpha-beta pruning.
- `TicTacToeSession` owns configuration, current turn, round lifecycle, match score, best-of completion, mark assignment, and duplicate-result protection.

The graphical scenes read and mutate this focused session through its public API. They do not use or modify `Board<T>`, `Move<T>`, `Player<T>`, `GameManager<T>`, or the console `XO_Classes` implementation. This boundary keeps the console game stable while allowing event-driven GUI input and animation.

`GameVerseArenaTests` links only to the pure library and is registered with CTest. It covers board rules, session scoring and lifecycle, all AI levels, and recursive Hard-AI no-loss validation without opening an SFML window.

## Shared turn-based abstractions

`BoardGame_Classes.h` defines the template-based framework used by the board games:

- `Board<T>` owns a game's board matrix and exposes virtual operations for applying moves and detecting wins, losses, draws, and game completion.
- `Move<T>` represents a discrete move with a row, column, and symbol.
- `Player<T>` stores a player's name, type, symbol, and active board pointer.
- `UI<T>` defines console display, input, player setup, and player creation hooks.
- `GameManager<T>` runs the alternating two-player, turn-based game loop.
- `PlayerType` identifies Human, Computer, AI, and Random player categories; the current top-level setup offers Human and Computer.

Most modules use `char` boards. Numerical Tic-Tac-Toe uses the same abstractions with `int`.

## Game modules

| Game | Main files | Notes |
| --- | --- | --- |
| Classic Tic-Tac-Toe | `XO_Classes.h`, `XO_Classes.cpp` | Standard `Board<char>` and `UI<char>` implementation. |
| Numerical Tic-Tac-Toe | `NumericalTicTacToe.h`, `NumericalTicTacToe.cpp` | Uses `Board<int>` and `UI<int>`. |
| SUS | `SUS.h`, `SUS.cpp`, `SUS_AI.h`, `SUS_AI.cpp` | Uses its own `run_sus_game` flow and computer-player support. |
| 5x5 Tic-Tac-Toe | `5x5 Tic Tac Toe.h`, `5x5 Tic Tac Toe.cpp` | Filenames and include paths intentionally retain spaces. |
| Misere Tic-Tac-Toe | `Misere_Board.h`, `Misere_Board.cpp` | Reports the losing-line condition through the shared board interface. |
| Four-in-a-Row | `FourInRow.h`, `FourInRow.cpp` | Includes module-specific computer move behavior. |
| 4x4 Tic-Tac-Toe | `T4x4_Classes.h`, `T4x4_Classes.cpp` | Implements movement-oriented turns within the board-game layer. |
| Word Tic-Tac-Toe | `Word_Tic_Tac_Toe.h`, `Word_Tic_Tac_Toe.cpp`, `dic.txt` | Loads `dic.txt` from the process working directory. |
| Pyramid Tic-Tac-Toe | `pyramid.h`, `pyramid.cpp` | Its UI is constructed with the active pyramid board. |
| Diamond | `Diamond.h`, `Diamond.cpp` | Standard character board/UI module. |
| Infinity XO | `Infinity_XO.h`, `Infinity_XO.cpp` | Standard character board/UI module with module-specific rules. |
| Ultimate XO | `Ultimate_TTT.h`, `Ultimate_TTT.cpp`, `Ultimate_TTT_AI.h`, `Ultimate_TTT_AI.cpp` | Uses `UltimateTTT_Manager` because a turn requires custom multi-step handling. |
| Memory XO | `Memory_XO.h`, `Memory_XO.cpp`, `Memory_XO_AI.h` | Computer support is implemented in the module and its AI header. |
| Obstacle Tic-Tac-Toe | `Obstacle.h`, `Obstacle.cpp` | Standard character board/UI module with obstacle rules. |

## Player setup and game flow

The top-level `TournamentScore` in `XO_Demo.cpp` holds both player names, both player types, wins, draws, and the games-played count for the current process. `configure_players()` prompts for each name and Human/Computer selection, then clears the scoreboard.

When a game is selected, `play_selected_game()` constructs that module's board and UI and creates players using the stored top-level configuration. Most games run through `GameManager<T>`. SUS and Ultimate XO use their existing specialized runners, while Pyramid uses custom construction before entering the standard manager. After a game ends, `detect_result()` queries the board's win, loss, and draw methods before `update_score()` records the result.

The scoreboard is shared across games during the current application session. It can be displayed or reset from the main menu, and changing the configured players also clears it. There is no persistence between application runs.

## Current constraints

- The playable board-game user interface and input model remain console-based and synchronous.
- Classic Tic-Tac-Toe is the only game currently migrated to the graphical application; the other 13 board games remain console-only.
- Settings are labeled previews and do not persist or change application behavior.
- Ping Pong is displayed only as planned and has not been implemented.
- The shared framework assumes two players taking discrete, alternating turns.
- Game completion is expressed through `Board<T>` win, loss, and draw queries.
- Individual modules contain their existing rule, presentation, input, and computer-player behavior; these have not been reorganized.
- Word Tic-Tac-Toe depends on `dic.txt` being available in the process working directory. The CMake build places a copy beside the executable.

This framework is specifically a turn-based board-game layer. A real-time game has different needs, including frame updates, delta time, continuous input, physics, and collision detection. Real-time games must not be forced into `Board<T>`, `Move<T>`, or the existing `GameManager<T>` loop.

The SFML scene layer is currently independent of the console framework. Future tasks must use an explicit integration boundary rather than moving console presentation, `Board<T>`, or `GameManager<T>` directly into GUI scenes.
