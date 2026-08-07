#include "PingPongAI.hpp"
#include "PingPongSession.hpp"
#include "PingPongSimulation.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace ping_pong;

namespace {
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

SimulationState collisionState(const PingPongSimulation& simulation, Side side, double impact)
{
    auto state = simulation.state();
    const auto& paddle = side == Side::Left ? state.leftPaddle : state.rightPaddle;
    state.ball.position.y = paddle.position.y + paddle.height / 2.0 + impact * paddle.height / 2.0;
    state.ball.position.x = side == Side::Left
        ? paddle.position.x + paddle.width + state.ball.radius - 1.0
        : paddle.position.x - state.ball.radius + 1.0;
    state.ball.velocity = {side == Side::Left ? -state.ballSpeed : state.ballSpeed, 0.0};
    return state;
}

void simulationTests()
{
    PingPongSimulation simulation(42);
    const auto initial = simulation.state();
    expect(initial.leftPaddle.position.y >= initial.field.top, "left paddle starts in bounds");
    expect(initial.rightPaddle.position.y + initial.rightPaddle.height <= initial.field.bottom(), "right paddle starts in bounds");
    expect(near(initial.ball.position.x, initial.field.left + initial.field.width / 2.0), "ball starts centered horizontally");
    expect(near(initial.ball.position.y, initial.field.top + initial.field.height / 2.0), "ball starts centered vertically");
    expect(std::abs(initial.ball.velocity.x) > 1.0, "serve has horizontal motion");

    ControlInput controls;
    controls.left = Movement::Up;
    controls.right = Movement::Down;
    simulation.step(10.0, controls);
    expect(near(simulation.state().leftPaddle.position.y, initial.field.top), "left paddle clamps at top");
    expect(near(simulation.state().rightPaddle.position.y + simulation.state().rightPaddle.height, initial.field.bottom()), "right paddle clamps at bottom");

    auto state = initial;
    state.ball.position.y = state.field.top + state.ball.radius - 2.0;
    state.ball.velocity = {200.0, -300.0};
    state.ballSpeed = std::hypot(state.ball.velocity.x, state.ball.velocity.y);
    simulation.restoreState(state);
    simulation.step(1.0 / 120.0, {});
    expect(simulation.state().ball.velocity.y > 0.0, "top wall reflects downward");
    expect(simulation.state().ball.position.y >= state.field.top + state.ball.radius, "top collision corrects position");

    state.ball.position.y = state.field.bottom() - state.ball.radius + 2.0;
    state.ball.velocity = {200.0, 300.0};
    simulation.restoreState(state);
    simulation.step(1.0 / 120.0, {});
    expect(simulation.state().ball.velocity.y < 0.0, "bottom wall reflects upward");
    expect(simulation.state().ball.position.y <= state.field.bottom() - state.ball.radius, "bottom collision corrects position");

    simulation.restoreState(collisionState(simulation, Side::Left, 0.0));
    simulation.step(1.0 / 120.0, {});
    expect(simulation.state().ball.velocity.x > 0.0, "left paddle reflects to the right");
    expect(std::abs(simulation.state().ball.velocity.y) < 1.0, "center hit stays shallow");
    const double centerVertical = simulation.state().ball.velocity.y;

    simulation.restoreState(collisionState(simulation, Side::Left, 0.8));
    simulation.step(1.0 / 120.0, {});
    expect(simulation.state().ball.velocity.x > 0.0, "left edge hit reflects horizontally");
    expect(std::abs(simulation.state().ball.velocity.y) > std::abs(centerVertical) + 100.0, "impact offset changes vertical component");

    simulation.restoreState(collisionState(simulation, Side::Right, -0.3));
    simulation.step(1.0 / 120.0, {});
    expect(simulation.state().ball.velocity.x < 0.0, "right paddle reflects to the left");
    auto away = simulation.state();
    const unsigned int hits = away.rallyHits;
    away.ball.position.x = away.rightPaddle.position.x - away.ball.radius + 1.0;
    away.ball.velocity.x = -std::abs(away.ball.velocity.x);
    simulation.restoreState(away);
    simulation.step(1.0 / 120.0, {});
    expect(simulation.state().rallyHits == hits, "ball travelling away does not bounce repeatedly");

    simulation.resetForServe();
    state = simulation.state();
    state.ball.position.x = state.field.left - state.ball.radius - 1.0;
    state.ball.velocity = {-100.0, 0.0};
    simulation.restoreState(state);
    expect(simulation.step(1.0 / 120.0, {}) == Side::Right, "left scoring boundary awards Right");
    expect(!simulation.step(1.0 / 120.0, {}).has_value(), "score is awarded exactly once");

    simulation.resetForServe();
    state = simulation.state();
    state.ball.position.x = state.field.right() + state.ball.radius + 1.0;
    state.ball.velocity = {100.0, 0.0};
    simulation.restoreState(state);
    expect(simulation.step(1.0 / 120.0, {}) == Side::Left, "right scoring boundary awards Left");

    simulation.resetForServe();
    const double base = simulation.state().ballSpeed;
    simulation.restoreState(collisionState(simulation, Side::Left, 0.2));
    simulation.step(1.0 / 120.0, {});
    expect(simulation.state().ballSpeed > base, "paddle hit increases speed");
    state = collisionState(simulation, Side::Left, 0.0);
    state.ballSpeed = PingPongSimulation::maximumBallSpeed;
    state.ball.velocity.x = -state.ballSpeed;
    simulation.restoreState(state);
    simulation.step(1.0 / 120.0, {});
    expect(near(simulation.state().ballSpeed, PingPongSimulation::maximumBallSpeed), "speed respects maximum clamp");
    simulation.resetForServe();
    expect(near(simulation.state().ballSpeed, PingPongSimulation::baseBallSpeed), "point reset restores base speed");

    PingPongSimulation first(77);
    PingPongSimulation second(77);
    ControlInput sameInput{Movement::Down, Movement::Up, 1.0, 0.84};
    for (int index = 0; index < 240; ++index) {
        first.step(1.0 / 120.0, sameInput);
        second.step(1.0 / 120.0, sameInput);
    }
    expect(near(first.state().ball.position.x, second.state().ball.position.x) &&
           near(first.state().ball.position.y, second.state().ball.position.y) &&
           near(first.state().leftPaddle.position.y, second.state().leftPaddle.position.y),
           "identical seed, state, and input update deterministically");
}

void sessionTests()
{
    PingPongSession session;
    SessionConfig config{"  Alice  ", "  Bob  ", GameMode::HumanVsHuman, AIDifficulty::Hard};
    session.startMatch(config);
    expect(session.score().left == 0 && session.score().right == 0, "initial score is 0-0");
    expect(session.playerName(Side::Left) == "Alice" && session.playerName(Side::Right) == "Bob", "names are trimmed");
    session.update(PingPongSession::serveCountdownSeconds);
    expect(session.state() == MatchState::Playing, "serve countdown enters play");
    expect(session.awardPoint(Side::Left), "point is accepted during play");
    expect(session.score().left == 1, "score increments correctly");
    expect(!session.awardPoint(Side::Left), "point cannot increment twice during point pause");
    for (unsigned int point = 1; point < PingPongSession::winningScore; ++point) {
        session.update(PingPongSession::pointPauseSeconds);
        expect(session.consumePointResetRequest(), "next point requests a simulation reset");
        session.update(PingPongSession::serveCountdownSeconds);
        session.awardPoint(Side::Left);
    }
    expect(session.matchFinished() && session.winner() == Side::Left, "first to five wins");
    expect(!session.awardPoint(Side::Right) && session.score().right == 0, "score cannot change after completion");
    session.rematch();
    expect(session.score().left == 0 && session.score().right == 0, "rematch resets score");
    expect(session.config().mode == GameMode::HumanVsHuman && session.config().difficulty == AIDifficulty::Hard &&
           session.playerName(Side::Left) == "Alice" && session.playerName(Side::Right) == "Bob",
           "rematch preserves configuration");

    config = {"", "ignored", GameMode::HumanVsComputer, AIDifficulty::Easy};
    session.startMatch(config);
    expect(session.playerName(Side::Left) == "Player 1" && session.playerName(Side::Right) == "Computer", "empty and computer names use defaults");
}

void aiTests()
{
    PingPongSimulation simulation(11);
    const auto initial = simulation.state();
    for (const auto difficulty : {AIDifficulty::Easy, AIDifficulty::Medium, AIDifficulty::Hard}) {
        PingPongAI ai(99);
        const auto before = simulation.state().rightPaddle.position.y;
        const auto command = ai.decide(1.0, simulation.state(), difficulty);
        expect(command.movement == Movement::Up || command.movement == Movement::None || command.movement == Movement::Down,
               "AI returns a legal movement intention");
        expect(command.speedScale > 0.0 && command.speedScale <= 1.0, "AI uses a legal paddle speed scale");
        expect(command.targetY >= initial.field.top + initial.rightPaddle.height / 2.0 &&
               command.targetY <= initial.field.bottom() - initial.rightPaddle.height / 2.0,
               "AI target stays inside the playfield");
        expect(near(before, simulation.state().rightPaddle.position.y), "AI decision never teleports the paddle");
        auto controlled = simulation.state();
        controlled.rightPaddle.position.y = controlled.field.top + 100.0;
        simulation.restoreState(controlled);
        ControlInput input;
        input.right = command.movement;
        input.rightSpeedScale = command.speedScale;
        const double startingY = simulation.state().rightPaddle.position.y;
        simulation.step(1.0 / 120.0, input);
        expect(std::abs(simulation.state().rightPaddle.position.y - startingY) <=
                   PingPongSimulation::paddleSpeed * command.speedScale / 120.0 + 1e-6,
               "AI movement remains constrained by normal paddle physics");
        simulation.restoreState(initial);
    }

    PingPongAI first(1234);
    PingPongAI second(1234);
    const auto firstCommand = first.decide(1.0, initial, AIDifficulty::Easy);
    const auto secondCommand = second.decide(1.0, initial, AIDifficulty::Easy);
    expect(firstCommand.movement == secondCommand.movement && near(firstCommand.targetY, secondCommand.targetY),
           "deterministic seed produces deterministic AI behavior");

    auto reflection = initial;
    reflection.ball.position = {600.0, reflection.field.top + 30.0};
    reflection.ball.velocity = {400.0, -260.0};
    PingPongAI hard(7);
    const double predicted = hard.predictInterceptY(reflection);
    expect(predicted >= reflection.field.top + reflection.ball.radius &&
           predicted <= reflection.field.bottom() - reflection.ball.radius,
           "Hard prediction handles wall-reflection intercept inside the field");
    expect(!near(predicted, reflection.ball.position.y + reflection.ball.velocity.y *
                 ((reflection.rightPaddle.position.x - reflection.ball.radius - reflection.ball.position.x) /
                  reflection.ball.velocity.x)), "Hard prediction reflects rather than using an out-of-bounds straight line");
}
} // namespace

int main()
{
    simulationTests();
    sessionTests();
    aiTests();
    if (failures != 0) {
        std::cerr << failures << " Ping Pong test(s) failed.\n";
        return EXIT_FAILURE;
    }
    std::cout << "All Ping Pong tests passed.\n";
    return EXIT_SUCCESS;
}
