# Current Architecture

## Application entry points

`XO_Demo.cpp` contains `main()` for the existing `GameVerseArena` console application. It initializes the process-wide random number generator, collects the two players' names and Human/Computer types, displays the main and game-selection menus, starts the selected game, detects its result, and updates the session scoreboard.

`src/gui/main.cpp` contains `main()` for the separate `GameVerseArenaGUI` executable. `Application` owns the SFML 3.1.0 window, runtime asset manager, responsive view, event/update/render loop, and scene manager. The GUI runs at a 1280 x 720 logical size with a 60 FPS limit and clamps long frame times before updating animations. Ping Pong performs its own fixed-step accumulation inside its game scene.

## Graphical shell

`SceneManager` owns the scenes and changes the active scene without transferring or exposing ownership. Its activation callback lets stateful scenes safely initialize transient work such as an AI-turn timer. Scene switches are synchronous, but the event that requests a switch finishes in the old scene; application-wide key repeat is disabled so a held activation key cannot immediately trigger an overlay or control in the newly active scene. The shell contains:

- `MainMenuScene`: Play, Profiles, Settings, About, and Exit navigation with keyboard and mouse input.
- `ProfilesScene`: bounded/scrollable local profile selection plus Create, Rename, Delete, Set Active, View Stats, and Back actions. `ProfileEditOverlay` and `ProfileDeleteOverlay` own transient input and confirmation presentation; all validation and state transitions remain in `ProfileService`.
- `ProfileStatsScene`: read-only statistics for the selected profile: a scrollable table with an All games row and one row per game (`StatisticsRepository::perGame`, one grouped query), a detail card for the selected row (`overall`/`forGame`), the zero-history empty state, the achievement summary, and navigation to history or achievements.
- `ProfileAchievementsScene`: profile-scoped achievement cards, All/General/Tic-Tac-Toe/Ping Pong filters, bounded scrolling, first-unlock dates, and history-derived locked progress.
- `MatchHistoryScene`: newest-first bounded pages with repository-level game filters (All or any of the 15 games) and result filters and Previous/Next navigation. The Application owns the selected statistics game (`std::optional<GameKey>`) shared by both scenes, so Recent Matches opens filtered to the selected game and the statistics follow the history's game filter.
- `GameLibraryScene`: a searchable, filterable (All / Board / Arcade), bounded-scrolling three-column grid of every entry in the game catalogue, with keyboard focus zones (search, filters, grid, Back) and mouse hover, click, and wheel support. Every catalogue entry is playable; a game without a graphical version would be labelled CONSOLE ONLY and explain itself instead of opening a scene.
- `TicTacToeSetupScene`: keyboard- and mouse-accessible mode, name, mark, AI, and match-length configuration.
- `TicTacToeGameScene`: event-driven board input, score display, mark animation, non-blocking AI turns, and safe navigation.
- `TicTacToeResultOverlay`: round and match results with next-round, restart, rematch, setup, and library actions.
- `PingPongSetupScene`: local/AI mode, player names, and Easy/Medium/Hard configuration with keyboard and mouse input.
- `PingPongGameScene`: held controls, fixed-timestep simulation, score and arena rendering, focus-loss safety, and scene navigation.
- `PingPongPauseOverlay`: Resume, Restart Match, New Setup, and Return to Library actions while all real-time state is stopped.
- `PingPongResultOverlay`: winner, final score, Rematch, New Setup, and Return to Library actions.
- `BoardGameSetupScene`: the setup shared by every migrated board game: mode, editable names, and the human's side against the computer, next to the catalogue's rules and computer description.
- `BoardGameScene`: the shared play scene: seat panels with points and turn badges, the game's board view, status and help lines, Restart/Rules/Back focus with Tab, background computer moves, sounds, exit confirmation, and one-time recording. `BoardGameResultOverlay` (Rematch, View Final Board, New Setup, Return to Library) and `BoardGameRulesOverlay` are its overlays.
- `SettingsScene`: interactive app-wide settings: Master, UI, Gameplay, and Achievement volume sliders (keyboard steps and mouse click/drag), Mute All and Reduced Motion toggles, Reset to Defaults, and Back. Every change is applied immediately through `SettingsController` and saved through `SettingsService`.
- `AboutScene`: technology and current-milestone information.

The game catalogue (`src/catalogue`, library `GameVerseArenaCatalogue`) is SFML-independent, immutable, project-owned metadata for all 15 graphical games: stable key (identical to the persistence game key), display name, one-line description, rules, board summary, category, player modes, a truthful description of the computer opponent, seat labels, tournament and history eligibility, launch kind (its own dedicated scenes, a board-game factory, or console-only while the game awaits graphical migration), the optional factory, and the game's console menu number, label, and legacy board class. `GameLauncher` binds catalogue entries to scenes, so the library contains no per-game code; at startup `Application` verifies that every entry the catalogue marks playable can actually be launched, so the library can never advertise a game it cannot start. `GameVerseArenaCatalogueTests` checks unique keys and names, agreement with the persistence keys, complete and consistent descriptors, search and filtering, and parses `XO_Demo.cpp` to prove the catalogue matches the console's 14 menu entries and their board classes.

`UiButton` provides common bounds, label rendering, hover and selected states, click hit-testing, and delta-time-based visual transitions that snap to their final state when Reduced Motion is on. `Theme.hpp` centralizes the shell's colors, spacing, type sizes, and animation speed. `AssetManager` loads each required Inter font once, and CMake copies the assets beside the GUI executable.

Every scene receives an `AppContext`: the fonts, `SceneManager`, window, `AudioEngine`, and `SettingsController`. Game and persistence services are still passed explicitly to the scenes that need them. `Application` declares the scenes after the context and services they reference, so scenes are destroyed first. `Application::dispatch` is the single event path used by the real loop and by the `--smoke-test` developer harness (`SmokeTestDriver`), which drives real scenes with synthetic SFML events and saves PNG captures at 1280 x 720 and 960 x 540.

Audio is split into an SFML-independent `src/audio` library and the SFML-backed `AudioEngine`. `ProceduralSynth` generates every effect as deterministic 16-bit PCM in memory; `SoundTypes` maps effects to the UI, Gameplay, and Achievement categories and computes Master x category volume; `AudioPolicy` chooses a voice (idle first, otherwise the oldest) and throttles immediate duplicates. `AudioEngine` owns one `sf::SoundBuffer` per effect and a fixed pool of 12 `sf::Sound` voices (buffers are declared first so voices are destroyed before the buffers they use). It never throws: with no output device (or only SFML's fallback null device) it stays silent and reports why. Mix changes apply to voices already playing.

`AchievementToast` is application-wide presentation over the active scene. Its SFML-independent `AchievementNotificationQueue` displays one newly recognized achievement for 3.5 seconds, accepts delta-time updates and early click dismissal, preserves multiple unlocks in order, and exposes a display sequence number that increases once per visible notification. The toast uses that sequence to play the unlock chime and start its optional slide-in exactly once per notification. It dismisses only on a click whose press and release both land on the toast, so it never swallows the release of a drag that began in the scene.

## Local persistence layer

The SQLite-backed persistence module lives under `src/persistence` and links as the SFML-independent `GameVerseArenaPersistence` library. Only `GameVerseArenaGUI` links it; the 14-game `GameVerseArena` console target remains SQLite-independent.

- `Database` owns the SQLite connection and statement lifetime through RAII, applies a 3000 ms busy timeout, enables foreign keys, initializes schema once, and provides transaction boundaries.
- `DatabasePaths` resolves the production file to `%LOCALAPPDATA%\GameVerseArena\gameverse.db` on Windows, XDG/home application data on other platforms, or a portable temporary-directory fallback. Database construction creates a missing parent directory. Tests inject an explicit temporary path and never open production storage.
- `ProfileRepository` contains all profile and active-selection SQL. User values are bound through prepared statements; no GUI source contains raw SQL.
- `ProfileService` owns name normalization/validation, uniqueness checks, first-run bootstrap, active-profile changes, and safe delete replacement rules.

The schema is tracked with `PRAGMA user_version` and is currently version 4. Version 0 first creates the v1 `profiles` and `app_state` schema (only in an empty file; a non-empty SQLite file without a version is refused unchanged), a transactional v1-to-v2 migration adds `matches`, a v2-to-v3 migration adds `achievement_unlocks`, and a v3-to-v4 migration adds `app_settings` and rebuilds `matches` with CHECK constraints that accept all 15 graphical game keys and the `standard` difficulty, copying every row with its original id and recreating the three history indexes. Each step re-reads the version inside its own `BEGIN IMMEDIATE` transaction, so concurrent first launches apply each step exactly once, and the v4 step verifies `PRAGMA foreign_key_check(matches)` before committing. Committed migration SQL is never edited. Unknown future or negative versions fail startup without modification. Foreign keys prevent orphaned active state. Both `matches.profile_id` and `achievement_unlocks.profile_id` reference `profiles(id) ON DELETE CASCADE`, so profile deletion removes local history and unlock facts.

`app_settings(key, value, updated_at)` stores app-wide settings independently of profiles. `SettingsRepository` owns its SQL; `SettingsService` parses values strictly (plain decimal 0-100 volumes, `0`/`1` flags), falls back to the documented default for any malformed or out-of-range stored value and reports it, and saves all six settings atomically after rejecting out-of-range input. `SettingsController` (GUI) keeps the active settings, pushes them to the audio mix and Reduced Motion immediately, and keeps a change active for the session even if saving fails.

`matches` stores typed game/mode/result/difficulty keys, both match-time display names/sides, final scores, optional draw count, match format, monotonic `duration_ms`, and UTC epoch-millisecond `started_at`/`completed_at`. Indexes on `(profile_id, completed_at)`, `(profile_id, game_key, completed_at)`, and `(profile_id, result, completed_at)` serve newest-first, game-filter, and result-filter queries.

`achievement_unlocks(profile_id, achievement_key, unlocked_at)` has a composite primary key on `(profile_id, achievement_key)`. It stores only durable first-recognition facts. Progress, match counts, streaks, mark wins, and score-pattern state are not stored in this table.

- `MatchRepository` owns prepared insertion, newest-first pagination, counts, and SQL game/result filters.
- `MatchService` validates terminal records and profile existence before insertion.
- `StatisticsRepository` derives overall and per-game aggregates and streaks from history. `perGame` returns one summary per game in `allGameKeys()` order (zero rows for unplayed games) from a single `GROUP BY game_key` query; `forGame` adds each game's own figures, including the matches played on each shared board-game side. There are no persistent aggregate counters.
- `StatisticsRepository` also supplies a compact achievement snapshot: total matches/wins, best streak, per-game wins, Tic-Tac-Toe wins by X/O, and Ping Pong 5-0/5-4 win existence. Startup backfill uses one aggregate query plus one ordered all-profile streak scan rather than a full-history scan per achievement or profile.
- `AchievementRepository` owns all unlock SQL. `INSERT OR IGNORE` and the composite primary key preserve the first timestamp and provide duplicate protection.
- `AchievementService` coordinates snapshot evaluation, status/progress views, post-match recognition, and startup backfill. It returns only newly inserted catalogue items for toast presentation.
- `MatchRecorder` is an SFML-independent completion adapter shared by setup/game scenes. It captures the active profile ID at start, maps pure session winners from that profile's perspective, measures active time with `steady_clock`, and marks a match finalized before its one database attempt.

The completion flow is `setup capture -> pure game/session -> terminal-state mapping -> MatchService -> MatchRepository -> AchievementService -> AchievementRepository -> toast queue`. Achievement evaluation occurs only when the recorder reports that the completed-match insert succeeded. Setup time, result-overlay time, and paused Ping Pong time are excluded. Rematch/New Match resets timing. Abandoning setup/game/library flows invalidates the recorder without a write. A history-save failure leaves the in-memory result intact, logs context, shows a non-blocking warning, and skips achievement evaluation.

The project-owned achievement domain lives under `src/achievements`. `AchievementCatalogue` contains the immutable metadata for exactly 12 achievements. `AchievementEvaluator` explicitly evaluates those 12 conditions from an `AchievementSnapshot`; it is intentionally not a generic rule engine. `AchievementProgress` clamps numeric values to their targets, while Clean Sweep and Clutch Finish expose no fake numeric progress. The domain has no SQLite or SFML dependency.

On startup, `AchievementService::backfillAll` loads compact snapshots and existing unlocks in batches, evaluates every profile, and inserts only missing satisfied keys. Backfilled rows use the current UTC recognition time, never a guessed historical completion time. Repeating startup or post-match evaluation inserts nothing new because both the service and database uniqueness reject duplicates.

On first run the service creates exactly one `Player 1` and makes it active. Activating a profile persists its ID and advances `last_used_at`. Deleting an inactive profile leaves the active selection alone; deleting the active profile chooses the most recently used remaining profile; deleting the final profile recreates and activates `Player 1`.

`Application` initializes the database, bootstraps profiles, loads settings, and performs achievement backfill before registering scenes. A startup failure is reported to stderr and in a Windows error dialog (suppressed in smoke-test mode) that distinguishes a database that could not be opened or upgraded from a later loading failure. A last-resort handler reports any unexpected exception that escapes a scene instead of terminating silently, and the history, statistics, and achievement scenes show an error state if their queries fail. Both setup scenes request the active display name on activation and copy it into their editable Player 1 field. That copy is a per-match default only: setup edits never call profile rename, while the completed-match row retains its match-time display name.

`GameVerseArenaPersistenceTests`, `GameVerseArenaMatchHistoryTests`, and `GameVerseArenaAchievementTests` use only temporary injected database files and open no SFML window. Achievement coverage includes v2-to-v3 preservation/reopen/future rejection, repository insertion/duplicate/timestamp/isolation/cascade, all 12 evaluator boundaries, numeric progress and clamping, score/mark polarity, draw-broken streaks, compact batch snapshots, repeated evaluation/backfill idempotency, recognition timestamps, and notification queue order.

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

## Graphical board games

The original board games are rebuilt for the GUI behind one SFML-independent contract in `src/games/common`, compiled with the games into the `GameVerseArenaBoardGames` library:

- `TurnBasedGame` exposes what generic code needs: the seat to move (`First` always moves first), the outcome (in progress, won by a seat, or drawn, with a short reason), legal moves as opaque non-negative `MoveId`s, `play`, `reset`, `clone`, optional per-seat points, and `chooseComputerMove(random, cancel)`, which must only read the game so it can run on a clone.
- `BoardGameSession` owns one game between two named players: trimmed names with seat defaults, Human vs Human or Human vs Computer with the human's seat, whose turn belongs to the computer, restart, and `takeCompletedOutcome`, which reports each finished game exactly once.
- `ComputerMoveTask` runs a computer search on a worker thread over a clone, delivers its result once through a future, and on cancel or destruction sets the shared cancel flag and joins the thread, so no search outlives the scene.

Each game keeps its typed rules and computer strategy in its own folder (`src/games/numerical`, `sus`, `five_by_five`, `misere`, `four_in_a_row`, `four_by_four`, `pyramid`, `diamond`, `word`, `infinity`, `memory`, `obstacle`, `ultimate`). They are ports of the console modules' rules and strategies, not wrappers: the GUI never uses `Board<T>`, `Move<T>`, `Player<T>`, or `GameManager<T>`, and the console sources are unchanged. Long searches poll the cancel flag: Misere's full minimax and Ultimate XO's depth-limited minimax every 1,024 nodes, and Four-in-a-Row's iterative-deepening search wherever it checks its fixed budget of positions (the deterministic replacement for the console's 1.5-second clock). Each game has a CTest executable with rule, scoring, and computer-strategy cases plus `TurnBasedTestSupport`'s shared contract: seeded random and computer-versus-computer games stay legal and alternate turns (and, for games of bounded length, end), finished games refuse moves, reset restores the opening, clones are independent, play is deterministic per seed, and a cancelled search returns no move. `GameVerseArenaBoardGameSessionTests` covers the session and the background task, including cancellation, replacement, and error propagation.

In the GUI, `BoardGameHost` holds the chosen catalogue entry, the session, and the game's `BoardView`. The library's board-game launcher stores the entry and opens `BoardGameSetupScene`; Start creates the game through the catalogue factory, creates its view through `board_view::createBoardView`, and arms the match recorder. `BoardView` (`src/gui/board`) draws a board inside the area the scene gives it and turns pointer, arrow, Enter, and typed input into complete moves; it never changes the game. `CellBoardView` implements boards made of selectable cells (any arrangement of rectangles, with arrow-key movement to the nearest cell in that direction and wrap-around), and the per-game views add marks, highlights, and game-specific input: Numerical's number tray, Four-in-a-Row's column cursor and landing preview, 4x4's two-step pick-and-slide, Word's A-Z letter tray, Infinity's dimmed and fading marks, Memory's hidden marks (each shown briefly, all revealed at the end, with previews that never reveal whether a cell is taken), Obstacle's hatched blocks, Ultimate XO's nine small boards (the board in play outlined, decided boards marked), and the pyramid and diamond layouts. A view can attach a short message to a refused choice (`Response::message`, from `CellBoardView::refusal`), which the scene shows under the board for a moment. `Application` refuses to start if a catalogue board game has no view.

`BoardGameScene` applies a human move only while no overlay is open and the human is to move, starts the computer's background search on the computer's turn, and applies its move only after at least `Theme::aiThinkingDelay` (0.35 s) and only while no rules panel or exit confirmation is open. A search that fails or returns an illegal move stops the computer with a visible message instead of retrying. When `takeCompletedOutcome` reports the end, the scene plays the result sound, shows the result panel, and records the game once.

Board-game completion uses `MatchRecorder::beginBoardGame`/`restartBoardGame`/`completeBoardGame`. The profile's player is Player 1 (the first seat) in Human vs Human play and the human against the computer. `MatchService` validates board-game rows against `boardGameSides` (stored sides such as `X`/`O`, `S`/`U`, `Odd`/`Even`, or `First`/`Second` for Word Tic-Tac-Toe, in either order), the `single` format, no draw count, the `standard` difficulty for a computer opponent, and points exactly for the games `boardGameRecordsPoints` names (SUS and 5x5). Classic Tic-Tac-Toe and Ping Pong keep their own rules and reject the `standard` difficulty.

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

- The console application's board-game user interface and input model remain console-based and synchronous.
- All 14 original board games are also graphical turn-based games, and Ping Pong is the only graphical arcade game.
- Player profiles, completed matches, first achievement unlock timestamps, the active selection, and app-wide settings are persistent local application data; achievement progress and statistics remain derived.
- Ping Pong currently supports local two-player and local Human-vs-Computer play only; it has no controller support or networking. Completed matches are tracked locally.
- The shared framework assumes two players taking discrete, alternating turns.
- Game completion is expressed through `Board<T>` win, loss, and draw queries.
- Individual modules contain their existing rule, presentation, input, and computer-player behavior; these have not been reorganized.
- The console version of Word Tic-Tac-Toe depends on `dic.txt` being available in the process working directory; the CMake build places a copy beside the console executable. The graphical version has the same words built in.

This framework is specifically a turn-based board-game layer. A real-time game has different needs, including frame updates, delta time, continuous input, physics, and collision detection. Real-time games must not be forced into `Board<T>`, `Move<T>`, or the existing `GameManager<T>` loop.

The SFML scene layer is currently independent of the console framework. Future tasks must use an explicit integration boundary rather than moving console presentation, `Board<T>`, or `GameManager<T>` directly into GUI scenes.
