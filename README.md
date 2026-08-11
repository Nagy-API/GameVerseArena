# GameVerseArena

GameVerseArena is a C++17 games platform with two independently buildable applications:

- `GameVerseArena`, the existing console collection of 14 turn-based board games, including shared player setup, Human/Computer selection, result detection, and an in-memory scoreboard.
- `GameVerseArenaGUI`, an SFML 3.1.0 graphical application with a launcher, game library, graphical Classic Tic-Tac-Toe, and real-time Ping Pong.

Classic Tic-Tac-Toe and Ping Pong are playable in the GUI. The other 13 original board games remain playable in the console application only. Ping Pong is a separate GUI arcade game, so the original console collection remains 14 board games rather than becoming a 15-game board collection.

## Games

1. Classic Tic-Tac-Toe
2. Numerical Tic-Tac-Toe
3. SUS
4. 5x5 Tic-Tac-Toe
5. Misere Tic-Tac-Toe
6. Four-in-a-Row
7. 4x4 Tic-Tac-Toe
8. Word Tic-Tac-Toe
9. Pyramid Tic-Tac-Toe
10. Diamond
11. Infinity XO
12. Ultimate XO
13. Memory XO
14. Obstacle Tic-Tac-Toe

## Requirements

- CMake 3.28 or newer
- Git (used by CMake FetchContent)
- A C++17 compiler, such as Visual Studio C++ or GNU C++

The graphical target uses SFML 3.1.0. CMake fetches the pinned release from the official SFML repository, so the first configure requires an internet connection and may take several minutes while SFML is downloaded and built. The console sources can still be compiled directly without SFML.

## Build on Windows

From PowerShell in the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Build either application individually with:

```powershell
cmake --build build --config Release --target GameVerseArena
cmake --build build --config Release --target GameVerseArenaGUI
cmake --build build --config Release --target GameVerseArenaTests
cmake --build build --config Release --target GameVerseArenaPingPongTests
ctest --test-dir build --output-on-failure
```

With a Visual Studio multi-configuration generator, the executables are normally at:

```powershell
.\build\Release\GameVerseArena.exe
.\build\Release\GameVerseArenaGUI.exe
```

For a single-configuration generator, they are normally at `build\GameVerseArena.exe` and `build\GameVerseArenaGUI.exe`. Runtime GUI assets are copied into an `assets` directory beside `GameVerseArenaGUI`.

## Portable CMake workflow

From a shell in the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/GameVerseArena
./build/GameVerseArenaGUI
```

The exact executable suffix and output folder can vary by platform and CMake generator.

## Direct GNU C++ build

When GNU C++ is available, the current source layout can also be built directly from the repository root:

```sh
g++ -std=c++17 *.cpp -o games_arena
./games_arena
```

On Windows, run `games_arena.exe` instead.

Word Tic-Tac-Toe loads `dic.txt` at runtime. The CMake build copies the dictionary beside the built executable. For a direct compiler build, run the program from the repository root so the existing `dic.txt` is available.

## Current architecture

The application entry point is `XO_Demo.cpp`, and the shared turn-based abstractions are defined in `BoardGame_Classes.h`. See [Current Architecture](docs/CURRENT_ARCHITECTURE.md) for the module map and current application flow.

The existing Visual Studio solution and project files remain available for IDE builds.

## GUI shell controls

- Move the pointer over a button to highlight it; click the left mouse button to activate it.
- Use Up and Down to change the selected main-menu item and Enter to activate it.
- Keyboard actions activate once per key press; release the key before activating another scene or overlay action.
- Press Escape on Game Library, Settings, or About to return to the main menu.
- Press Escape on the main menu to close the application.
- Resize the window normally; the 16:9 interface view scales while preserving the layout. The practical design size is 960 x 540 or larger, with a default window size of 1280 x 720 and a 60 FPS frame limit.

## Classic Tic-Tac-Toe in the GUI

Choose **Play**, select **Classic Tic-Tac-Toe**, complete Player Setup, and start the match. The graphical version supports:

- Human vs Human and Human vs Computer.
- Human choice of X or O; X always opens the round.
- Easy AI (random legal move), Medium AI (win, block, center, corner priority), and unbeatable Hard AI (alpha-beta Minimax).
- Single Game, Best of 3, and Best of 5 matches, with draws tracked separately.
- Next Round, Restart Round, New Match, Rematch, New Setup, and Return to Library flows as appropriate.

Setup is fully keyboard accessible: use Up/Down or Tab to move, Left/Right to change choices, Enter to edit names or activate a control, Backspace to edit, and Escape to go back. During a match, use the arrow keys to select a board cell and Enter or Space to play it. Mouse hover and click are supported throughout. Escape or **Back to Library** opens a confirmation before discarding an active in-memory match.

No player profile, score history, or other game data is persisted between application runs.

## Ping Pong in the GUI

Choose **Play**, select **Ping Pong** under Arcade Games, configure the match, and select **Start Match**. Every match is first to 5 points with no win-by-two rule.

- Human vs Human: W/S controls the left paddle and Up/Down controls the right paddle.
- Human vs Computer: the human controls the left paddle with either W/S or Up/Down.
- Easy AI reacts every 190 ms, moves at 68% paddle speed, follows the current ball position, and uses broad aiming error.
- Medium AI reacts every 105 ms, moves at 84% speed, predicts the intercept including wall reflections, and uses moderate error.
- Hard AI reacts every 52 ms, uses the full legal paddle speed, predicts reflected intercepts, and uses small controlled error.

The ball starts at 470 logical pixels per second. Each successful paddle contact increases its speed by 4%, capped at 900, and every point resets it to base speed. After a point, play freezes briefly, the scorer is shown, the paddles and ball reset, and a three-second serve countdown begins.

Press Escape or select **Pause** to stop physics, AI timers, and the serve clock. The pause menu provides Resume, Restart Match, New Setup, and Return to Library. Losing window focus clears held movement and opens the pause overlay. The winner overlay provides Rematch, New Setup, and Return to Library.

Ping Pong physics run at a fixed 1/120-second step independently of rendering. The pure Ping Pong tests do not create a window or link SFML.
