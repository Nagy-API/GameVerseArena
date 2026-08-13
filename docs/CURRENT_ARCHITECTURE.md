# Current Architecture

## Application entry points

`XO_Demo.cpp` contains `main()` for the existing `GameVerseArena` console application. It initializes the process-wide random number generator, collects the two players' names and Human/Computer types, displays the main and game-selection menus, starts the selected game, detects its result, and updates the session scoreboard.

`src/gui/main.cpp` contains `main()` for the separate `GameVerseArenaGUI` executable. `Application` owns the SFML 3.1.0 window, runtime asset manager, responsive view, event/update/render loop, and scene manager. The GUI runs at a 1280 x 720 logical size with a 60 FPS limit and clamps long frame times before updating animations. Ping Pong performs its own fixed-step accumulation inside its game scene.

## Graphical shell

`SceneManager` owns the scenes and changes the active scene without transferring or exposing ownership. Its activation callback lets stateful scenes safely initialize transient work such as an AI-turn timer. Scene switches are synchronous, but the event that requests a switch finishes in the old scene; application-wide key repeat is disabled so a held activation key cannot immediately trigger an overlay or control in the newly active scene. The shell contains:

- `MainMenuScene`: Play, Profiles, Settings, About, and Exit navigation with keyboard and mouse input.
- `ProfilesScene`: bounded/scrollable local profile selection plus Create, Rename, Delete, Set Active, View Stats, and Back actions. `ProfileEditOverlay` and `ProfileDeleteOverlay` own transient input and confirmation presentation; all validation and state transitions remain in `ProfileService`.
- `ProfileStatsScene`: read-only overall and per-game statistics for the selected profile, including the zero-history empty state.
- `MatchHistoryScene`: newest-first bounded pages with repository-level game/result filters and Previous/Next navigation.
- `GameLibraryScene`: launches graphical Classic Tic-Tac-Toe and Ping Pong while truthfully identifying the other 13 board games as console-only.
- `TicTacToeSetupScene`: keyboard- and mouse-accessible mode, name, mark, AI, and match-length configuration.
- `TicTacToeGameScene`: event-driven board input, score display, mark animation, non-blocking AI turns, and safe navigation.
- `TicTacToeResultOverlay`: round and match results with next-round, restart, rematch, setup, and library actions.
- `PingPongSetupScene`: local/AI mode, player names, and Easy/Medium/Hard configuration with keyboard and mouse input.
- `PingPongGameScene`: held controls, fixed-timestep simulation, score and arena rendering, focus-loss safety, and scene navigation.
- `PingPongPauseOverlay`: Resume, Restart Match, New Setup, and Return to Library actions while all real-time state is stopped.
- `PingPongResultOverlay`: winner, final score, Rematch, New Setup, and Return to Library actions.
- `SettingsScene`: non-functional placeholders for Display, Audio, Controls, and Theme.
- `AboutScene`: technology and current-milestone information.

`UiButton` provides common bounds, label rendering, hover and selected states, click hit-testing, and delta-time-based visual transitions. `Theme.hpp` centralizes the shell's colors, spacing, type sizes, and animation speed. `AssetManager` loads each required Inter font once, and CMake copies the assets beside the GUI executable.

## Local persistence layer

The SQLite-backed persistence module lives under `src/persistence` and links as the SFML-independent `GameVerseArenaPersistence` library. Only `GameVerseArenaGUI` links it; the 14-game `GameVerseArena` console target remains SQLite-independent.

- `Database` owns the SQLite connection and statement lifetime through RAII, applies a 3000 ms busy timeout, enables foreign keys, initializes schema once, and provides transaction boundaries.
- `DatabasePaths` resolves the production file to `%LOCALAPPDATA%\GameVerseArena\gameverse.db` on Windows, XDG/home application data on other platforms, or a portable temporary-directory fallback. Database construction creates a missing parent directory. Tests inject an explicit temporary path and never open production storage.
- `ProfileRepository` contains all profile and active-selection SQL. User values are bound through prepared statements; no GUI source contains raw SQL.
- `ProfileService` owns name normalization/validation, uniqueness checks, first-run bootstrap, active-profile changes, and safe delete replacement rules.

Schema v2 is tracked with `PRAGMA user_version`. Version 0 first creates the v1 `profiles` and `app_state` schema, then a transactional v1-to-v2 migration adds `matches`; existing profiles and active selection are never recreated. Version 2 reopens idempotently and unknown future versions fail startup without modification. Foreign keys prevent orphaned active state and `matches.profile_id REFERENCES profiles(id) ON DELETE CASCADE` deliberately removes local history when a profile is deleted.

`matches` stores typed game/mode/result/difficulty keys, both match-time display names/sides, final scores, optional draw count, match format, monotonic `duration_ms`, and UTC epoch-millisecond `started_at`/`completed_at`. Indexes on `(profile_id, completed_at)`, `(profile_id, game_key, completed_at)`, and `(profile_id, result, completed_at)` serve newest-first, game-filter, and result-filter queries.

- `MatchRepository` owns prepared insertion, newest-first pagination, counts, and SQL game/result filters.
- `MatchService` validates terminal records and profile existence before insertion.
- `StatisticsRepository` derives overall and per-game aggregates and streaks from history. There are no persistent aggregate counters.
- `MatchRecorder` is an SFML-independent completion adapter shared by setup/game scenes. It captures the active profile ID at start, maps pure session winners from that profile's perspective, measures active time with `steady_clock`, and marks a match finalized before its one database attempt.

The completion flow is `setup capture -> pure game/session -> terminal-state mapping -> MatchService -> MatchRepository`. Setup time, result-overlay time, and paused Ping Pong time are excluded. Rematch/New Match resets timing. Abandoning setup/game/library flows invalidates the recorder without a write. Save failure leaves the in-memory result intact, logs context, and shows a non-blocking overlay warning.

On first run the service creates exactly one `Player 1` and makes it active. Activating a profile persists its ID and advances `last_used_at`. Deleting an inactive profile leaves the active selection alone; deleting the active profile chooses the most recently used remaining profile; deleting the final profile recreates and activates `Player 1`.

`Application` initializes the database and bootstraps the service before registering scenes. A startup failure is reported to stderr and in a Windows fatal error dialog. Both setup scenes request the active display name on activation and copy it into their editable Player 1 field. That copy is a per-match default only: setup edits never call rename and match results are not stored.

`GameVerseArenaPersistenceTests` and `GameVerseArenaMatchHistoryTests` use only temporary injected database files and open no SFML window. They cover schema migration/reopen/version rejection, profiles, validation, inserts, SQL-safe text, filters, pagination, profile isolation/cascade, derived aggregates/streaks, game-result mapping, abandonment, and duplicate finalization.

## Graphical Classic Tic-Tac-Toe module

The first migrated game is isolated under `src/games/classic_tic_tac_toe` and links to the GUI as the `GameVerseArenaTicTacToe` library:

- `TicTacToeBoard` is a pure C++ 3×3 board with legal-move validation, status detection, move count, reset, and winning-line coordinates.
- `TicTacToeAI` is SFML-independent. Easy chooses a random legal move, Medium uses tactical priorities, and Hard uses depth-aware Minimax with alpha-beta pruning.
- `TicTacToeSession` owns configuration, current turn, round lifecycle, match score, best-of completion, mark assignment, and duplicate-result protection.

The graphical scenes read and mutate this focused session through its public API. They do not use or modify `Board<T>`, `Move<T>`, `Player<T>`, `GameManager<T>`, or the console `XO_Classes` implementation. This boundary keeps the console game stable while allowing event-driven GUI input and animation.

`GameVerseArenaTests` links only to the pure library and is registered with CTest. It covers board rules, session scoring and lifecycle, all AI levels, and recursive Hard-AI no-loss validation without opening an SFML window.

## Graphical Ping Pong module

Ping Pong is isolated under `src/games/ping_pong` and links to the GUI as the SFML-independent `GameVerseArenaPingPong` library:

- `PingPongTypes` defines focused numeric vectors, field/paddle/ball state, controls, modes, difficulty, score, and match lifecycle without SFML types.
- `PingPongSimulation` owns deterministic paddle movement, ball integration, wall/paddle collisions, impact-angle response, rally acceleration, and one-shot scoring. It uses a 1280 x 720 logical coordinate system with a 1140 x 500 playfield, 470 base ball speed, 4% contact growth, and a 900 speed cap.
- `PingPongAI` returns only a movement intention and a legal speed scale. Easy follows the current ball with delay and error; Medium and Hard predict the right-paddle intercept with reflected top/bottom travel. Production RNG is seeded once, while tests can inject a seed.
- `PingPongSession` owns trimmed names, mode, AI difficulty, the 0-0 score, first-to-5 winner detection, duplicate-point protection, point pause, three-second serve countdown, and rematch configuration preservation.

`PingPongSimulation::step` integrates movement and resolves a crossed scoring boundary before any wall or paddle contact, so a completed point cannot mutate the rally count or speed. Paddle contacts separate the ball to the outgoing face before the next fixed step. At the 900 logical-pixel/s cap, one 1/120-second step travels at most 7.5 pixels on either axis; this is smaller than the 24-pixel ball diameter and the 42-pixel horizontal paddle contact interval (paddle depth plus ball diameter). Deterministic maximum-speed tests cover both paddles and both walls, so swept collision is not currently needed.

`PingPongGameScene` accumulates render delta time and advances physics at 1/120 second. It clamps its accepted frame delta to 100 ms, processes at most eight catch-up steps, and discards excess backlog to avoid a spiral after dragging, focus changes, or a debugger stall. Its active flow is `ServeCountdown` -> `Playing` -> `PointScored` -> `ServeCountdown`, with `MatchFinished` replacing `PointScored` on the winning point; pause is a scene-level overlay that preserves whichever logical state it interrupted. Pausing returns before simulation, AI decisions, and session timers, so all of them freeze. Resume clears accumulated frame time and continues the same logical state. Focus loss clears all held keys and pauses; focus gain clears held keys again, requires fresh input, and does not resume automatically. Rematch resets score, physics, AI timing, held input, trail, and frame accumulation. Leaving through Setup or Library stops all updates because only the active scene is updated, and reactivation clears transient state before play.

`GameVerseArenaPingPongTests` links only the pure Ping Pong library. It verifies simulation initialization and clamping; a deterministic 120,000-step long rally; finite/bounded state and speed invariants; maximum-speed paddle and wall contacts; collision separation, angle, and one-growth-per-contact rules; scoring order and one-shot completion; 20,000-step replay determinism; session lifecycle; AI movement bounds and difficulty ordering; multi-wall intercept reflection; fixed-step reaction cadence; and deterministic randomized aim without opening an SFML window.

The GUI Tic-Tac-Toe lifecycle cancels its non-blocking AI timer before Restart Round, Rematch, New Setup, Return to Library, or confirmed match exit. Result and exit overlays consume input before board handling, so no board move is accepted while either overlay is active. When the computer owns X, scene activation schedules one opening; after that move the pure session hands the turn to O, preventing a second automatic opening. The pure Tic-Tac-Toe tests cover this Computer-as-X handoff in addition to duplicate score protection and finished-board move rejection.

The Tic-Tac-Toe path remains event-driven and turn-based: discrete moves update a board/session model and AI chooses a discrete cell. Ping Pong instead consumes continuous control intentions and advances numeric state through fixed real-time steps. Neither graphical game depends on the console implementation, and Ping Pong is not forced into `Board<T>`, `Move<T>`, `Player<T>`, or `GameManager<T>`.

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
- Classic Tic-Tac-Toe is the graphical turn-based game, Ping Pong is the only graphical arcade game, and the other 13 board games remain console-only.
- Settings are labeled previews and do not persist or change application behavior. Player profiles, completed matches, and the active selection are persistent local application data.
- Ping Pong currently supports local two-player and local Human-vs-Computer play only; it has no audio, controller support, or networking. Completed matches are tracked locally.
- The shared framework assumes two players taking discrete, alternating turns.
- Game completion is expressed through `Board<T>` win, loss, and draw queries.
- Individual modules contain their existing rule, presentation, input, and computer-player behavior; these have not been reorganized.
- Word Tic-Tac-Toe depends on `dic.txt` being available in the process working directory. The CMake build places a copy beside the executable.

This framework is specifically a turn-based board-game layer. A real-time game has different needs, including frame updates, delta time, continuous input, physics, and collision detection. Real-time games must not be forced into `Board<T>`, `Move<T>`, or the existing `GameManager<T>` loop.

The SFML scene layer is currently independent of the console framework. Future tasks must use an explicit integration boundary rather than moving console presentation, `Board<T>`, or `GameManager<T>` directly into GUI scenes.
