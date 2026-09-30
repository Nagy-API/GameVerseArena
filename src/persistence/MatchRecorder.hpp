#pragma once

#include "MatchService.hpp"
#include "PersistenceTypes.hpp"
#include "PingPongSession.hpp"
#include "TicTacToeSession.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace persistence {

class MatchRecorder {
public:
    using SteadyNow = std::function<std::chrono::steady_clock::time_point()>;
    using UtcNow = std::function<std::int64_t()>;

    explicit MatchRecorder(MatchService& service,
                           SteadyNow steadyNow = [] { return std::chrono::steady_clock::now(); },
                           UtcNow utcNow = [] {
                               return std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
                           })
        : service_(service), steadyNow_(std::move(steadyNow)), utcNow_(std::move(utcNow)) {}

    void beginTicTacToe(const Profile& profile, const classic_ttt::SessionConfig& config)
    {
        reset(profile);
        game_ = GameKey::ClassicTicTacToe;
        mode_ = config.mode == classic_ttt::GameMode::HumanVsHuman
            ? MatchMode::HumanVsHuman : MatchMode::HumanVsComputer;
        profileName_ = config.playerOneName;
        opponentName_ = config.playerTwoName;
        ticTacToeMark_ = config.mode == classic_ttt::GameMode::HumanVsComputer
            ? config.humanMark : classic_ttt::Cell::X;
        difficulty_ = config.mode == classic_ttt::GameMode::HumanVsComputer
            ? difficulty(config.difficulty) : DifficultyKey::None;
        format_ = config.bestOf == classic_ttt::BestOf::Single ? "single"
            : config.bestOf == classic_ttt::BestOf::Three ? "best_of_3" : "best_of_5";
    }

    void beginPingPong(const Profile& profile, const ping_pong::SessionConfig& config)
    {
        reset(profile);
        game_ = GameKey::PingPong;
        mode_ = config.mode == ping_pong::GameMode::HumanVsHuman
            ? MatchMode::HumanVsHuman : MatchMode::HumanVsComputer;
        profileName_ = config.leftPlayerName;
        opponentName_ = config.rightPlayerName;
        difficulty_ = config.mode == ping_pong::GameMode::HumanVsComputer
            ? difficulty(config.difficulty) : DifficultyKey::None;
        format_ = "first_to_5";
    }

    // A shared board game (one game per match). The profile's player holds the first seat when
    // `profileMovesFirst` (always Player 1 in human-vs-human play); the computer opponent of
    // these games has one strategy, stored as the 'standard' difficulty.
    void beginBoardGame(const Profile& profile, GameKey game, MatchMode mode, bool profileMovesFirst,
                        std::string profileName, std::string opponentName)
    {
        const auto sides = boardGameSides(game);
        if (!sides) throw std::invalid_argument("That game is not played in the shared board-game scenes");
        reset(profile);
        game_ = game;
        mode_ = mode;
        profileName_ = std::move(profileName);
        opponentName_ = std::move(opponentName);
        difficulty_ = mode == MatchMode::HumanVsComputer ? DifficultyKey::Standard : DifficultyKey::None;
        format_ = "single";
        profileSide_ = profileMovesFirst ? sides->first : sides->second;
        opponentSide_ = profileMovesFirst ? sides->second : sides->first;
        boardGameConfigured_ = true;
    }

    // Restart or rematch with the same players and sides: a fresh, unrecorded game.
    void restartBoardGame()
    {
        if (!boardGameConfigured_) return;
        Profile captured; captured.id = profileId_; captured.displayName = profileName_;
        reset(captured);
        boardGameConfigured_ = true;
    }

    // Records the finished game once. Points are passed only for games that score them.
    bool completeBoardGame(MatchResult result, std::optional<int> profileScore, std::optional<int> opponentScore)
    {
        if (!boardGameConfigured_ || !active_ || finalized_) return false;
        CompletedMatch match = base();
        match.profileSideOrMark = profileSide_;
        match.opponentSideOrMark = opponentSide_;
        match.result = result;
        match.profileScore = profileScore;
        match.opponentScore = opponentScore;
        return persistOnce(match);
    }

    void restartTicTacToe(const classic_ttt::SessionConfig& config)
    {
        Profile captured; captured.id = profileId_; captured.displayName = profileName_;
        beginTicTacToe(captured, config);
    }

    void restartPingPong(const ping_pong::SessionConfig& config)
    {
        Profile captured; captured.id = profileId_; captured.displayName = profileName_;
        beginPingPong(captured, config);
    }

    bool completeTicTacToe(const classic_ttt::TicTacToeSession& session)
    {
        if (!ready(GameKey::ClassicTicTacToe) || !session.matchFinished()) return false;
        CompletedMatch match = base();
        const auto mark = ticTacToeMark_;
        const auto opponent = classic_ttt::opposite(mark);
        match.profileSideOrMark = std::string(1, classic_ttt::markCharacter(mark));
        match.opponentSideOrMark = std::string(1, classic_ttt::markCharacter(opponent));
        match.profileScore = mark == classic_ttt::Cell::X
            ? static_cast<int>(session.score().xWins) : static_cast<int>(session.score().oWins);
        match.opponentScore = opponent == classic_ttt::Cell::X
            ? static_cast<int>(session.score().xWins) : static_cast<int>(session.score().oWins);
        match.drawValue = static_cast<int>(session.score().draws);
        if (session.score().draws > 0 && *match.profileScore == 0 && *match.opponentScore == 0) {
            match.result = MatchResult::Draw;
        } else {
            match.result = *match.profileScore > *match.opponentScore ? MatchResult::Win : MatchResult::Loss;
        }
        return persistOnce(match);
    }

    bool completePingPong(const ping_pong::PingPongSession& session)
    {
        if (!ready(GameKey::PingPong) || !session.matchFinished() || !session.winner()) return false;
        CompletedMatch match = base();
        match.profileSideOrMark = "Left";
        match.opponentSideOrMark = "Right";
        match.profileScore = static_cast<int>(session.score().left);
        match.opponentScore = static_cast<int>(session.score().right);
        match.result = *session.winner() == ping_pong::Side::Left ? MatchResult::Win : MatchResult::Loss;
        return persistOnce(match);
    }

    void pause() noexcept
    {
        if (!active_ || !running_) return;
        accumulated_ += steadyNow_() - runningSince_;
        running_ = false;
    }

    void resume() noexcept
    {
        if (!active_ || running_ || finalized_) return;
        runningSince_ = steadyNow_();
        running_ = true;
    }

    void abandon() noexcept { active_ = false; running_ = false; finalized_ = true; boardGameConfigured_ = false; }
    bool finalized() const noexcept { return finalized_; }
    std::int64_t profileId() const noexcept { return profileId_; }

private:
    using SteadyDuration = std::chrono::steady_clock::duration;

    static DifficultyKey difficulty(classic_ttt::AIDifficulty value) noexcept
    {
        if (value == classic_ttt::AIDifficulty::Easy) return DifficultyKey::Easy;
        if (value == classic_ttt::AIDifficulty::Hard) return DifficultyKey::Hard;
        return DifficultyKey::Medium;
    }

    static DifficultyKey difficulty(ping_pong::AIDifficulty value) noexcept
    {
        if (value == ping_pong::AIDifficulty::Easy) return DifficultyKey::Easy;
        if (value == ping_pong::AIDifficulty::Hard) return DifficultyKey::Hard;
        return DifficultyKey::Medium;
    }

    void reset(const Profile& profile)
    {
        boardGameConfigured_ = false;
        profileId_ = profile.id;
        startedAt_ = utcNow_();
        accumulated_ = SteadyDuration::zero();
        runningSince_ = steadyNow_();
        active_ = true; running_ = true; finalized_ = false;
    }

    bool ready(GameKey expected) const noexcept { return active_ && !finalized_ && game_ == expected; }

    CompletedMatch base()
    {
        pause();
        CompletedMatch match;
        match.profileId = profileId_; match.game = game_; match.mode = mode_;
        match.opponentName = opponentName_; match.profileDisplayName = profileName_;
        match.difficulty = difficulty_; match.matchFormat = format_;
        match.durationMs = std::max<std::int64_t>(0, std::chrono::duration_cast<std::chrono::milliseconds>(accumulated_).count());
        match.startedAt = startedAt_; match.completedAt = std::max(startedAt_, utcNow_());
        return match;
    }

    bool persistOnce(const CompletedMatch& match)
    {
        finalized_ = true; active_ = false; running_ = false;
        service_.recordCompleted(match);
        return true;
    }

    MatchService& service_;
    SteadyNow steadyNow_;
    UtcNow utcNow_;
    std::int64_t profileId_{};
    std::int64_t startedAt_{};
    GameKey game_{GameKey::ClassicTicTacToe};
    MatchMode mode_{MatchMode::HumanVsHuman};
    DifficultyKey difficulty_{DifficultyKey::None};
    classic_ttt::Cell ticTacToeMark_{classic_ttt::Cell::X};
    std::string profileName_;
    std::string opponentName_;
    std::string format_;
    std::string profileSide_;
    std::string opponentSide_;
    bool boardGameConfigured_{};
    SteadyDuration accumulated_{};
    std::chrono::steady_clock::time_point runningSince_{};
    bool active_{};
    bool running_{};
    bool finalized_{};
};

} // namespace persistence
