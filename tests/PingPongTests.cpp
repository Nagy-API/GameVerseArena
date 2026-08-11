#include "PingPongAI.hpp"
#include "PingPongSession.hpp"
#include "PingPongSimulation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace ping_pong;

namespace {
constexpr double fixedStep = 1.0 / 120.0;
constexpr double pi = 3.14159265358979323846;
int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++failures;
    }
}

bool near(double first, double second, double tolerance = 1e-6)
{
    return std::abs(first - second) <= tolerance;
}

bool finite(Vec2 value)
{
    return std::isfinite(value.x) && std::isfinite(value.y);
}

double magnitude(Vec2 value)
{
    return std::hypot(value.x, value.y);
}

bool sameState(const SimulationState& first, const SimulationState& second, double tolerance = 1e-9)
{
    return near(first.leftPaddle.position.x, second.leftPaddle.position.x, tolerance) &&
           near(first.leftPaddle.position.y, second.leftPaddle.position.y, tolerance) &&
           near(first.rightPaddle.position.x, second.rightPaddle.position.x, tolerance) &&
           near(first.rightPaddle.position.y, second.rightPaddle.position.y, tolerance) &&
           near(first.ball.position.x, second.ball.position.x, tolerance) &&
           near(first.ball.position.y, second.ball.position.y, tolerance) &&
           near(first.ball.velocity.x, second.ball.velocity.x, tolerance) &&
           near(first.ball.velocity.y, second.ball.velocity.y, tolerance) &&
           near(first.ballSpeed, second.ballSpeed, tolerance) && first.rallyHits == second.rallyHits;
}

SimulationState collisionState(const PingPongSimulation& simulation, Side side, double impact,
                               double speed = PingPongSimulation::baseBallSpeed)
{
    auto state = simulation.state();
    const auto& paddle = side == Side::Left ? state.leftPaddle : state.rightPaddle;
    state.ball.position.y = paddle.position.y + paddle.height / 2.0 + impact * paddle.height / 2.0;
    state.ball.position.x = side == Side::Left
        ? paddle.position.x + paddle.width + state.ball.radius - 0.25
        : paddle.position.x - state.ball.radius + 0.25;
    state.ballSpeed = speed;
    state.ball.velocity = {side == Side::Left ? -speed : speed, 0.0};
    return state;
}

SimulationState approachingCollisionState(const PingPongSimulation& simulation, Side side, double impact,
                                           double speed)
{
    auto state = collisionState(simulation, side, impact, speed);
    const auto& paddle = side == Side::Left ? state.leftPaddle : state.rightPaddle;
    state.ball.position.x = side == Side::Left
        ? paddle.position.x + paddle.width + state.ball.radius + speed * fixedStep - 0.25
        : paddle.position.x - state.ball.radius - speed * fixedStep + 0.25;
    return state;
}

void initializationAndBoundsTests()
{
    PingPongSimulation simulation(42);
    const auto initial = simulation.state();
    expect(initial.leftPaddle.position.y >= initial.field.top, "left paddle starts in bounds");
    expect(initial.rightPaddle.position.y + initial.rightPaddle.height <= initial.field.bottom(),
           "right paddle starts in bounds");
    expect(near(initial.ball.position.x, initial.field.left + initial.field.width / 2.0),
           "ball starts centered horizontally");
    expect(near(initial.ball.position.y, initial.field.top + initial.field.height / 2.0),
           "ball starts centered vertically");
    expect(near(magnitude(initial.ball.velocity), PingPongSimulation::baseBallSpeed),
           "serve velocity matches base speed");

    ControlInput controls;
    controls.left = Movement::Up;
    controls.right = Movement::Down;
    controls.leftSpeedScale = 4.0;
    controls.rightSpeedScale = -2.0;
    simulation.step(10.0, controls);
    expect(near(simulation.state().leftPaddle.position.y, initial.field.top), "left paddle clamps at top");
    expect(near(simulation.state().rightPaddle.position.y, initial.rightPaddle.position.y),
           "negative speed scale clamps to no movement");

    controls = {};
    controls.right = Movement::Down;
    simulation.restoreState(initial);
    simulation.step(10.0, controls);
    expect(near(simulation.state().rightPaddle.position.y + simulation.state().rightPaddle.height,
                initial.field.bottom()), "right paddle clamps at bottom");
}

void wallCollisionTests()
{
    PingPongSimulation simulation(12);
    auto state = simulation.state();
    state.ball.position.y = state.field.top + state.ball.radius +
                            PingPongSimulation::maximumBallSpeed * fixedStep - 0.25;
    state.ball.velocity = {0.0, -PingPongSimulation::maximumBallSpeed};
    state.ballSpeed = PingPongSimulation::maximumBallSpeed;
    simulation.restoreState(state);
    simulation.step(fixedStep, {});
    expect(simulation.state().ball.velocity.y > 0.0, "near-max-speed top wall contact is not skipped");
    expect(near(simulation.state().ball.position.y, state.field.top + state.ball.radius),
           "top wall separates the ball into the field");

    state.ball.position.y = state.field.bottom() - state.ball.radius -
                            PingPongSimulation::maximumBallSpeed * fixedStep + 0.25;
    state.ball.velocity = {0.0, PingPongSimulation::maximumBallSpeed};
    simulation.restoreState(state);
    simulation.step(fixedStep, {});
    expect(simulation.state().ball.velocity.y < 0.0, "near-max-speed bottom wall contact is not skipped");
    expect(near(simulation.state().ball.position.y, state.field.bottom() - state.ball.radius),
           "bottom wall separates the ball into the field");
}

void paddleCollisionTests()
{
    PingPongSimulation simulation(19);
    for (const Side side : {Side::Left, Side::Right}) {
        simulation.restoreState(approachingCollisionState(
            simulation, side, 0.0, PingPongSimulation::maximumBallSpeed));
        simulation.step(fixedStep, {});
        const auto& state = simulation.state();
        const auto& paddle = side == Side::Left ? state.leftPaddle : state.rightPaddle;
        expect(side == Side::Left ? state.ball.velocity.x > 0.0 : state.ball.velocity.x < 0.0,
               side == Side::Left ? "near-max-speed left paddle contact is not skipped"
                                  : "near-max-speed right paddle contact is not skipped");
        expect(side == Side::Left
                   ? near(state.ball.position.x, paddle.position.x + paddle.width + state.ball.radius)
                   : near(state.ball.position.x, paddle.position.x - state.ball.radius),
               "paddle collision separates the ball from the contact surface");
        expect(near(state.ballSpeed, PingPongSimulation::maximumBallSpeed) &&
                   near(magnitude(state.ball.velocity), PingPongSimulation::maximumBallSpeed),
               "maximum-speed paddle contact remains capped");
        const unsigned int hitsAfterContact = state.rallyHits;
        simulation.step(fixedStep, {});
        expect(simulation.state().rallyHits == hitsAfterContact,
               "one paddle contact cannot grow speed again on the following step");
    }

    constexpr double incomingSpeed = 700.0;
    const double expectedSpeed = incomingSpeed * PingPongSimulation::speedGrowth;
    for (const Side side : {Side::Left, Side::Right}) {
        for (const double impact : {-0.8, 0.0, 0.8}) {
            simulation.restoreState(collisionState(simulation, side, impact, incomingSpeed));
            simulation.step(fixedStep, {});
            const auto& state = simulation.state();
            expect(side == Side::Left ? state.ball.velocity.x > 0.0 : state.ball.velocity.x < 0.0,
                   "paddle impact has the correct outgoing horizontal direction");
            if (impact < 0.0) expect(state.ball.velocity.y < 0.0, "upper paddle impact travels upward");
            if (impact > 0.0) expect(state.ball.velocity.y > 0.0, "lower paddle impact travels downward");
            if (near(impact, 0.0)) expect(std::abs(state.ball.velocity.y) < 1e-6, "center impact is horizontal");
            const double angle = std::atan2(std::abs(state.ball.velocity.y), std::abs(state.ball.velocity.x));
            expect(angle <= pi / 3.0 + 1e-9, "paddle bounce angle stays within sixty degrees");
            expect(near(state.ballSpeed, expectedSpeed) && near(magnitude(state.ball.velocity), expectedSpeed),
                   "paddle impact applies exactly one rally-speed increase");
        }
    }
}

void scoringTests()
{
    PingPongSimulation simulation(27);
    auto state = simulation.state();
    state.ball.position.x = state.field.left + state.field.width / 2.0;
    state.ball.velocity = {-2500.0, 0.0};
    state.ballSpeed = 2500.0;
    simulation.restoreState(state);
    expect(simulation.step(1.0, {}) == Side::Right, "far overshoot of left goal still scores once");
    expect(!simulation.step(fixedStep, {}).has_value(), "left goal cannot score twice before reset");

    simulation.resetForServe();
    state = simulation.state();
    state.ball.position.x = state.field.left + state.field.width / 2.0;
    state.ball.velocity = {2500.0, 0.0};
    state.ballSpeed = 2500.0;
    simulation.restoreState(state);
    expect(simulation.step(1.0, {}) == Side::Left, "far overshoot of right goal still scores once");

    simulation.resetForServe();
    state = simulation.state();
    state.leftPaddle.position.x = state.field.left - 100.0;
    state.leftPaddle.width = 180.0;
    state.leftPaddle.position.y = state.field.top;
    state.leftPaddle.height = state.field.height;
    state.ball.position = {state.field.left - state.ball.radius - 45.0,
                           state.field.top + state.field.height / 2.0};
    state.ball.velocity = {-100.0, 0.0};
    state.ballSpeed = 100.0;
    simulation.restoreState(state);
    expect(simulation.step(fixedStep, {}) == Side::Right, "crossed goal is resolved before any collision");
    expect(simulation.state().rallyHits == state.rallyHits,
           "no paddle contact is processed after the ball has scored");
}

void longRunStabilityTests()
{
    PingPongSimulation simulation(314159);
    auto state = simulation.state();
    state.leftPaddle.position.y = state.field.top;
    state.rightPaddle.position.y = state.field.top;
    state.leftPaddle.height = state.field.height;
    state.rightPaddle.height = state.field.height;
    state.ball.position = {state.field.left + state.field.width / 2.0,
                           state.field.top + state.field.height / 2.0};
    state.ball.velocity = {420.0, 210.0};
    state.ballSpeed = magnitude(state.ball.velocity);
    state.rallyHits = 0;
    simulation.restoreState(state);

    bool allFinite = true;
    bool paddlesInBounds = true;
    bool ballInVerticalBounds = true;
    bool legalSpeed = true;
    bool noScore = true;
    constexpr int stressSteps = 120000;
    for (int step = 0; step < stressSteps; ++step) {
        noScore = noScore && !simulation.step(fixedStep, {}).has_value();
        const auto& current = simulation.state();
        allFinite = allFinite && finite(current.leftPaddle.position) && finite(current.rightPaddle.position) &&
                    finite(current.ball.position) && finite(current.ball.velocity) && std::isfinite(current.ballSpeed);
        paddlesInBounds = paddlesInBounds && current.leftPaddle.position.y >= current.field.top - 1e-9 &&
                          current.rightPaddle.position.y >= current.field.top - 1e-9 &&
                          current.leftPaddle.position.y + current.leftPaddle.height <= current.field.bottom() + 1e-9 &&
                          current.rightPaddle.position.y + current.rightPaddle.height <= current.field.bottom() + 1e-9;
        ballInVerticalBounds = ballInVerticalBounds &&
                               current.ball.position.y >= current.field.top + current.ball.radius - 1e-9 &&
                               current.ball.position.y <= current.field.bottom() - current.ball.radius + 1e-9;
        const double actualSpeed = magnitude(current.ball.velocity);
        legalSpeed = legalSpeed && actualSpeed > 0.0 &&
                     actualSpeed <= PingPongSimulation::maximumBallSpeed + 1e-8 &&
                     current.ballSpeed <= PingPongSimulation::maximumBallSpeed + 1e-8 &&
                     near(actualSpeed, current.ballSpeed, 1e-8);
    }
    expect(allFinite, "120,000-step rally keeps every position, velocity, and speed finite");
    expect(paddlesInBounds, "120,000-step rally keeps paddles within field bounds");
    expect(ballInVerticalBounds, "120,000-step rally keeps the ball within wall bounds");
    expect(legalSpeed, "120,000-step rally keeps active speed positive and capped");
    expect(noScore && simulation.state().rallyHits > 100,
           "120,000-step full-height-paddle rally remains active through many contacts");
}

void deterministicReplayTests()
{
    PingPongSimulation first(77);
    PingPongSimulation second(77);
    bool sameScores = true;
    for (int index = 0; index < 20000; ++index) {
        ControlInput input;
        input.left = index % 240 < 80 ? Movement::Up : index % 240 < 160 ? Movement::Down : Movement::None;
        input.right = index % 180 < 60 ? Movement::Down : index % 180 < 120 ? Movement::Up : Movement::None;
        input.rightSpeedScale = 0.84;
        const auto firstScore = first.step(fixedStep, input);
        const auto secondScore = second.step(fixedStep, input);
        sameScores = sameScores && firstScore == secondScore;
        if (firstScore) {
            first.resetForServe();
            second.resetForServe();
        }
    }
    expect(sameScores && sameState(first.state(), second.state()),
           "same seed and fixed-step input stream reproduce the complete final simulation state");
}

void sessionTests()
{
    PingPongSession session;
    SessionConfig config{"  Alice  ", "  Bob  ", GameMode::HumanVsHuman, AIDifficulty::Hard};
    session.startMatch(config);
    expect(session.score().left == 0 && session.score().right == 0, "initial score is 0-0");
    expect(session.playerName(Side::Left) == "Alice" && session.playerName(Side::Right) == "Bob",
           "names are trimmed");
    expect(session.consumePointResetRequest() && !session.consumePointResetRequest(),
           "initial simulation reset request is one-shot");

    session.update(PingPongSession::serveCountdownSeconds / 2.0);
    const double frozenCountdown = session.stateSecondsRemaining();
    session.update(0.0);
    session.update(-1.0);
    expect(session.state() == MatchState::ServeCountdown && near(session.stateSecondsRemaining(), frozenCountdown),
           "non-positive updates leave countdown state unchanged");
    session.update(frozenCountdown);
    expect(session.state() == MatchState::Playing, "serve countdown enters play once");
    expect(session.awardPoint(Side::Left), "point is accepted during play");
    expect(session.score().left == 1, "score increments correctly");
    expect(!session.awardPoint(Side::Left), "point cannot increment twice during point pause");
    session.update(PingPongSession::pointPauseSeconds);
    expect(session.state() == MatchState::ServeCountdown && session.consumePointResetRequest() &&
               !session.consumePointResetRequest(),
           "point flow requests exactly one reset before exactly one countdown");

    session.update(PingPongSession::serveCountdownSeconds);
    for (unsigned int point = 1; point < PingPongSession::winningScore; ++point) {
        expect(session.awardPoint(Side::Left), "winning run accepts one point during play");
        if (point + 1 < PingPongSession::winningScore) {
            session.update(PingPongSession::pointPauseSeconds);
            expect(session.consumePointResetRequest(), "next rally requests a simulation reset");
            session.update(PingPongSession::serveCountdownSeconds);
        }
    }
    const MatchScore finalScore = session.score();
    const auto finalWinner = session.winner();
    expect(session.matchFinished() && finalWinner == Side::Left && finalScore.left == PingPongSession::winningScore,
           "winning point finishes exactly at five");
    session.update(100.0);
    expect(!session.awardPoint(Side::Right) && session.score().left == finalScore.left &&
               session.score().right == finalScore.right && session.winner() == finalWinner,
           "score and winner cannot change after match completion");

    session.rematch();
    expect(session.score().left == 0 && session.score().right == 0 && !session.lastScorer() && !session.winner() &&
               session.state() == MatchState::ServeCountdown && session.consumePointResetRequest(),
           "rematch clears score, point metadata, timers, and requests fresh physics");
    expect(session.config().mode == GameMode::HumanVsHuman && session.config().difficulty == AIDifficulty::Hard &&
               session.playerName(Side::Left) == "Alice" && session.playerName(Side::Right) == "Bob",
           "rematch preserves configuration");

    config = {"", "ignored", GameMode::HumanVsComputer, AIDifficulty::Easy};
    session.startMatch(config);
    expect(session.playerName(Side::Left) == "Player 1" && session.playerName(Side::Right) == "Computer",
           "empty and computer names use defaults");
}

double independentlyReflected(double position, double velocity, double seconds, double minimum, double maximum)
{
    double result = position + velocity * seconds;
    while (result < minimum || result > maximum) {
        if (result < minimum) result = minimum + (minimum - result);
        if (result > maximum) result = maximum - (result - maximum);
    }
    return result;
}

void aiTests()
{
    PingPongSimulation simulation(11);
    const auto initial = simulation.state();
    std::array<double, 3> speedScales{};
    std::size_t difficultyIndex = 0;
    for (const auto difficulty : {AIDifficulty::Easy, AIDifficulty::Medium, AIDifficulty::Hard}) {
        PingPongAI ai(99);
        auto away = initial;
        away.ball.velocity.x = -std::abs(away.ball.velocity.x);
        const auto command = ai.decide(1.0, away, difficulty);
        speedScales[difficultyIndex++] = command.speedScale;
        expect(command.movement == Movement::Up || command.movement == Movement::None || command.movement == Movement::Down,
               "AI returns a legal movement while the ball travels away");
        expect(command.targetY >= initial.field.top + initial.rightPaddle.height / 2.0 &&
                   command.targetY <= initial.field.bottom() - initial.rightPaddle.height / 2.0,
               "randomized AI target stays inside playable paddle-center bounds");
        expect(near(initial.rightPaddle.position.y, simulation.state().rightPaddle.position.y),
               "AI decision never teleports the paddle");

        auto controlled = initial;
        controlled.rightPaddle.position.y = controlled.field.top + 100.0;
        simulation.restoreState(controlled);
        ControlInput input;
        input.right = command.movement;
        input.rightSpeedScale = command.speedScale;
        const double startingY = simulation.state().rightPaddle.position.y;
        simulation.step(fixedStep, input);
        expect(std::abs(simulation.state().rightPaddle.position.y - startingY) <=
                   PingPongSimulation::paddleSpeed * command.speedScale * fixedStep + 1e-6,
               "AI movement remains constrained by normal paddle physics");
        simulation.restoreState(initial);
    }
    expect(speedScales[0] < speedScales[1] && speedScales[1] < speedScales[2] && near(speedScales[2], 1.0),
           "configured AI speeds preserve Easy < Medium < Hard ordering");

    auto reflection = initial;
    reflection.ball.position = {300.0, reflection.field.top + 40.0};
    reflection.ball.velocity = {100.0, 850.0};
    reflection.ballSpeed = magnitude(reflection.ball.velocity);
    const double paddleX = reflection.rightPaddle.position.x - reflection.ball.radius;
    const double travelTime = (paddleX - reflection.ball.position.x) / reflection.ball.velocity.x;
    const double minimum = reflection.field.top + reflection.ball.radius;
    const double maximum = reflection.field.bottom() - reflection.ball.radius;
    const double expectedIntercept = independentlyReflected(
        reflection.ball.position.y, reflection.ball.velocity.y, travelTime, minimum, maximum);
    PingPongAI predictor(7);
    const double predicted = predictor.predictInterceptY(reflection);
    expect(near(predicted, expectedIntercept), "intercept prediction handles multiple wall reflections");
    expect(predicted >= minimum && predicted <= maximum, "predicted intercept remains inside ball travel bounds");

    for (const auto difficulty : {AIDifficulty::Easy, AIDifficulty::Medium, AIDifficulty::Hard}) {
        PingPongAI bounded(123);
        bool allTargetsBounded = true;
        for (int decision = 0; decision < 1000; ++decision) {
            const auto command = bounded.decide(1.0, reflection, difficulty);
            allTargetsBounded = allTargetsBounded &&
                command.targetY >= reflection.field.top + reflection.rightPaddle.height / 2.0 &&
                command.targetY <= reflection.field.bottom() - reflection.rightPaddle.height / 2.0;
        }
        expect(allTargetsBounded, "repeated randomized aim never leaves playable target bounds");
    }

    const std::array<double, 3> intervals{0.19, 0.105, 0.052};
    difficultyIndex = 0;
    for (const auto difficulty : {AIDifficulty::Easy, AIDifficulty::Medium, AIDifficulty::Hard}) {
        PingPongAI cadence(456 + static_cast<unsigned int>(difficultyIndex));
        const double initialTarget = cadence.decide(0.0, reflection, difficulty).targetY;
        const int stepsUntilReaction = static_cast<int>(std::ceil(intervals[difficultyIndex] / fixedStep));
        bool stableBeforeReaction = true;
        for (int step = 1; step < stepsUntilReaction; ++step) {
            stableBeforeReaction = stableBeforeReaction &&
                near(cadence.decide(fixedStep, reflection, difficulty).targetY, initialTarget, 0.0);
        }
        const double reactedTarget = cadence.decide(fixedStep, reflection, difficulty).targetY;
        expect(stableBeforeReaction && !near(reactedTarget, initialTarget, 0.0),
               "AI reaction target changes only when its fixed-step interval elapses");
        ++difficultyIndex;
    }

    PingPongAI first(1234);
    PingPongAI second(1234);
    bool deterministic = true;
    for (int decision = 0; decision < 500; ++decision) {
        const auto firstCommand = first.decide(fixedStep, reflection, AIDifficulty::Easy);
        const auto secondCommand = second.decide(fixedStep, reflection, AIDifficulty::Easy);
        deterministic = deterministic && firstCommand.movement == secondCommand.movement &&
                        near(firstCommand.speedScale, secondCommand.speedScale, 0.0) &&
                        near(firstCommand.targetY, secondCommand.targetY, 0.0);
    }
    expect(deterministic, "deterministic seed reproduces the complete randomized aim sequence");
}
} // namespace

int main()
{
    initializationAndBoundsTests();
    wallCollisionTests();
    paddleCollisionTests();
    scoringTests();
    longRunStabilityTests();
    deterministicReplayTests();
    sessionTests();
    aiTests();
    if (failures != 0) {
        std::cerr << failures << " Ping Pong test(s) failed.\n";
        return EXIT_FAILURE;
    }
    std::cout << "All Ping Pong tests passed, including 120,000-step stability stress.\n";
    return EXIT_SUCCESS;
}
