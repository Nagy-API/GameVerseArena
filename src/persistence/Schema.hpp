#pragma once

namespace persistence::schema {

inline constexpr int currentVersion = 3;

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

} // namespace persistence::schema
