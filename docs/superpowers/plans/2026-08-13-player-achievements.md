# Player Achievements Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add 12 local, profile-scoped achievements whose conditions and progress are derived from authoritative completed-match history, with durable first-unlock timestamps, startup backfill, post-match notifications, and a browsable achievements scene.

**Architecture:** Migrate SQLite schema v2 to v3 with an `achievement_unlocks` fact table. Keep immutable catalogue metadata and an SFML-independent evaluator under `src/achievements`, keep SQL and unlock uniqueness in persistence repositories, coordinate evaluation/backfill through a focused service, and let GUI scenes only request/display results. Numeric progress is recomputed from a compact statistics snapshot; no progress counter is stored.

**Tech Stack:** C++17, SQLite 3.53.4, SFML 3.1.0, CMake/CTest.

## Global Constraints

- Implement exactly the 12 achievements and copy in the Task 09 specification.
- Preserve all 14 console games; keep the console target SQLite-independent.
- Preserve SFML 3.1.0, SQLite 3.53.4, the FreeType workaround, profiles, match history, and derived statistics.
- Persist only `(profile_id, achievement_key, unlocked_at)`; never persist progress counters.
- Evaluate only after a completed match row is successfully inserted.
- Backfill every existing profile with current UTC recognition time, idempotently and without N+1 full-history scans.
- Keep SQL inside persistence and catalogue metadata in project-owned code.
- Preserve `5x5 Tic Tac Toe.cpp` and `BoardGames.slnx` byte-for-byte.
- Create exactly one final commit: `feat: add player achievements`; push normally to `main`.

---

### Task 1: Schema v3 and achievement unlock repository

**Files:** `src/persistence/Schema.hpp`, `src/persistence/AchievementRepository.*`, `tests/AchievementTests.cpp`, `CMakeLists.txt`.

- [ ] Add migration and repository tests for v2-to-v3 preservation, v3 reopen, future rejection, insert, duplicate protection, timestamp preservation, isolation, listing, and cascade deletion.
- [ ] Register `GameVerseArenaAchievementTests` and run it to verify RED because Task 09 APIs/schema are absent.
- [ ] Implement the transactional migration and prepared unlock repository with `INSERT OR IGNORE`.
- [ ] Re-run achievement and persistence tests to verify GREEN.

### Task 2: Catalogue, compact snapshot, evaluator, progress, and coordinator

**Files:** `src/achievements/AchievementTypes.hpp`, `AchievementCatalogue.*`, `AchievementEvaluator.*`, `AchievementProgress.hpp`, `src/persistence/StatisticsRepository.*`, `src/persistence/AchievementService.*`, `tests/AchievementTests.cpp`.

- [ ] Add failing tests for exact catalogue metadata, all 12 false/true boundaries, mark/score polarity, two-game versatility, streak-breaking draws, progress/clamping/zero history, and nonnumeric conditions.
- [ ] Add focused aggregate/existence queries and construct one evaluation snapshot per profile.
- [ ] Implement the immutable catalogue and explicit evaluator switch; do not add a generic rule engine.
- [ ] Add failing exact-once evaluation/backfill tests, then implement idempotent unlock recognition and batched startup backfill using one timestamp.
- [ ] Re-run achievement tests and strict-compile project-owned achievement/persistence logic.

### Task 3: Post-match flow and toast queue

**Files:** `src/persistence/MatchRecorder.hpp`, `src/gui/AchievementToast.*`, both graphical game scenes, `src/gui/Application.*`, `tests/AchievementTests.cpp`.

- [ ] Add failing tests proving repeated evaluation produces one row, already-unlocked conditions produce no new notification, and toast state preserves sequential order/duration.
- [ ] Make successful recorder completion expose the persistent profile ID without weakening its exact-once match guard.
- [ ] Evaluate only when recorder completion returns true, enqueue returned catalogue items, and leave failed history writes unable to unlock anything.
- [ ] Render one non-blocking 3.5-second toast at a time and update it from SFML delta time.

### Task 4: Profile stats and achievements UI

**Files:** `src/gui/ProfileStatsScene.*`, `src/gui/ProfileAchievementsScene.*`, `src/gui/SceneManager.hpp`, `src/gui/Application.*`.

- [ ] Show `Achievements: X / 12` and add an Achievements button from Profile Stats.
- [ ] Build bounded keyboard/mouse scrolling, category filters, locked/unlocked cards, UTC unlock dates, and meaningful progress text.
- [ ] Sort unlocked items by newest timestamp first and locked items in catalogue order.
- [ ] Keep the existing 1280x720 logical layout usable through the exact 960x540 minimum view.

### Task 5: Documentation and verification

**Files:** `README.md`, `docs/CURRENT_ARCHITECTURE.md`, `AGENTS.md`.

- [ ] Document the 12 local achievements, schema v3, boundaries, history-derived progress, recognition-time backfill, exact-once flow, toast queue, cascade deletion, and absence of XP/levels.
- [ ] Run the full build, all registered CTests, strict warning build, direct console build/smoke/removal, and feasible isolated GUI validation.
- [ ] Audit ignored/generated artifacts, recheck protected hashes, stage only Task 09 files, create exactly one commit, push `main`, verify `origin/main`, and report final status.
