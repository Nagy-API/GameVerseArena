# GameVerseArena

GameVerseArena is a C++17 games platform with two independently buildable applications:

- `GameVerseArena`, the existing console collection of 14 turn-based board games, including shared player setup, Human/Computer selection, result detection, and an in-memory scoreboard.
- `GameVerseArenaGUI`, a new SFML 3.1.0 graphical launcher shell with a main menu, game-library preview, settings preview, and project information.

No game has been migrated to the GUI yet. The GUI does not launch the console games, and Ping Pong remains a planned arcade game rather than a playable feature.

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
- Press Escape on Game Library, Settings, or About to return to the main menu.
- Press Escape on the main menu to close the application.
- Resize the window normally; the 16:9 interface view scales while preserving the layout. The practical design size is 960 x 540 or larger, with a default window size of 1280 x 720 and a 60 FPS frame limit.
