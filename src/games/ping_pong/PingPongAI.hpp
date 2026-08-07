#pragma once

#include "PingPongTypes.hpp"

#include <cstdint>
#include <random>

namespace ping_pong {

struct AICommand {
    Movement movement{Movement::None};
    double speedScale{1.0};
    double targetY{};
};

class PingPongAI {
public:
    PingPongAI();
    explicit PingPongAI(std::uint32_t seed);

    AICommand decide(double seconds, const SimulationState& state, AIDifficulty difficulty);
    double predictInterceptY(const SimulationState& state) const;
    void reset() noexcept;

private:
    double chooseTarget(const SimulationState& state, AIDifficulty difficulty);
    double reactionInterval(AIDifficulty difficulty) const;
    double speedScale(AIDifficulty difficulty) const;
    double errorRange(AIDifficulty difficulty) const;

    std::mt19937 rng_;
    double elapsed_{1000.0};
    double targetY_{};
};

} // namespace ping_pong
