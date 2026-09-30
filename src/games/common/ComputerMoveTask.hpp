#pragma once

#include "TurnBasedGame.hpp"

#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <thread>

namespace turn_based {

// Runs a computer move search on a worker thread so the UI never blocks. The worker only
// touches its own snapshot (a clone of the game) and a shared cancellation flag. Cancelling
// or destroying the task sets the flag and joins the thread, so no work outlives its owner.
class ComputerMoveTask {
public:
    ComputerMoveTask() = default;
    ~ComputerMoveTask();

    ComputerMoveTask(const ComputerMoveTask&) = delete;
    ComputerMoveTask& operator=(const ComputerMoveTask&) = delete;

    // Starts searching `snapshot` for the move of the seat to move. Any earlier search is cancelled.
    void start(std::unique_ptr<TurnBasedGame> snapshot, std::uint32_t seed);
    void cancel();
    bool running() const noexcept { return future_.valid(); }

    // Once the search has finished, returns its result exactly once (a move, or nullopt when the
    // game had no legal move or the search was cancelled); otherwise returns nothing.
    std::optional<std::optional<MoveId>> poll();

private:
    std::thread worker_;
    std::future<std::optional<MoveId>> future_;
    CancelToken cancel_;
};

} // namespace turn_based
