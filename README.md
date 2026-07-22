# GameVerseArena

GameVerseArena is currently a C++17 console collection of 14 turn-based board games. It provides a shared player setup, Human/Computer selection, game menu, result detection, and an in-memory scoreboard across matches in one application session.

The current release is console-based. It does not yet include a graphical interface, real-time arcade games, profiles, achievements, or persistent storage.

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

- CMake 3.20 or newer
- A C++17 compiler, such as Visual Studio C++ or GNU C++

No external libraries are required.

## Build on Windows

From PowerShell in the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

With a Visual Studio multi-configuration generator, run:

```powershell
.\build\Release\GameVerseArena.exe
```

For a single-configuration generator, the executable is normally at `build\GameVerseArena.exe`.

## Portable CMake workflow

From a shell in the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/GameVerseArena
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
