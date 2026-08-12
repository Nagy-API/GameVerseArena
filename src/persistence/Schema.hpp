#pragma once

namespace persistence::schema {

inline constexpr int currentVersion = 1;

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

} // namespace persistence::schema
