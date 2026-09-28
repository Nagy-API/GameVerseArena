#pragma once

#include "Database.hpp"
#include "Schema.hpp"

#include <stdexcept>

namespace test_support {

// Converts a database created at the current schema into the exact schema of an older
// version, keeping every row that the older schema can represent. It replays the
// historical migration SQL from Schema.hpp, so migration tests start from the real
// tables and CHECK constraints that earlier releases created.
inline void downgradeToVersion(persistence::Database& database, int version)
{
    if (version < 1 || version > persistence::schema::currentVersion) {
        throw std::invalid_argument("Unsupported downgrade target");
    }
    int current = database.userVersion();
    if (current >= 4 && version <= 3) {
        database.execute("DROP TABLE IF EXISTS app_settings;");
        database.execute("CREATE TEMP TABLE matches_backup AS SELECT * FROM matches;");
        database.execute("DROP TABLE matches;");
        // Recreates the version 2 `matches` table (and its indexes) and sets user_version = 2.
        database.execute(persistence::schema::migrateVersionOneToTwo);
        database.execute("INSERT INTO matches SELECT * FROM temp.matches_backup;");
        database.execute("DROP TABLE temp.matches_backup;");
        database.execute("PRAGMA user_version = 3;");
        current = 3;
    }
    if (current >= 3 && version <= 2) {
        database.execute("DROP TABLE IF EXISTS achievement_unlocks;");
        database.execute("PRAGMA user_version = 2;");
        current = 2;
    }
    if (current >= 2 && version <= 1) {
        database.execute("DROP TABLE IF EXISTS matches;");
        database.execute("PRAGMA user_version = 1;");
    }
}

} // namespace test_support
