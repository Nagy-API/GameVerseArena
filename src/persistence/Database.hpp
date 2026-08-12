#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace persistence {

class Statement {
public:
    Statement(sqlite3* database, const char* sql);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    void bind(int index, std::int64_t value);
    void bind(int index, const std::string& value);
    bool step();
    std::int64_t integer(int column) const;
    std::string text(int column) const;

private:
    sqlite3_stmt* statement_{};
};

class Database;

class Transaction {
public:
    explicit Transaction(Database& database);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    void commit();

private:
    Database* database_;
    bool committed_{false};
};

class Database {
public:
    explicit Database(std::filesystem::path path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    Statement prepare(const char* sql);
    void execute(const char* sql);
    Transaction transaction();
    int userVersion() const;
    bool foreignKeysEnabled() const;
    const std::filesystem::path& path() const noexcept { return path_; }
    std::int64_t lastInsertId() const;

private:
    friend class Transaction;
    void initializeSchema();

    std::filesystem::path path_;
    sqlite3* database_{};
};

} // namespace persistence
