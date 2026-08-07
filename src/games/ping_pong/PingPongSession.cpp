#include "PingPongSession.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace ping_pong {
namespace {
std::string trimmed(std::string value, const char* fallback)
{
    const auto notSpace = [](unsigned char character) { return !std::isspace(character); };
    const auto first = std::find_if(value.begin(), value.end(), notSpace);
    const auto last = std::find_if(value.rbegin(), value.rend(), notSpace).base();
    if (first >= last) return fallback;
    return std::string(first, last);
}
} // namespace

void PingPongSession::startMatch(SessionConfig config)
{
    config.leftPlayerName = trimmed(std::move(config.leftPlayerName), "Player 1");
    config.rightPlayerName = config.mode == GameMode::HumanVsComputer
        ? "Computer" : trimmed(std::move(config.rightPlayerName), "Player 2");
    config_ = std::move(config);
    score_ = {};
    beginMatchFlow();
}

bool PingPongSession::awardPoint(Side scorer)
{
    if (state_ != MatchState::Playing) return false;
    lastScorer_ = scorer;
    auto& score = scorer == Side::Left ? score_.left : score_.right;
    ++score;
    if (score >= winningScore) {
        winner_ = scorer;
        state_ = MatchState::MatchFinished;
        stateSecondsRemaining_ = 0.0;
    } else {
        state_ = MatchState::PointScored;
        stateSecondsRemaining_ = pointPauseSeconds;
    }
    return true;
}

void PingPongSession::update(double seconds)
{
    if (seconds <= 0.0 || state_ == MatchState::Playing || state_ == MatchState::MatchFinished) return;
    stateSecondsRemaining_ = std::max(0.0, stateSecondsRemaining_ - seconds);
    if (stateSecondsRemaining_ > 0.0) return;
    if (state_ == MatchState::PointScored) {
        state_ = MatchState::ServeCountdown;
        stateSecondsRemaining_ = serveCountdownSeconds;
        resetRequested_ = true;
    } else if (state_ == MatchState::ServeCountdown) {
        state_ = MatchState::Playing;
    }
}

bool PingPongSession::consumePointResetRequest() noexcept
{
    const bool result = resetRequested_;
    resetRequested_ = false;
    return result;
}

void PingPongSession::rematch()
{
    score_ = {};
    beginMatchFlow();
}

const std::string& PingPongSession::playerName(Side side) const noexcept
{
    return side == Side::Left ? config_.leftPlayerName : config_.rightPlayerName;
}

void PingPongSession::beginMatchFlow()
{
    state_ = MatchState::ServeCountdown;
    stateSecondsRemaining_ = serveCountdownSeconds;
    lastScorer_.reset();
    winner_.reset();
    resetRequested_ = true;
}

} // namespace ping_pong
