#pragma once

#include "MatchRepository.hpp"

namespace persistence {
class Database;
class MatchService {
public:
    explicit MatchService(Database& database) : database_(database), repository_(database) {}
    std::int64_t recordCompleted(const CompletedMatch& match);
private:
    static void validate(const CompletedMatch& match);
    Database& database_;
    MatchRepository repository_;
};
} // namespace persistence
