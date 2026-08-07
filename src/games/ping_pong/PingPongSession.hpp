#pragma once

#include "PingPongTypes.hpp"

#include <optional>
#include <string>

namespace ping_pong {

class PingPongSession {
public:
    static constexpr unsigned int winningScore = 5;
    static constexpr double pointPauseSeconds = 0.85;
    static constexpr double serveCountdownSeconds = 3.0;

    void startMatch(SessionConfig config);
    bool awardPoint(Side scorer);
    void update(double seconds);
    bool consumePointResetRequest() noexcept;
    void rematch();

    const SessionConfig& config() const noexcept { return config_; }
    const MatchScore& score() const noexcept { return score_; }
    MatchState state() const noexcept { return state_; }
    double stateSecondsRemaining() const noexcept { return stateSecondsRemaining_; }
    bool matchFinished() const noexcept { return state_ == MatchState::MatchFinished; }
    std::optional<Side> lastScorer() const noexcept { return lastScorer_; }
    std::optional<Side> winner() const noexcept { return winner_; }
    const std::string& playerName(Side side) const noexcept;

private:
    void beginMatchFlow();

    SessionConfig config_{};
    MatchScore score_{};
    MatchState state_{MatchState::ServeCountdown};
    double stateSecondsRemaining_{serveCountdownSeconds};
    std::optional<Side> lastScorer_{};
    std::optional<Side> winner_{};
    bool resetRequested_{true};
};

} // namespace ping_pong
