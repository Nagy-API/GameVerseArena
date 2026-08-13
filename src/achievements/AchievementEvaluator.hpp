#pragma once

#include "AchievementTypes.hpp"

#include <vector>

namespace achievements {

class AchievementEvaluator {
public:
    static std::vector<AchievementEvaluation> evaluate(const AchievementSnapshot& snapshot);
};

} // namespace achievements
