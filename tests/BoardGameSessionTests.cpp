#include "BoardGameSession.hpp"
#include "ComputerMoveTask.hpp"
#include "MisereGame.hpp"
#include "NumericalGame.hpp"
#include "TurnBasedTestSupport.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>

using turn_based::BoardGameSession;
using turn_based::ComputerMoveTask;
using turn_based::MoveId;
using turn_based::PlayMode;
using turn_based::Seat;
using turn_based::SessionSetup;
using turn_based_test::check;

namespace {

// A game whose computer search always fails, to prove failures reach the caller.
class ThrowingGame final : public turn_based::TurnBasedGame {
public:
    std::unique_ptr<TurnBasedGame> clone() const override { return std::make_unique<ThrowingGame>(*this); }
    void reset() override {}
    Seat currentSeat() const override { return Seat::First; }
    turn_based::Outcome outcome() const override { return {}; }
    std::vector<MoveId> legalMoves() const override { return {0}; }
    bool play(MoveId) override { return false; }
    int movesPlayed() const override { return 0; }
    std::optional<MoveId> chooseComputerMove(std::mt19937&, const turn_based::CancelToken&) const override
    {
        throw std::runtime_error("search failed");
    }
};

struct SearchFlags {
    std::atomic<bool> started{false};
    std::atomic<bool> sawCancel{false};
};

// A game whose computer search runs until it is cancelled, so cancellation is tested
// deterministically rather than by racing a fast search.
class WaitingGame final : public turn_based::TurnBasedGame {
public:
    explicit WaitingGame(std::shared_ptr<SearchFlags> flags) : flags_(std::move(flags)) {}
    std::unique_ptr<TurnBasedGame> clone() const override { return std::make_unique<WaitingGame>(*this); }
    void reset() override {}
    Seat currentSeat() const override { return Seat::First; }
    turn_based::Outcome outcome() const override { return {}; }
    std::vector<MoveId> legalMoves() const override { return {0}; }
    bool play(MoveId) override { return false; }
    int movesPlayed() const override { return 0; }
    std::optional<MoveId> chooseComputerMove(std::mt19937&, const turn_based::CancelToken& cancel) const override
    {
        flags_->started = true;
        // The deadline only stops a broken cancellation from hanging the whole test run.
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (!cancel.cancelled() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        flags_->sawCancel = cancel.cancelled();
        return std::nullopt;
    }

private:
    std::shared_ptr<SearchFlags> flags_;
};

bool waitUntil(const std::atomic<bool>& flag)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    while (!flag && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return flag;
}

// Polls until the task reports a result or the deadline passes.
std::optional<std::optional<MoveId>> waitFor(ComputerMoveTask& task, std::chrono::seconds limit = std::chrono::seconds(20))
{
    const auto deadline = std::chrono::steady_clock::now() + limit;
    while (std::chrono::steady_clock::now() < deadline) {
        if (auto result = task.poll()) return result;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::nullopt;
}

void testHumanVsHuman()
{
    BoardGameSession session;
    check(!session.started(), "a new session has not started");
    bool threw = false;
    try {
        (void)session.game();
    } catch (const std::logic_error&) {
        threw = true;
    }
    check(threw, "reading the game before start is refused");
    threw = false;
    try {
        session.start(nullptr, {});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    check(threw, "a session cannot start without a game");

    session.start(std::make_unique<misere::MisereGame>(), SessionSetup{PlayMode::HumanVsHuman, "  Ada  ", "   ", Seat::First});
    check(session.started() && session.gameNumber() == 1, "the session starts its first game");
    check(session.nameOf(Seat::First) == "Ada" && session.nameOf(Seat::Second) == "Player 2",
          "names are trimmed and a blank name falls back to the seat default");
    check(!session.isComputerTurn() && !session.computerSeat() && !session.humanSeat(),
          "human-vs-human play has no computer seat");
    check(!session.takeCompletedOutcome(), "an unfinished game has no completed outcome");

    check(session.play(0) && !session.play(0), "legal moves apply and illegal ones are refused");
    for (const MoveId move : {3, 1, 4}) session.play(move);
    check(session.play(2), "the losing move is applied");
    const auto outcome = session.takeCompletedOutcome();
    check(outcome && outcome->winner == Seat::Second, "the finished outcome is reported once");
    check(!session.takeCompletedOutcome() && session.completionTaken(), "the outcome is never reported twice");
    check(!session.play(5), "a finished game refuses moves through the session");

    session.restart();
    check(session.gameNumber() == 2 && !session.completionTaken() && session.game().movesPlayed() == 0,
          "restart begins a fresh game with the same players");
    check(session.nameOf(Seat::First) == "Ada", "restart keeps the players");
}

void testHumanVsComputer()
{
    BoardGameSession first;
    first.start(std::make_unique<misere::MisereGame>(), SessionSetup{PlayMode::HumanVsComputer, "Ada", "ignored", Seat::First});
    check(first.nameOf(Seat::First) == "Ada" && first.nameOf(Seat::Second) == "Computer",
          "the computer takes the other seat and is named Computer");
    check(first.humanSeat() == Seat::First && first.computerSeat() == Seat::Second, "seats follow the chosen side");
    check(!first.isComputerTurn(), "the human moves first when playing the first seat");
    first.play(4);
    check(first.isComputerTurn(), "after the human moves it is the computer's turn");

    BoardGameSession second;
    second.start(std::make_unique<misere::MisereGame>(), SessionSetup{PlayMode::HumanVsComputer, "Ada", "", Seat::Second});
    check(second.nameOf(Seat::First) == "Computer" && second.nameOf(Seat::Second) == "Ada",
          "playing the second seat gives the computer the first move");
    check(second.isComputerTurn(), "the computer opens when the human plays second");
}

void testComputerMoveTask()
{
    {
        ComputerMoveTask task;
        check(!task.running() && !task.poll(), "an idle task has no result");
        numerical_ttt::NumericalGame game;
        task.start(game.clone(), 7);
        check(task.running(), "a started task is running");
        const auto result = waitFor(task);
        check(result && *result && game.isLegal(**result), "the background search returns a legal move");
        check(!task.running() && !task.poll(), "a result is delivered exactly once");

        numerical_ttt::NumericalGame copy;
        std::mt19937 random(7);
        turn_based::CancelToken cancel;
        check(result && *result == copy.chooseComputerMove(random, cancel),
              "the background move matches the same seed on the calling thread");
    }
    {
        // A search that only ends when cancelled: cancel() must signal it and wait for it.
        auto flags = std::make_shared<SearchFlags>();
        ComputerMoveTask task;
        task.start(std::make_unique<WaitingGame>(flags), 1);
        check(waitUntil(flags->started) && task.running() && !task.poll(), "a running search has no result yet");
        task.cancel();
        check(flags->sawCancel, "cancel() signals the running search and waits for it to stop");
        check(!task.running() && !task.poll(), "a cancelled search reports nothing");
    }
    {
        // Starting again cancels the earlier search and answers for the newest position.
        auto flags = std::make_shared<SearchFlags>();
        ComputerMoveTask task;
        task.start(std::make_unique<WaitingGame>(flags), 1);
        check(waitUntil(flags->started), "the first search started");
        misere::MisereGame late;
        late.play(4);
        task.start(late.clone(), 2);
        check(flags->sawCancel, "starting a new search cancels the running one");
        const auto result = waitFor(task);
        check(result && *result && late.isLegal(**result), "a restarted search answers for the newest position");
    }
    {
        auto flags = std::make_shared<SearchFlags>();
        {
            ComputerMoveTask task;
            task.start(std::make_unique<WaitingGame>(flags), 1);
            check(waitUntil(flags->started), "the search to be abandoned started");
        }
        check(flags->sawCancel, "destroying a running task cancels its search and joins the thread");
    }
    {
        ComputerMoveTask task;
        task.start(std::make_unique<ThrowingGame>(), 1);
        bool threw = false;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
        while (std::chrono::steady_clock::now() < deadline) {
            try {
                if (task.poll()) break;
            } catch (const std::runtime_error&) {
                threw = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        check(threw && !task.running(), "a failed search reports its error to the caller");
    }
}

} // namespace

int main()
{
    testHumanVsHuman();
    testHumanVsComputer();
    testComputerMoveTask();
    return turn_based_test::finish("Board game session");
}
