#pragma once

#include "MatchTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace persistence {
class Database;
class Statement;

class MatchRepository {
public:
    explicit MatchRepository(Database& database) : database_(database) {}
    std::int64_t insert(const CompletedMatch& match);
    std::vector<CompletedMatch> recent(std::int64_t profileId, const MatchFilter& filter,
                                       std::size_t limit, std::size_t offset) const;
    std::int64_t count(std::int64_t profileId, const MatchFilter& filter) const;
private:
    static CompletedMatch read(Statement& statement);
    Database& database_;
};
} // namespace persistence
