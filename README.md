# GameVerseArena

GameVerseArena is a C++17 games platform with two independently buildable applications:

- `GameVerseArena`, the existing console collection of 14 turn-based board games, including shared player setup, Human/Computer selection, result detection, and an in-memory scoreboard.
- `GameVerseArenaGUI`, an SFML 3.1.0 graphical application with a launcher, local player profiles, persistent match history, derived statistics and achievements, persistent audio and accessibility settings, procedural sound effects, a game library, graphical versions of the original board games (Classic Tic-Tac-Toe plus the games listed under [Board games in the GUI](#board-games-in-the-gui)), and real-time Ping Pong.

Classic Tic-Tac-Toe, Numerical Tic-Tac-Toe, SUS, 5x5 Tic-Tac-Toe, Misere Tic-Tac-Toe, and Ping Pong are playable in the GUI. The other 9 original board games remain playable in the console application only. Ping Pong is a separate GUI arcade game, so the original console collection remains 14 board games rather than becoming a 15-game board collection.

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

The graphical target uses the SFML 3.1.0 Graphics and Audio modules. CMake fetches the pinned release from the official SFML repository, and SFML's own build fetches the Ogg 1.3.6, FLAC 1.5.0, and Vorbis 1.3.7 sources its Audio module depends on, so the first configure requires an internet connection and may take several minutes. SQLite 3.53.4 is vendored from the official amalgamation and requires no installed DLL, package manager, or network access. The console sources remain independent of SFML (including SFML Audio) and SQLite and can still be compiled directly.

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
cmake --build build --config Release --target GameVerseArenaPersistenceTests
cmake --build build --config Release --target GameVerseArenaMatchHistoryTests
cmake --build build --config Release --target GameVerseArenaAchievementTests
cmake --build build --config Release --target GameVerseArenaSettingsTests
cmake --build build --config Release --target GameVerseArenaSchemaSafetyTests
cmake --build build --config Release --target GameVerseArenaAudioTests
cmake --build build --config Release --target GameVerseArenaAudioEngineTests
cmake --build build --config Release --target GameVerseArenaCatalogueTests
cmake --build build --config Release --target GameVerseArenaBoardGameSessionTests
cmake --build build --config Release --target GameVerseArenaNumericalGameTests
cmake --build build --config Release --target GameVerseArenaSusGameTests
cmake --build build --config Release --target GameVerseArenaFiveByFiveGameTests
cmake --build build --config Release --target GameVerseArenaMisereGameTests
ctest --test-dir build --output-on-failure
```

No test needs a window or sound hardware: the audio-engine tests use SFML's silent null playback device, and every persistence test uses temporary database files.

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

## Local player profiles

Choose **Profiles** from the main menu to create, rename, delete, and select local player profiles. Profile names are trimmed, limited to 24 Unicode text units, reject control characters, and must be unique without regard to ASCII letter casing. A fresh database contains exactly one active `Player 1` profile.

The active profile persists across application restarts and supplies the initial Player 1 name in every game's setup. That setup field remains freely editable for each match; editing it affects only the match and never renames the stored profile. Deleting the active profile automatically selects the most recently used remaining profile. Deleting the last profile safely recreates and activates `Player 1`.

On Windows, profile data is stored at `%LOCALAPPDATA%\GameVerseArena\gameverse.db`. Other platforms use `$XDG_DATA_HOME/GameVerseArena/gameverse.db`, then `$HOME/.local/share/GameVerseArena/gameverse.db`, with the system temporary directory as a final fallback. Profiles, history, statistics, and achievements remain local to the machine; cloud synchronization is not implemented.

## Match history and statistics

Select a profile under **Profiles**, then choose **View Stats**. The statistics view shows overall results, play time, last-played time, current and best win streaks, and focused Classic Tic-Tac-Toe and Ping Pong totals; every other graphical game counts toward the overall totals. **Recent Matches** opens a bounded, newest-first history with game and result filters plus Previous/Next pagination.

Every game played in the graphical app is tracked; the console application keeps no history. The persistent owner is the active profile captured when the match starts; editing Player 1's match-time display name does not change ownership. Guests and computers never receive hidden profiles. Human-vs-Computer results are stored from the active human profile's perspective.

One row is written only when a complete Single Game, Best of 3, Best of 5, or first-to-5 Ping Pong match, or a board game played in the shared board-game scenes, reaches its final result. Tic-Tac-Toe rounds are not separate history rows. Abandoned matches, incomplete restarts, setup screens, and Return-to-Library before completion are not recorded. A focused completion guard permits one persistence attempt per match even when overlays keep updating or receive repeated input.

Statistics are always recomputed from completed match history; profile rows contain no duplicated win/loss counters. Win rate is wins divided by all completed matches, including draws in the denominator. A loss or draw breaks a win streak. Deleting a profile cascades deletion to its local history.

Each shared board game is recorded as one single game. SUS and 5x5 Tic-Tac-Toe store both players' points; the other board games store only the result. Their computer opponents have one strategy each, stored as the `standard` difficulty. In Human vs Human play the stored side is Player 1's, which is always the side that moves first.

History rows store UTC epoch-millisecond start/completion timestamps and a monotonic active duration. Setup and result-overlay time are excluded, and paused Ping Pong time is excluded. Three history indexes support newest-first profile pages and SQL game/result filters without loading an unbounded list.

The current database schema is version 4. It adds app-wide settings and widens the history table's game list so that every graphical game can be recorded; upgrading an older database keeps every existing row. A database created by a newer, unknown schema version is refused without being modified, and a non-empty SQLite file that is not a GameVerseArena database is never adopted.

## Command-line options

- `--database <path>` uses an explicit database file instead of the per-user default. Use it for any destructive testing so real profile data is never touched.
- `--silent-audio` routes sound to SFML's silent null device.
- `--smoke-test <output-dir>` is a developer verification mode: it drives the real scenes with synthetic keyboard and mouse input, checks scene changes, settings persistence, and one-row-per-match history, and writes PNG captures at 1280 x 720 and 960 x 540 plus `smoke-test.log` to the output directory, then exits with 0 when every check passed. It requires `--database` with a path that does not exist yet and refuses the production database.

Unknown options are rejected. On Windows the command line, the executable location, and `%LOCALAPPDATA%` are read as Unicode, so folders with non-ASCII names work.

## Player achievements

From a profile's **Statistics** view, the summary shows `Achievements: X / 12`; choose **Achievements** to browse all items, filter by All, General, Tic-Tac-Toe, or Ping Pong, and scroll through bounded cards. Unlocked cards show their first UTC unlock date. Locked cards show clamped numeric progress when a meaningful target exists; Clean Sweep and Clutch Finish show only their locked condition instead of invented fractional progress.

The 12 achievements are:

1. **First Victory** — win one match.
2. **Arena Regular** — complete 10 matches.
3. **Dedicated Player** — complete 25 matches.
4. **On a Roll** — reach a best win streak of 3.
5. **Unstoppable** — reach a best win streak of 5.
6. **Versatile Player** — win at least one Classic Tic-Tac-Toe match and one Ping Pong match.
7. **XO Winner** — win one Classic Tic-Tac-Toe match.
8. **X Marks the Spot** — win Classic Tic-Tac-Toe while playing X.
9. **O Takes the Win** — win Classic Tic-Tac-Toe while playing O.
10. **Pong Winner** — win one Ping Pong match.
11. **Clean Sweep** — win Ping Pong 5-0.
12. **Clutch Finish** — win Ping Pong 5-4.

Conditions and progress are recomputed from authoritative completed-match history; no achievement progress counters are stored. Only the profile ID, achievement key, and immutable first-unlock timestamp are persisted. Existing profiles are evaluated at startup after schema migration, and qualifying achievements receive the current UTC recognition time rather than a fabricated historical date. Recognition is idempotent, database uniqueness prevents duplicates, and profile deletion cascades both history and unlock facts.

After a completed match is saved successfully, its persistent profile is evaluated and each newly unlocked achievement is shown once in a non-blocking 3.5-second queue, with one unlock chime per toast. Multiple unlocks appear sequentially and may be dismissed by clicking the toast. A failed match-history write never triggers achievement evaluation. This system is local-only and intentionally has no XP, levels, currency, rewards, leaderboard, networking, or cloud component.

## Settings, audio, and Reduced Motion

Choose **Settings** from the main menu. Settings apply immediately, are saved automatically, and are shared by every profile on this computer:

| Setting | Range | Default |
| --- | --- | --- |
| Master Volume | 0-100% | 80% |
| UI Volume | 0-100% | 70% |
| Gameplay Volume | 0-100% | 80% |
| Achievement Volume | 0-100% | 85% |
| Mute All | On / Off | Off |
| Reduced Motion | On / Off | Off |

Each category's loudness is Master Volume multiplied by the category volume. Mute All silences everything, including sounds already playing, without changing the stored levels. **Reset to Defaults** restores the table above. Use Up/Down or Tab to move, Left/Right to adjust by 5% (hold Shift for 1%), Home/End for 0% or 100%, Enter or Space to toggle, and Escape or **Back** to return; the mouse can click or drag a volume track and click a toggle. Stored values that are malformed or out of range fall back to their defaults and are reported on the Settings screen.

All sound effects are short, original tones generated in memory when the application starts; no audio files are shipped and there is no background music. Sounds are grouped into three categories:

- UI: focus/hover changes, confirm, back, and validation errors.
- Gameplay: board-game moves (one tone for the side that moves first, another for the second), scoring moves in SUS and 5x5 Tic-Tac-Toe, and win/draw/loss results; Ping Pong paddle hits, wall bounces, points, and the match result.
- Achievement: the unlock chime when an achievement toast appears.

At most 12 effects play at once; a new effect reuses the oldest voice. If no audio output device is available, the application runs silently and Settings still work and save.

Reduced Motion turns off decorative motion: button hover/focus easing, the achievement toast slide-in, the mark pop-in and selection pulse on every board, and the Ping Pong ball trail. Focus highlights, marks, and every result message stay visible, and gameplay speed, physics, AI timing, and timers are never changed.

## GUI shell controls

- Move the pointer over a button to highlight it; click the left mouse button to activate it.
- Use Up and Down to change the selected main-menu item and Enter to activate it.
- Keyboard actions activate once per key press; release the key before activating another scene or overlay action.
- Tab moves focus forward and Shift+Tab moves it backward on menus, setup screens, and overlays.
- Press Escape on Profiles, Game Library, Settings, or About to return to the main menu. In profile edit/delete overlays, Escape cancels the overlay first.
- Press Escape on the main menu to close the application.
- Resize the window normally; the 16:9 interface view scales while preserving the layout. The practical design size is 960 x 540 or larger, with a default window size of 1280 x 720 and a 60 FPS frame limit.
- In Achievements, Tab/Left/Right selects category or Back controls; Up/Down, Page Up/Page Down, or the mouse wheel scrolls the bounded list.

## Game library

Choose **Play** to open the game library. It lists every game in one searchable, scrollable grid: the 14 original board games in console-menu order, then Ping Pong. Each card shows the game's name, category (BOARD or ARCADE), a one-line description, its board, the player modes, and whether it is **PLAYABLE** in the graphical app or, for board games whose graphical version is not available yet, **CONSOLE ONLY**. Choosing a console-only game explains that it is playable in the console application instead of opening an empty screen.

- Type anywhere to search by name; Backspace edits, and Escape clears the search before leaving the library.
- **All**, **Board**, and **Arcade** filter the grid; with the filters focused, Left/Right switches between them.
- Tab and Shift+Tab move between the search box, the filters, the game grid, and **Back**. In the grid, the arrow keys move the focus, Page Up/Page Down move it nine games (one screenful) at a time, Home/End jump to the first or last game, and Enter or Space opens the focused game.
- With the mouse, hover a card to focus it (while you are typing a search, hovering only highlights it), click to open it, use the wheel to scroll, and click the search box, a filter, or **Back**.

The library is driven by a single catalogue (`src/catalogue`) that also supplies each game's rules, player modes, truthful description of its computer opponent, tournament and history eligibility, and its identity in the console menu. `GameVerseArenaCatalogueTests` reads `XO_Demo.cpp` and checks that the catalogue describes exactly the 14 console games plus Ping Pong.

## Classic Tic-Tac-Toe in the GUI

Choose **Play**, select **Classic Tic-Tac-Toe**, complete Player Setup, and start the match. The graphical version supports:

- Human vs Human and Human vs Computer.
- Human choice of X or O; X always opens the round.
- Easy AI (random legal move), Medium AI (win, block, center, corner priority), and unbeatable Hard AI (alpha-beta Minimax).
- Single Game, Best of 3, and Best of 5 matches, with draws tracked separately.
- Next Round, Restart Round, New Match, Rematch, New Setup, and Return to Library flows as appropriate.

Setup is fully keyboard accessible: use Up/Down or Tab to move, Left/Right to change choices, Enter to edit names or activate a control, Backspace to edit, and Escape to go back. During a match, use the arrow keys to select a board cell and Enter or Space to play it. Mouse hover and click are supported throughout. Escape or **Back to Library** opens a confirmation before discarding an active in-memory match.

The active profile provides Player 1's initial setup name. Match-specific display-name edits are preserved in history while ownership remains tied to the captured profile ID.

## Board games in the GUI

Numerical Tic-Tac-Toe, SUS, 5x5 Tic-Tac-Toe, and Misere Tic-Tac-Toe are played in shared board-game scenes. Choose one in the library to open its setup:

- **Game mode**: Human vs Human or Human vs Computer.
- **Player names**: Player 1 starts as the active profile's name; both names can be edited for this game only.
- **Your side** (against the computer): play the side that moves first or the side that moves second. In Human vs Human play, Player 1 always takes the side that moves first.
- The right-hand panel shows the game's rules and describes its computer opponent.

During a game, the side panels show each player's side, name, points (SUS and 5x5), and whose turn it is, and the line under the board says what to do next. Move the cell cursor with the arrow keys and play with Enter or Space, or click a cell. In Numerical Tic-Tac-Toe, choose a number with the number keys or the tray under the board (the lowest unused number is preselected each turn) and then place it. Tab moves the focus to **Restart Game**, **Rules**, and **Back to Library**; F1 opens the rules. Escape or **Back to Library** asks for confirmation before discarding a game in progress. The computer searches on a background thread, so the window stays responsive, and it waits at least 0.35 seconds before moving.

When a game ends, the result panel names the winner and the reason and shows the final points where the game has them. **Rematch** starts the next game with the same players, **View Final Board** (or Escape) hides the panel so the finished board can be studied, **New Setup** returns to the setup, and **Return to Library** leaves. Restarting a game in progress discards it without recording it.

The graphical versions keep the console games' rules and computer strategies, with these deliberate differences:

- Numerical Tic-Tac-Toe credits the player whose placement completes a 15-line, as the console game's result message does; the console scoreboard credits Player 1 for every Numerical win because its result check does not know who moved last.
- The 5x5 computer looks for a cell that makes a *new* three-in-a-row for itself, then for one that would give the opponent a new one (to block it). The console version tested whole-board totals instead: once the computer held any three it always took the first free cell, and once the opponent held any three it stopped blocking and took the first free cell whenever it had no new three of its own to make.

## Ping Pong in the GUI

Choose **Play**, select **Ping Pong** (the last card, or use the **Arcade** filter or search), configure the match, and select **Start Match**. Every match is first to 5 points with no win-by-two rule.

- Human vs Human: W/S controls the left paddle and Up/Down controls the right paddle.
- Human vs Computer: the human controls the left paddle with either W/S or Up/Down.
- Easy AI reacts every 190 ms, moves at 68% paddle speed, follows the current ball position, and uses broad aiming error.
- Medium AI reacts every 105 ms, moves at 84% speed, predicts the intercept including wall reflections, and uses moderate error.
- Hard AI reacts every 52 ms, uses the full legal paddle speed, predicts reflected intercepts, and uses small controlled error.

The ball starts at 470 logical pixels per second. Each successful paddle contact increases its speed by 4%, capped at 900, and every point resets it to base speed. After a point, play freezes briefly, the scorer is shown, the paddles and ball reset, and a three-second serve countdown begins.

Press Escape or select **Pause** to stop physics, AI timers, and the serve clock. The pause menu provides Resume, Restart Match, New Setup, and Return to Library. Losing window focus clears held movement and opens the pause overlay. The winner overlay provides Rematch, New Setup, and Return to Library.

Ping Pong physics run at a fixed 1/120-second step independently of rendering. The pure Ping Pong tests do not create a window or link SFML.
