#include "PingPongAI.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace ping_pong {

PingPongAI::PingPongAI()
    : PingPongAI(std::random_device{}())
{
}

PingPongAI::PingPongAI(std::uint32_t seed)
    : rng_(seed)
{
}

AICommand PingPongAI::decide(double seconds, const SimulationState& state, AIDifficulty difficulty)
{
    elapsed_ += std::max(0.0, seconds);
    if (elapsed_ >= reactionInterval(difficulty)) {
        elapsed_ = 0.0;
        targetY_ = chooseTarget(state, difficulty);
    }

    const double center = state.rightPaddle.position.y + state.rightPaddle.height / 2.0;
    const double deadZone = difficulty == AIDifficulty::Easy ? 18.0 : difficulty == AIDifficulty::Medium ? 11.0 : 7.0;
    Movement movement = Movement::None;
    if (targetY_ < center - deadZone) movement = Movement::Up;
    else if (targetY_ > center + deadZone) movement = Movement::Down;
    return {movement, speedScale(difficulty), targetY_};
}

double PingPongAI::predictInterceptY(const SimulationState& state) const
{
    if (state.ball.velocity.x <= 0.0) return state.field.top + state.field.height / 2.0;
    const double paddleX = state.rightPaddle.position.x - state.ball.radius;
    const double travelTime = std::max(0.0, (paddleX - state.ball.position.x) / state.ball.velocity.x);
    const double minimum = state.field.top + state.ball.radius;
    const double maximum = state.field.bottom() - state.ball.radius;
    const double span = maximum - minimum;
    if (span <= 0.0) return minimum;
    double projected = state.ball.position.y + state.ball.velocity.y * travelTime - minimum;
    const double period = 2.0 * span;
    projected = std::fmod(projected, period);
    if (projected < 0.0) projected += period;
    if (projected > span) projected = period - projected;
    return minimum + projected;
}

void PingPongAI::reset() noexcept
{
    elapsed_ = 1000.0;
    targetY_ = 0.0;
}

double PingPongAI::chooseTarget(const SimulationState& state, AIDifficulty difficulty)
{
    const double center = state.field.top + state.field.height / 2.0;
    double target = center;
    if (difficulty == AIDifficulty::Easy) {
        target = state.ball.position.y;
    } else if (state.ball.velocity.x > 0.0) {
        target = predictInterceptY(state);
    } else {
        target = center;
    }
    std::uniform_real_distribution<double> error(-errorRange(difficulty), errorRange(difficulty));
    target += error(rng_);
    const double halfPaddle = state.rightPaddle.height / 2.0;
    return std::clamp(target, state.field.top + halfPaddle, state.field.bottom() - halfPaddle);
}

double PingPongAI::reactionInterval(AIDifficulty difficulty) const
{
    if (difficulty == AIDifficulty::Easy) return 0.19;
    if (difficulty == AIDifficulty::Medium) return 0.105;
    return 0.052;
}

double PingPongAI::speedScale(AIDifficulty difficulty) const
{
    if (difficulty == AIDifficulty::Easy) return 0.68;
    if (difficulty == AIDifficulty::Medium) return 0.84;
    return 1.0;
}

double PingPongAI::errorRange(AIDifficulty difficulty) const
{
    if (difficulty == AIDifficulty::Easy) return 72.0;
    if (difficulty == AIDifficulty::Medium) return 30.0;
    return 10.0;
}

} // namespace ping_pong
