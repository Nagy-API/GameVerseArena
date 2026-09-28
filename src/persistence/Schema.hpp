#pragma once

namespace persistence::schema {

inline constexpr int currentVersion = 4;

inline constexpr const char* createVersionOne = R"sql(
CREATE TABLE profiles (
    id INTEGER PRIMARY KEY,
    display_name TEXT NOT NULL COLLATE NOCASE UNIQUE,
    created_at INTEGER NOT NULL,
    updated_at INTEGER NOT NULL,
    last_used_at INTEGER NOT NULL
);

CREATE TABLE app_state (
    key TEXT PRIMARY KEY CHECK (key = 'active_profile_id'),
    profile_id INTEGER NOT NULL REFERENCES profiles(id) ON DELETE RESTRICT
);

PRAGMA user_version = 1;
)sql";

inline constexpr const char* migrateVersionOneToTwo = R"sql(
CREATE TABLE matches (
    id INTEGER PRIMARY KEY,
    profile_id INTEGER NOT NULL REFERENCES profiles(id) ON DELETE CASCADE,
    game_key TEXT NOT NULL CHECK (game_key IN ('classic_tic_tac_toe', 'ping_pong')),
    mode_key TEXT NOT NULL CHECK (mode_key IN ('human_vs_human', 'human_vs_computer')),
    opponent_name TEXT NOT NULL,
    profile_display_name TEXT NOT NULL,
    profile_side_or_mark TEXT,
    opponent_side_or_mark TEXT,
    result TEXT NOT NULL CHECK (result IN ('win', 'loss', 'draw')),
    profile_score INTEGER,
    opponent_score INTEGER,
    draw_value INTEGER,
    difficulty_key TEXT NOT NULL CHECK (difficulty_key IN ('none', 'easy', 'medium', 'hard')),
    match_format TEXT NOT NULL,
    duration_ms INTEGER NOT NULL CHECK (duration_ms >= 0),
    started_at INTEGER NOT NULL,
    completed_at INTEGER NOT NULL CHECK (completed_at >= started_at)
);

CREATE INDEX matches_profile_completed ON matches(profile_id, completed_at DESC);
CREATE INDEX matches_profile_game_completed ON matches(profile_id, game_key, completed_at DESC);
CREATE INDEX matches_profile_result_completed ON matches(profile_id, result, completed_at DESC);

PRAGMA user_version = 2;
)sql";

inline constexpr const char* migrateVersionTwoToThree = R"sql(
CREATE TABLE achievement_unlocks (
    profile_id INTEGER NOT NULL REFERENCES profiles(id) ON DELETE CASCADE,
    achievement_key TEXT NOT NULL,
    unlocked_at INTEGER NOT NULL,
    PRIMARY KEY (profile_id, achievement_key)
);

PRAGMA user_version = 3;
)sql";

// Version 4 adds app-wide settings and rebuilds `matches` so its CHECK constraints
// accept every graphical game (all 14 original board games plus Ping Pong) and the
// 'standard' difficulty used by games whose computer opponent has a single strategy.
// Every existing row is copied with its original id; indexes are recreated.
//
// The rebuild runs with foreign_keys=ON inside one transaction. That departs from SQLite's
// generic 12-step procedure but is safe here because no table references `matches` in v3:
// dropping it cascades nowhere, and the copy re-validates each row's profile reference.
// A later migration that adds a reference to `matches` must not reuse this pattern blindly.
inline constexpr const char* migrateVersionThreeToFour = R"sql(
CREATE TABLE app_settings (
    key TEXT PRIMARY KEY NOT NULL,
    value TEXT NOT NULL,
    updated_at INTEGER NOT NULL
);

CREATE TABLE matches_v4 (
    id INTEGER PRIMARY KEY,
    profile_id INTEGER NOT NULL REFERENCES profiles(id) ON DELETE CASCADE,
    game_key TEXT NOT NULL CHECK (game_key IN (
        'classic_tic_tac_toe', 'numerical_tic_tac_toe', 'sus', 'five_by_five_tic_tac_toe',
        'misere_tic_tac_toe', 'four_in_a_row', 'four_by_four_tic_tac_toe', 'word_tic_tac_toe',
        'pyramid_tic_tac_toe', 'diamond', 'infinity_xo', 'ultimate_xo', 'memory_xo',
        'obstacle_tic_tac_toe', 'ping_pong')),
    mode_key TEXT NOT NULL CHECK (mode_key IN ('human_vs_human', 'human_vs_computer')),
    opponent_name TEXT NOT NULL,
    profile_display_name TEXT NOT NULL,
    profile_side_or_mark TEXT,
    opponent_side_or_mark TEXT,
    result TEXT NOT NULL CHECK (result IN ('win', 'loss', 'draw')),
    profile_score INTEGER,
    opponent_score INTEGER,
    draw_value INTEGER,
    difficulty_key TEXT NOT NULL CHECK (difficulty_key IN ('none', 'easy', 'medium', 'hard', 'standard')),
    match_format TEXT NOT NULL,
    duration_ms INTEGER NOT NULL CHECK (duration_ms >= 0),
    started_at INTEGER NOT NULL,
    completed_at INTEGER NOT NULL CHECK (completed_at >= started_at)
);

INSERT INTO matches_v4 (
    id, profile_id, game_key, mode_key, opponent_name, profile_display_name, profile_side_or_mark,
    opponent_side_or_mark, result, profile_score, opponent_score, draw_value, difficulty_key,
    match_format, duration_ms, started_at, completed_at)
SELECT
    id, profile_id, game_key, mode_key, opponent_name, profile_display_name, profile_side_or_mark,
    opponent_side_or_mark, result, profile_score, opponent_score, draw_value, difficulty_key,
    match_format, duration_ms, started_at, completed_at
FROM matches;

DROP TABLE matches;
ALTER TABLE matches_v4 RENAME TO matches;

CREATE INDEX matches_profile_completed ON matches(profile_id, completed_at DESC);
CREATE INDEX matches_profile_game_completed ON matches(profile_id, game_key, completed_at DESC);
CREATE INDEX matches_profile_result_completed ON matches(profile_id, result, completed_at DESC);

PRAGMA user_version = 4;
)sql";

} // namespace persistence::schema
