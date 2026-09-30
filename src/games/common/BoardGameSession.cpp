#include "BoardGameSession.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace turn_based {
namespace {
std::string trimmed(const std::string& value, const std::string& fallback)
{
    const auto notSpace = [](unsigned char character) { return !std::isspace(character); };
    const auto first = std::find_if(value.begin(), value.end(), notSpace);
    const auto last = std::find_if(value.rbegin(), value.rend(), notSpace).base();
    if (first >= last) return fallback;
    return std::string(first, last);
}
} // namespace

void BoardGameSession::start(std::unique_ptr<TurnBasedGame> game, SessionSetup setup)
{
    if (!game) throw std::invalid_argument("A board game session needs a game");
    setup.firstName = trimmed(setup.firstName, "Player 1");
    setup.secondName = setup.mode == PlayMode::HumanVsComputer ? "Computer" : trimmed(setup.secondName, "Player 2");
    setup_ = std::move(setup);
    if (setup_.mode == PlayMode::HumanVsComputer) {
        firstSeatName_ = setup_.humanSeat == Seat::First ? setup_.firstName : setup_.secondName;
        secondSeatName_ = setup_.humanSeat == Seat::First ? setup_.secondName : setup_.firstName;
    } else {
        firstSeatName_ = setup_.firstName;
        secondSeatName_ = setup_.secondName;
    }
    game_ = std::move(game);
    game_->reset();
    completionTaken_ = false;
    gameNumber_ = 1;
}

void BoardGameSession::restart()
{
    if (!game_) return;
    game_->reset();
    completionTaken_ = false;
    ++gameNumber_;
}

const TurnBasedGame& BoardGameSession::game() const
{
    if (!game_) throw std::logic_error("The board game session has not started");
    return *game_;
}

bool BoardGameSession::play(MoveId move)
{
    if (!game_ || game_->outcome().finished()) return false;
    return game_->play(move);
}

std::optional<Seat> BoardGameSession::computerSeat() const
{
    if (setup_.mode != PlayMode::HumanVsComputer) return std::nullopt;
    return otherSeat(setup_.humanSeat);
}

std::optional<Seat> BoardGameSession::humanSeat() const
{
    if (setup_.mode != PlayMode::HumanVsComputer) return std::nullopt;
    return setup_.humanSeat;
}

bool BoardGameSession::isComputerTurn() const
{
    const auto computer = computerSeat();
    return game_ && computer && !game_->outcome().finished() && game_->currentSeat() == *computer;
}

const std::string& BoardGameSession::nameOf(Seat seat) const
{
    return seat == Seat::First ? firstSeatName_ : secondSeatName_;
}

std::optional<Outcome> BoardGameSession::takeCompletedOutcome()
{
    if (!game_ || completionTaken_) return std::nullopt;
    auto outcome = game_->outcome();
    if (!outcome.finished()) return std::nullopt;
    completionTaken_ = true;
    return outcome;
}

} // namespace turn_based
