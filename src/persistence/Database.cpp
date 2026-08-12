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
    return value != nullptr ? reinterpret_cast<const char*>(value) : std::string{};
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

void Database::initializeSchema()
{
    const int version = userVersion();
    if (version == schema::currentVersion) return;
    if (version != 0) {
        throw std::runtime_error("Unsupported profile database schema version " + std::to_string(version) +
                                 "; this build supports version " + std::to_string(schema::currentVersion));
    }

    auto change = transaction();
    execute(schema::createVersionOne);
    change.commit();
}

} // namespace persistence
