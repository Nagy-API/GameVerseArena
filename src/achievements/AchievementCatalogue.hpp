#pragma once

#include "AchievementTypes.hpp"

#include <vector>

namespace achievements {

class AchievementCatalogue {
public:
    static const std::vector<AchievementDefinition>& all();
};

} // namespace achievements
