#include "ComputerMoveTask.hpp"

#include <chrono>
#include <exception>
#include <random>
#include <utility>

namespace turn_based {

ComputerMoveTask::~ComputerMoveTask()
{
    cancel();
}

void ComputerMoveTask::start(std::unique_ptr<TurnBasedGame> snapshot, std::uint32_t seed)
{
    cancel();
    cancel_ = CancelToken{};
    std::promise<std::optional<MoveId>> promise;
    future_ = promise.get_future();
    worker_ = std::thread([game = std::move(snapshot), token = cancel_, seed, result = std::move(promise)]() mutable {
        try {
            std::mt19937 random(seed);
            result.set_value(game ? game->chooseComputerMove(random, token) : std::nullopt);
        } catch (...) {
            result.set_exception(std::current_exception());
        }
    });
}

void ComputerMoveTask::cancel()
{
    cancel_.cancel();
    if (worker_.joinable()) worker_.join();
    future_ = {};
}

std::optional<std::optional<MoveId>> ComputerMoveTask::poll()
{
    if (!future_.valid() || future_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return std::nullopt;
    if (worker_.joinable()) worker_.join();
    auto value = future_.get();  // rethrows a search failure to the caller
    return value;
}

} // namespace turn_based
