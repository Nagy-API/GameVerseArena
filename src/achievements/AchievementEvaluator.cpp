#include "AchievementEvaluator.hpp"

#include "AchievementCatalogue.hpp"

namespace achievements {
namespace {
AchievementEvaluation numeric(const AchievementDefinition& definition, std::int64_t value)
{
    return {definition.key, value >= definition.progressTarget,
            clampedProgress(value, definition.progressTarget)};
}

AchievementEvaluation condition(const AchievementDefinition& definition, bool satisfied)
{
    return {definition.key, satisfied, std::nullopt};
}
} // namespace

std::vector<AchievementEvaluation> AchievementEvaluator::evaluate(const AchievementSnapshot& snapshot)
{
    const auto& catalogue = AchievementCatalogue::all();
    std::vector<AchievementEvaluation> results;
    results.reserve(catalogue.size());
    results.push_back(numeric(catalogue[0], snapshot.totalWins));
    results.push_back(numeric(catalogue[1], snapshot.totalMatches));
    results.push_back(numeric(catalogue[2], snapshot.totalMatches));
    results.push_back(numeric(catalogue[3], snapshot.bestWinStreak));
    results.push_back(numeric(catalogue[4], snapshot.bestWinStreak));
    results.push_back(numeric(catalogue[5], (snapshot.classicWins > 0 ? 1 : 0) +
                                             (snapshot.pingPongWins > 0 ? 1 : 0)));
    results.push_back(numeric(catalogue[6], snapshot.classicWins));
    results.push_back(numeric(catalogue[7], snapshot.classicXWins));
    results.push_back(numeric(catalogue[8], snapshot.classicOWins));
    results.push_back(numeric(catalogue[9], snapshot.pingPongWins));
    results.push_back(condition(catalogue[10], snapshot.hasPingPongFiveZeroWin));
    results.push_back(condition(catalogue[11], snapshot.hasPingPongFiveFourWin));
    return results;
}

} // namespace achievements
