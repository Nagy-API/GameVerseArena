#pragma once

#include <cstdint>
#include <string>

namespace ping_pong {

struct Vec2 {
    double x{};
    double y{};
};

enum class Side { Left, Right };
enum class GameMode { HumanVsHuman, HumanVsComputer };
enum class AIDifficulty { Easy, Medium, Hard };
enum class Movement { Up = -1, None = 0, Down = 1 };
enum class MatchState { ServeCountdown, Playing, PointScored, MatchFinished };

struct FieldDimensions {
    double left{70.0};
    double top{150.0};
    double width{1140.0};
    double height{500.0};

    double right() const noexcept { return left + width; }
    double bottom() const noexcept { return top + height; }
};

struct PaddleState {
    Vec2 position{};
    double width{18.0};
    double height{108.0};
};

struct BallState {
    Vec2 position{};
    Vec2 velocity{};
    double radius{12.0};
};

struct SimulationState {
    FieldDimensions field{};
    PaddleState leftPaddle{};
    PaddleState rightPaddle{};
    BallState ball{};
    double ballSpeed{};
    unsigned int rallyHits{};
};

struct ControlInput {
    Movement left{Movement::None};
    Movement right{Movement::None};
    double leftSpeedScale{1.0};
    double rightSpeedScale{1.0};
};

struct MatchScore {
    unsigned int left{};
    unsigned int right{};
};

struct SessionConfig {
    std::string leftPlayerName{"Player 1"};
    std::string rightPlayerName{"Player 2"};
    GameMode mode{GameMode::HumanVsHuman};
    AIDifficulty difficulty{AIDifficulty::Medium};
};

inline Side opposite(Side side) noexcept
{
    return side == Side::Left ? Side::Right : Side::Left;
}

} // namespace ping_pong
