#include "PingPongSimulation.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace ping_pong {
namespace {
constexpr double pi = 3.14159265358979323846;
constexpr double maximumBounceAngle = 60.0 * pi / 180.0;

double movementValue(Movement movement)
{
    return static_cast<double>(static_cast<int>(movement));
}
} // namespace

PingPongSimulation::PingPongSimulation()
    : PingPongSimulation(std::random_device{}())
{
}

PingPongSimulation::PingPongSimulation(std::uint32_t seed)
    : rng_(seed)
{
    initializeGeometry();
    resetForServe();
}

void PingPongSimulation::initializeGeometry()
{
    state_.field = {};
    state_.leftPaddle.width = state_.rightPaddle.width = 18.0;
    state_.leftPaddle.height = state_.rightPaddle.height = 108.0;
    state_.leftPaddle.position.x = state_.field.left + 35.0;
    state_.rightPaddle.position.x = state_.field.right() - 35.0 - state_.rightPaddle.width;
    state_.ball.radius = 12.0;
}

void PingPongSimulation::resetForServe()
{
    const double paddleY = state_.field.top + (state_.field.height - state_.leftPaddle.height) / 2.0;
    state_.leftPaddle.position.y = paddleY;
    state_.rightPaddle.position.y = paddleY;
    state_.ball.position = {state_.field.left + state_.field.width / 2.0,
                            state_.field.top + state_.field.height / 2.0};

    std::uniform_int_distribution<int> horizontalDirection(0, 1);
    std::uniform_real_distribution<double> verticalComponent(-0.38, 0.38);
    const double vertical = verticalComponent(rng_);
    const double horizontal = std::sqrt(1.0 - vertical * vertical) *
                              (horizontalDirection(rng_) == 0 ? -1.0 : 1.0);
    state_.ballSpeed = baseBallSpeed;
    state_.ball.velocity = {horizontal * baseBallSpeed, vertical * baseBallSpeed};
    state_.rallyHits = 0;
    pointAwarded_ = false;
}

std::optional<Side> PingPongSimulation::step(double seconds, const ControlInput& input)
{
    lastEvents_ = {};
    if (seconds <= 0.0 || pointAwarded_) return std::nullopt;

    movePaddle(state_.leftPaddle, input.left, input.leftSpeedScale, seconds);
    movePaddle(state_.rightPaddle, input.right, input.rightSpeedScale, seconds);
    state_.ball.position.x += state_.ball.velocity.x * seconds;
    state_.ball.position.y += state_.ball.velocity.y * seconds;

    // A point ends this simulation step. In particular, do not let a ball that
    // has already crossed a goal line interact with a wall or paddle first.
    if (state_.ball.position.x + state_.ball.radius < state_.field.left) {
        pointAwarded_ = true;
        return Side::Right;
    }
    if (state_.ball.position.x - state_.ball.radius > state_.field.right()) {
        pointAwarded_ = true;
        return Side::Left;
    }

    lastEvents_.wallHit = resolveWalls();
    const bool leftHit = resolvePaddle(state_.leftPaddle, Side::Left);
    const bool rightHit = resolvePaddle(state_.rightPaddle, Side::Right);
    lastEvents_.paddleHit = leftHit || rightHit;
    return std::nullopt;
}

void PingPongSimulation::restoreState(const SimulationState& state)
{
    state_ = state;
    pointAwarded_ = false;
    lastEvents_ = {};
}

void PingPongSimulation::movePaddle(PaddleState& paddle, Movement movement, double speedScale, double seconds)
{
    const double safeScale = std::clamp(speedScale, 0.0, 1.0);
    paddle.position.y += movementValue(movement) * paddleSpeed * safeScale * seconds;
    paddle.position.y = std::clamp(paddle.position.y, state_.field.top,
                                   state_.field.bottom() - paddle.height);
}

bool PingPongSimulation::resolveWalls()
{
    const double top = state_.field.top + state_.ball.radius;
    const double bottom = state_.field.bottom() - state_.ball.radius;
    if (state_.ball.position.y < top) {
        state_.ball.position.y = top;
        if (state_.ball.velocity.y < 0.0) {
            state_.ball.velocity.y = -state_.ball.velocity.y;
            return true;
        }
    } else if (state_.ball.position.y > bottom) {
        state_.ball.position.y = bottom;
        if (state_.ball.velocity.y > 0.0) {
            state_.ball.velocity.y = -state_.ball.velocity.y;
            return true;
        }
    }
    return false;
}

bool PingPongSimulation::resolvePaddle(PaddleState& paddle, Side side)
{
    const bool travellingIntoPaddle = side == Side::Left ? state_.ball.velocity.x < 0.0
                                                          : state_.ball.velocity.x > 0.0;
    if (!travellingIntoPaddle) return false;

    const double paddleLeft = paddle.position.x;
    const double paddleRight = paddle.position.x + paddle.width;
    const double paddleTop = paddle.position.y;
    const double paddleBottom = paddle.position.y + paddle.height;
    const bool overlapsX = state_.ball.position.x + state_.ball.radius >= paddleLeft &&
                           state_.ball.position.x - state_.ball.radius <= paddleRight;
    const bool overlapsY = state_.ball.position.y + state_.ball.radius >= paddleTop &&
                           state_.ball.position.y - state_.ball.radius <= paddleBottom;
    if (!overlapsX || !overlapsY) return false;

    bounceFromPaddle(paddle, side);
    return true;
}

void PingPongSimulation::bounceFromPaddle(const PaddleState& paddle, Side side)
{
    const double paddleCenter = paddle.position.y + paddle.height / 2.0;
    const double impact = std::clamp((state_.ball.position.y - paddleCenter) / (paddle.height / 2.0), -1.0, 1.0);
    const double angle = impact * maximumBounceAngle;
    state_.ballSpeed = std::min(maximumBallSpeed, state_.ballSpeed * speedGrowth);
    const double direction = side == Side::Left ? 1.0 : -1.0;
    state_.ball.velocity.x = direction * std::cos(angle) * state_.ballSpeed;
    state_.ball.velocity.y = std::sin(angle) * state_.ballSpeed;
    state_.ball.position.x = side == Side::Left
        ? paddle.position.x + paddle.width + state_.ball.radius
        : paddle.position.x - state_.ball.radius;
    ++state_.rallyHits;
}

} // namespace ping_pong
