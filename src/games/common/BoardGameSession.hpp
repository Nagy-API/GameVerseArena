#pragma once

#include "TurnBasedGame.hpp"

#include <memory>
#include <optional>
#include <string>

namespace turn_based {

enum class PlayMode { HumanVsHuman, HumanVsComputer };

struct SessionSetup {
    PlayMode mode{PlayMode::HumanVsHuman};
    std::string firstName{"Player 1"};   // Player 1 (the active profile's default name)
    std::string secondName{"Player 2"};  // Player 2 in human-vs-human play
    Seat humanSeat{Seat::First};         // human-vs-computer: the human's seat
};

// One board game between two named players. It owns the game, applies moves, knows whose turn
// belongs to the computer, and reports each finished game's outcome exactly once.
class BoardGameSession {
public:
    void start(std::unique_ptr<TurnBasedGame> game, SessionSetup setup);
    // Starts a fresh game with the same players and setup (Restart / Rematch).
    void restart();

    bool started() const noexcept { return static_cast<bool>(game_); }
    const TurnBasedGame& game() const;
    const SessionSetup& setup() const noexcept { return setup_; }

    // Applies a move for the seat to move; false (no change) if illegal or already finished.
    bool play(MoveId move);

    bool isComputerTurn() const;
    std::optional<Seat> computerSeat() const;
    std::optional<Seat> humanSeat() const;  // set in human-vs-computer play
    const std::string& nameOf(Seat seat) const;

    // Returns the outcome the first time it is called after the game has finished, and nullopt
    // before that and on every later call, so a result can never be processed twice.
    std::optional<Outcome> takeCompletedOutcome();
    bool completionTaken() const noexcept { return completionTaken_; }
    int gameNumber() const noexcept { return gameNumber_; }

private:
    std::unique_ptr<TurnBasedGame> game_;
    SessionSetup setup_{};
    std::string firstSeatName_;
    std::string secondSeatName_;
    bool completionTaken_{false};
    int gameNumber_{0};
};

} // namespace turn_based
