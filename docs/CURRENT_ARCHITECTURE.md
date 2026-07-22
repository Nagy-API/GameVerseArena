# Current Architecture

## Application entry point

`XO_Demo.cpp` contains `main()` and the current console application shell. It initializes the process-wide random number generator, collects the two players' names and Human/Computer types, displays the main and game-selection menus, starts the selected game, detects its result, and updates the session scoreboard.

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

- The user interface and input model are console-based and synchronous.
- The shared framework assumes two players taking discrete, alternating turns.
- Game completion is expressed through `Board<T>` win, loss, and draw queries.
- Individual modules contain their existing rule, presentation, input, and computer-player behavior; these have not been reorganized.
- Word Tic-Tac-Toe depends on `dic.txt` being available in the process working directory. The CMake build places a copy beside the executable.

This framework is specifically a turn-based board-game layer. A real-time game has different needs, including frame updates, delta time, continuous input, physics, and collision detection. Real-time games must not be forced into `Board<T>`, `Move<T>`, or the existing `GameManager<T>` loop.
