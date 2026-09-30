#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

// Shared, SFML-independent contract for the graphical versions of the original board games.
// Each game keeps its own typed rules and state; this interface exposes only what generic
// code needs: turn order, legal moves as opaque ids, the outcome, and the computer move.
namespace turn_based {

// Seat in turn order. First always moves first (X, S, odd numbers, ...).
enum class Seat : std::uint8_t { First = 0, Second = 1 };

constexpr Seat otherSeat(Seat seat) noexcept { return seat == Seat::First ? Seat::Second : Seat::First; }
constexpr std::size_t seatIndex(Seat seat) noexcept { return static_cast<std::size_t>(seat); }

enum class OutcomeStatus { InProgress, Won, Draw };

struct Outcome {
    OutcomeStatus status{OutcomeStatus::InProgress};
    std::optional<Seat> winner;  // set only when status == Won
    std::string reason;          // short explanation for the result screen

    bool finished() const noexcept { return status != OutcomeStatus::InProgress; }
};

// Game-specific move encoding (for example row * columns + column). Always >= 0.
using MoveId = std::int32_t;

struct SeatScores {
    int first{};
    int second{};
};

// Cooperative cancellation for computer searches that run on a worker thread. Copies share
// one flag, so the scene can cancel work it has handed to a background task.
class CancelToken {
public:
    CancelToken() : flag_(std::make_shared<std::atomic<bool>>(false)) {}

    void cancel() const noexcept { flag_->store(true, std::memory_order_relaxed); }
    bool cancelled() const noexcept { return flag_->load(std::memory_order_relaxed); }

private:
    std::shared_ptr<std::atomic<bool>> flag_;
};

class TurnBasedGame {
public:
    virtual ~TurnBasedGame() = default;

    // Deep copy used for computer searches on a worker thread and for tests.
    virtual std::unique_ptr<TurnBasedGame> clone() const = 0;

    // Starts a fresh game with the same configuration.
    virtual void reset() = 0;

    virtual Seat currentSeat() const = 0;
    virtual Outcome outcome() const = 0;
    virtual std::vector<MoveId> legalMoves() const = 0;
    virtual bool isLegal(MoveId move) const;

    // Applies a legal move for the seat to move. Returns false, leaving the state unchanged,
    // when the move is illegal or the game has already finished.
    virtual bool play(MoveId move) = 0;

    virtual int movesPlayed() const = 0;

    // Game-defined points (for example SUS sequences or 5x5 triples); nullopt when the game
    // has no meaningful score.
    virtual std::optional<SeatScores> scores() const { return std::nullopt; }

    // The game's computer strategy for the seat to move. Must only read `*this`, so it can run
    // on a clone in a background thread. Returns a legal move, or nullopt when the game is
    // finished, has no legal move, or `cancel` was triggered.
    virtual std::optional<MoveId> chooseComputerMove(std::mt19937& random, const CancelToken& cancel) const = 0;
};

inline bool TurnBasedGame::isLegal(MoveId move) const
{
    if (outcome().finished()) return false;
    for (const MoveId candidate : legalMoves()) {
        if (candidate == move) return true;
    }
    return false;
}

inline Outcome winFor(Seat seat, std::string reason)
{
    return {OutcomeStatus::Won, seat, std::move(reason)};
}

inline Outcome drawResult(std::string reason)
{
    return {OutcomeStatus::Draw, std::nullopt, std::move(reason)};
}

} // namespace turn_based
