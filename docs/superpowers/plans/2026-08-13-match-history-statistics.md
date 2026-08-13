# Match History and Player Statistics Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Persist completed graphical Classic Tic-Tac-Toe and Ping Pong matches for the active local profile and expose derived statistics and filtered, paginated history.

**Architecture:** Migrate the existing SQLite database from schema v1 to v2, add project-owned match types and repository/service boundaries, and derive every statistic from completed match rows. An SFML-independent match recorder captures the active profile at match start, tracks monotonic active duration, maps pure game-session results, and guards finalization; GUI scenes consume typed services and never contain SQL.

**Tech Stack:** C++17, SQLite 3.53.4, SFML 3.1.0, CMake/CTest.

## Global Constraints

- Preserve all 14 console games and keep the console target SQLite-independent.
- Preserve graphical Classic Tic-Tac-Toe, graphical Ping Pong, profiles, SFML 3.1.0, SQLite 3.53.4, and the FreeType workaround.
- `matches` rows are the only persistent source of truth; never store aggregate counters.
- Record only terminal sessions, once; never record abandoned or restarted unfinished matches.
- Use active-profile ID captured at match start even when the match display name is edited.
- Keep SQL in persistence repositories and use temporary injected paths in tests.
- Preserve `5x5 Tic Tac Toe.cpp` and `BoardGames.slnx` byte-for-byte.

---

### Task 1: Schema v2 and typed persistence

**Files:** `src/persistence/Schema.hpp`, `src/persistence/Database.*`, `src/persistence/MatchTypes.hpp`, `src/persistence/MatchRepository.*`, `src/persistence/MatchService.*`, `src/persistence/StatisticsRepository.*`, `tests/MatchHistoryTests.cpp`, `CMakeLists.txt`.

- [ ] Write migration, insertion, validation, filtering, pagination, isolation, cascade, aggregate, and streak tests.
- [ ] Run the new test target and verify it fails because Task 08 types are absent.
- [ ] Implement transactional v1-to-v2 migration, prepared match inserts/queries, typed validation, and derived SQL statistics.
- [ ] Re-run the target and all persistence tests.

### Task 2: Completion mapping and exact-once recording

**Files:** `src/persistence/MatchRecorder.*`, `tests/MatchHistoryTests.cpp`, `src/games/classic_tic_tac_toe/TicTacToeSession.cpp`, `tests/TicTacToeTests.cpp`.

- [ ] Add failing tests for X/O/draw mapping, best-of and Ping Pong finalization, duplicate attempts, and duration pause/reset behavior.
- [ ] Make a single Tic-Tac-Toe draw terminal and implement the focused recorder guard.
- [ ] Re-run match, Tic-Tac-Toe, and Ping Pong tests.

### Task 3: Graphical game integration

**Files:** `src/gui/Application.*`, both setup scenes, both game scenes, and both result overlays.

- [ ] Capture active identity and match configuration at setup completion.
- [ ] Finalize immediately at the first true terminal state; pause Ping Pong's recorder with gameplay.
- [ ] Reset recorder state on rematch and abandon it on unfinished exits.
- [ ] Surface a non-blocking save warning on result overlays.

### Task 4: Statistics and history UI

**Files:** `src/gui/ProfilesScene.*`, `src/gui/ProfileStatsScene.*`, `src/gui/MatchHistoryScene.*`, `src/gui/SceneManager.hpp`, `src/gui/Application.cpp`.

- [ ] Add View Stats for the selected profile.
- [ ] Display overall and per-game derived metrics plus an empty state.
- [ ] Display 20 newest matches per page with game/result SQL filters and previous/next controls.
- [ ] Keep the logical 1280x720 layout usable through the existing 960x540-scaled view.

### Task 5: Documentation and verification

**Files:** `README.md`, `docs/CURRENT_ARCHITECTURE.md`, `AGENTS.md`.

- [ ] Document schema v2, ownership, duration, cascade, indexes, filters, derived stats, and non-recorded abandoned games.
- [ ] Run strict project-owned builds, the full build, all CTest targets, console regression, and feasible isolated GUI smoke checks.
- [ ] Recheck protected hashes/diffs, stage only Task 08 files, commit once, push `main`, and verify `origin/main`.
