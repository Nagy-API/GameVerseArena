#include "Database.hpp"

#include "Schema.hpp"
#include "sqlite3.h"

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace persistence {
namespace {
std::runtime_error sqliteError(sqlite3* database, const std::string& context)
{
    return std::runtime_error(context + ": " + (database != nullptr ? sqlite3_errmsg(database) : "SQLite error"));
}
} // namespace

Statement::Statement(sqlite3* database, const char* sql)
{
    if (sqlite3_prepare_v2(database, sql, -1, &statement_, nullptr) != SQLITE_OK) {
        throw sqliteError(database, "Could not prepare database statement");
    }
}

Statement::~Statement()
{
    if (statement_ != nullptr) {
        sqlite3_finalize(statement_);
    }
}

Statement::Statement(Statement&& other) noexcept : statement_(other.statement_)
{
    other.statement_ = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept
{
    if (this != &other) {
        if (statement_ != nullptr) sqlite3_finalize(statement_);
        statement_ = other.statement_;
        other.statement_ = nullptr;
    }
    return *this;
}

void Statement::bind(int index, std::int64_t value)
{
    if (sqlite3_bind_int64(statement_, index, value) != SQLITE_OK) {
        throw sqliteError(sqlite3_db_handle(statement_), "Could not bind integer value");
    }
}

void Statement::bind(int index, const std::string& value)
{
    if (sqlite3_bind_text(statement_, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        throw sqliteError(sqlite3_db_handle(statement_), "Could not bind text value");
    }
}

void Statement::bindNull(int index)
{
    if (sqlite3_bind_null(statement_, index) != SQLITE_OK) {
        throw sqliteError(sqlite3_db_handle(statement_), "Could not bind null value");
    }
}

bool Statement::step()
{
    const int result = sqlite3_step(statement_);
    if (result == SQLITE_ROW) return true;
    if (result == SQLITE_DONE) return false;
    throw sqliteError(sqlite3_db_handle(statement_), "Could not execute database statement");
}

std::int64_t Statement::integer(int column) const
{
    return sqlite3_column_int64(statement_, column);
}

std::string Statement::text(int column) const
{
    const auto* value = sqlite3_column_text(statement_, column);
    if (value == nullptr) return {};
    // Use the stored byte length so a value with an embedded NUL is not silently truncated.
    const int bytes = sqlite3_column_bytes(statement_, column);
    return std::string(reinterpret_cast<const char*>(value), static_cast<std::size_t>(bytes));
}

bool Statement::isNull(int column) const
{
    return sqlite3_column_type(statement_, column) == SQLITE_NULL;
}

Transaction::Transaction(Database& database) : database_(&database)
{
    database_->execute("BEGIN IMMEDIATE;");
}

Transaction::~Transaction()
{
    if (!committed_ && database_ != nullptr) {
        try {
            database_->execute("ROLLBACK;");
        } catch (...) {
        }
    }
}

void Transaction::commit()
{
    database_->execute("COMMIT;");
    committed_ = true;
}

Database::Database(std::filesystem::path path) : path_(std::move(path))
{
    if (path_.empty()) {
        throw std::invalid_argument("Database path must not be empty");
    }
    std::error_code directoryError;
    if (path_.has_parent_path()) {
        std::filesystem::create_directories(path_.parent_path(), directoryError);
    }
    if (directoryError) {
        throw std::runtime_error("Could not create database directory '" + path_.parent_path().string() +
                                 "': " + directoryError.message());
    }

    const auto utf8Path = path_.u8string();
    const int result = sqlite3_open_v2(utf8Path.c_str(), &database_,
                                       SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (result != SQLITE_OK) {
        const auto error = sqliteError(database_, "Could not open profile database '" + path_.string() + "'");
        if (database_ != nullptr) sqlite3_close(database_);
        database_ = nullptr;
        throw error;
    }

    try {
        sqlite3_busy_timeout(database_, 3000);
        execute("PRAGMA foreign_keys = ON;");
        if (!foreignKeysEnabled()) {
            throw std::runtime_error("Could not enable SQLite foreign-key enforcement");
        }
        initializeSchema();
    } catch (...) {
        sqlite3_close(database_);
        database_ = nullptr;
        throw;
    }
}

Database::~Database()
{
    if (database_ != nullptr) {
        sqlite3_close(database_);
    }
}

Statement Database::prepare(const char* sql)
{
    return Statement(database_, sql);
}

void Database::execute(const char* sql)
{
    char* message = nullptr;
    const int result = sqlite3_exec(database_, sql, nullptr, nullptr, &message);
    if (result != SQLITE_OK) {
        const std::string detail = message != nullptr ? message : sqlite3_errmsg(database_);
        sqlite3_free(message);
        throw std::runtime_error("Could not execute database command: " + detail);
    }
}

Transaction Database::transaction()
{
    return Transaction(*this);
}

int Database::userVersion() const
{
    Statement statement(database_, "PRAGMA user_version;");
    return statement.step() ? static_cast<int>(statement.integer(0)) : 0;
}

bool Database::foreignKeysEnabled() const
{
    Statement statement(database_, "PRAGMA foreign_keys;");
    return statement.step() && statement.integer(0) == 1;
}

std::int64_t Database::lastInsertId() const
{
    return sqlite3_last_insert_rowid(database_);
}

void Database::requireNoForeignKeyViolations(const char* table, const std::string& context)
{
    // Scoped to the table a migration rebuilt, so an unrelated pre-existing inconsistency is
    // not misreported as a failure of this migration.
    Statement check(database_, (std::string("PRAGMA foreign_key_check(") + table + ");").c_str());
    if (check.step()) {
        throw std::runtime_error("Foreign-key violation detected in table '" + std::string(table) + "' after " +
                                 context);
    }
}

namespace {
std::runtime_error unsupportedVersion(int version)
{
    return std::runtime_error("Unsupported profile database schema version " + std::to_string(version) +
                              "; this build supports version " + std::to_string(schema::currentVersion));
}
} // namespace

void Database::initializeSchema()
{
    // Fast path without a write lock: an up-to-date or future database is never modified.
    const int initial = userVersion();
    if (initial == schema::currentVersion) return;
    if (initial > schema::currentVersion || initial < 0) throw unsupportedVersion(initial);

    // Each step re-reads the version inside its own BEGIN IMMEDIATE transaction, so two
    // processes upgrading the same file concurrently apply every step exactly once.
    while (true) {
        auto change = transaction();
        const int version = userVersion();
        if (version == schema::currentVersion) {
            change.commit();
            return;
        }
        if (version > schema::currentVersion || version < 0) throw unsupportedVersion(version);

        switch (version) {
        case 0: {
            // Never adopt an unrelated SQLite file: a new profile database must be empty.
            std::int64_t existingObjects = 0;
            {
                Statement objects(database_, "SELECT COUNT(*) FROM sqlite_master;");
                if (objects.step()) existingObjects = objects.integer(0);
            }
            if (existingObjects != 0) {
                throw std::runtime_error("The file is not a GameVerseArena profile database (it already contains "
                                         "other tables) and was left unchanged");
            }
            execute(schema::createVersionOne);
            break;
        }
        case 1:
            execute(schema::migrateVersionOneToTwo);
            break;
        case 2:
            execute(schema::migrateVersionTwoToThree);
            break;
        case 3:
            execute(schema::migrateVersionThreeToFour);
            requireNoForeignKeyViolations("matches", "the v3-to-v4 migration");
            break;
        default:
            throw unsupportedVersion(version);
        }
        // Every step must advance the version exactly once, or a mistaken step could repeat
        // forever while holding the write lock.
        if (userVersion() != version + 1) {
            throw std::runtime_error("Schema migration from version " + std::to_string(version) +
                                     " did not advance the version");
        }
        change.commit();
    }
}

} // namespace persistence
