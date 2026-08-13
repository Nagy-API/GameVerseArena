# AGENTS.md

## Project Purpose

This repository is being evolved from a C++ console board-games assignment into a polished C++ desktop games platform.

The existing product currently contains 14 playable board games:

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

The long-term direction is a single games arena containing both turn-based board games and real-time arcade games such as Ping Pong.

Use the repository's current name. Do not rename or rebrand the project unless the current task explicitly requests it.

---

## Current Baseline

- Language: C++17
- Current playable UI: console
- Console entry point: `XO_Demo.cpp`
- Graphical shell entry point: `src/gui/main.cpp`
- Approved GUI dependency: SFML 3.1.0 through CMake FetchContent
- Shared board-game abstractions: `BoardGame_Classes.h`
- Existing Visual Studio project files are present.
- The existing application, game rules, computer-player behavior, player setup, and scoreboard must remain functional unless a task explicitly changes them.

The current board-game framework is designed for turn-based games. Do not force future real-time games such as Ping Pong into `Board<T>`, `Move<T>`, or the existing turn-based `GameManager<T>`.

The `GameVerseArena` console target and all 14 existing games must remain preserved. New graphical code must use SFML 3 APIs. Do not silently merge console presentation or turn-based game logic into GUI scenes.

---

## Working Rules

1. Work only on the task described in the current prompt.
2. Do not implement future roadmap items unless explicitly requested.
3. Inspect the repository and relevant files before editing.
4. Preserve existing behavior unless the task requires a behavior change.
5. Avoid broad refactors, unrelated formatting, and unnecessary file renames.
6. Do not replace working game logic merely to make it look cleaner.
7. Do not silently remove a game, AI strategy, menu option, or scoreboard behavior.
8. Keep changes small, reviewable, and easy to validate.
9. Never claim a command or test passed unless it was actually run successfully.
10. If a required tool or dependency is unavailable, continue with all possible work and report the exact limitation.

---

## Architecture Direction

The future platform should support two different game categories:

- Turn-based games
- Real-time games

Shared platform concerns may later include:

- application lifecycle
- scenes and navigation
- player profiles
- settings
- match results
- statistics
- persistence
- assets
- audio
- game registration

Board-game-specific concepts such as `Board<T>`, `Move<T>`, and turn order must stay inside the turn-based layer.

Real-time concepts such as frame updates, delta time, continuous input, physics, and collision detection must stay inside the real-time layer.

Reusable real-time simulation must remain independent from rendering and framework-specific vector/rectangle types. Prefer a fixed timestep for deterministic real-time physics, clamp stalled frame time, and cap catch-up work. Continuous input must be cleared on application focus loss so hidden movement cannot continue.

SFML 3.1.0 is approved only for the graphical target. Do not introduce another graphics framework, SQLite, JSON libraries, or any other external dependency unless the current task explicitly requests it.

The graphical target now embeds SQLite for local player profiles. Keep all SQL inside the persistence layer; GUI scenes must use the profile service rather than database APIs. Schema changes require an explicit migration and `PRAGMA user_version` bump. Persistence tests must use injected temporary database paths, the console target must remain SQLite-independent, and runtime database files must never be stored in the repository or build directories.

Completed-match rows are the persistent source of truth for player statistics. Derive statistics from history queries rather than incrementing stored counters. Match writes must occur exactly once at final completion; abandoned or unfinished matches must never be persisted. Game scenes must not contain raw SQL. Every schema change requires a versioned migration plus migration, reopen, future-version, and relevant data-integrity tests.

Player achievements must derive their conditions and progress from authoritative completed-match history/statistics. Never persist progress counters; persist only idempotent first-unlock facts after the matching completed-match write succeeds. Achievement catalogue metadata is immutable project-owned code, not mutable database data. Startup backfill must be idempotent and use compact history queries. Every achievement-related schema migration requires migration, reopen, future-version, cascade, uniqueness, and data-preservation tests.

---

## C++ Standards

- Use C++17 unless a task explicitly changes the standard.
- Prefer RAII and standard-library containers.
- Prefer `std::unique_ptr` for new ownership code.
- Avoid introducing new raw owning pointers or manual `new`/`delete`.
- Use `const` where appropriate.
- Keep headers self-contained.
- Avoid `using namespace std;` in newly created header files.
- Do not add global mutable state unless clearly justified.
- Keep game rules separate from presentation and input when creating new code.
- Keep reusable graphical game rules and AI SFML-independent so they can be tested without opening a window.
- Graphical game scenes must not modify or directly depend on the matching console game implementation.
- Do not change old code only to enforce a style preference unless the task is specifically a cleanup task.

---

## Repository Hygiene

Never commit or intentionally create repository content from:

- `.vs/`
- `x64/`
- `Debug/`
- `Release/`
- `build/`
- `out/`
- compiled executables
- object files
- PDB or ILK files
- IDE caches
- temporary files

Keep `.gitignore` updated when a task changes the build workflow.

Do not delete user source files or assets because they appear unused without first proving they are generated or obsolete within the task scope.

---

## Build and Validation

Before a CMake build is introduced, use an available existing build method.

Portable compiler command when GNU C++ is available:

```bash
g++ -std=c++17 *.cpp -o games_arena
```

Stricter diagnostic build when appropriate:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic *.cpp -o games_arena
```

Visual Studio/MSBuild may be used when available.

After a root `CMakeLists.txt` exists, prefer:

```bash
cmake -S . -B build
cmake --build build --config Release
```

For every implementation task:

1. Build the project.
2. Run relevant automated tests if they exist.
3. Perform a focused smoke test when possible.
4. Report commands executed and their results.
5. Clearly separate pre-existing warnings from newly introduced errors.
6. Add or update automated CTest coverage when shared game rules, sessions, or AI behavior change.

Do not spend the current task fixing all pre-existing warnings unless the prompt explicitly requests warning cleanup.

---

## Documentation Rules

Update documentation when the task changes:

- build commands
- dependencies
- directory structure
- user-visible behavior
- game list
- controls
- setup steps

Documentation must match the actual repository after the implementation.

Do not advertise planned features as completed features.

---

## Required Final Response

At the end of every task, provide:

1. A concise summary of what changed.
2. The files created, modified, moved, or deleted.
3. Build and test commands that were actually run.
4. Their exact pass/fail outcome.
5. Any remaining limitation or risk relevant to the task.
6. Confirmation that unrelated functionality was not intentionally changed.

Do not stop after analysis when the prompt asks for implementation. Complete the requested code changes and validate them.
