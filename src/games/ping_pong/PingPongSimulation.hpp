#pragma once

#include "PingPongTypes.hpp"

#include <cstdint>
#include <optional>
#include <random>

namespace ping_pong {

// Contacts that occurred during the most recent step(); used for presentation feedback
// such as sound, never for rules.
struct StepEvents {
    bool paddleHit{false};
    bool wallHit{false};
};

class PingPongSimulation {
public:
    static constexpr double paddleSpeed = 560.0;
    static constexpr double baseBallSpeed = 470.0;
    static constexpr double speedGrowth = 1.04;
    static constexpr double maximumBallSpeed = 900.0;

    PingPongSimulation();
    explicit PingPongSimulation(std::uint32_t seed);

    void resetForServe();
    std::optional<Side> step(double seconds, const ControlInput& input);

    const SimulationState& state() const noexcept { return state_; }
    void restoreState(const SimulationState& state);
    bool pointAwarded() const noexcept { return pointAwarded_; }
    const StepEvents& lastStepEvents() const noexcept { return lastEvents_; }

private:
    void initializeGeometry();
    void movePaddle(PaddleState& paddle, Movement movement, double speedScale, double seconds);
    bool resolveWalls();
    bool resolvePaddle(PaddleState& paddle, Side side);
    void bounceFromPaddle(const PaddleState& paddle, Side side);

    SimulationState state_{};
    std::mt19937 rng_;
    bool pointAwarded_{false};
    StepEvents lastEvents_{};
};

} // namespace ping_pong
