#include "TicTacToeSession.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace classic_ttt {
namespace {
std::string trimmed(std::string value, const std::string& fallback)
{
    const auto notSpace = [](unsigned char character) { return !std::isspace(character); };
    const auto first = std::find_if(value.begin(), value.end(), notSpace);
    const auto last = std::find_if(value.rbegin(), value.rend(), notSpace).base();
    if (first >= last) return fallback;
    return std::string(first, last);
}
} // namespace

void TicTacToeSession::startNewMatch(SessionConfig config)
{
    config.playerOneName = trimmed(std::move(config.playerOneName), "Player 1");
    config.playerTwoName = config.mode == GameMode::HumanVsComputer
        ? "Computer" : trimmed(std::move(config.playerTwoName), "Player 2");
    if (config.humanMark == Cell::Empty) config.humanMark = Cell::X;
    config_ = std::move(config);
    score_ = {};
    roundNumber_ = 1;
    clearBoard();
}

bool TicTacToeSession::playMove(Position position)
{
    if (matchFinished() || !board_.placeMark(position, currentTurn_)) return false;
    if (board_.status() == GameStatus::InProgress) {
        currentTurn_ = opposite(currentTurn_);
    } else {
        recordRoundOnce();
    }
    return true;
}

void TicTacToeSession::restartRound()
{
    if (roundRecorded_) {
        switch (board_.status()) {
        case GameStatus::XWon: --score_.xWins; break;
        case GameStatus::OWon: --score_.oWins; break;
        case GameStatus::Draw: --score_.draws; break;
        case GameStatus::InProgress: break;
        }
    }
    clearBoard();
}

bool TicTacToeSession::nextRound()
{
    if (board_.status() == GameStatus::InProgress || matchFinished()) return false;
    ++roundNumber_;
    clearBoard();
    return true;
}

void TicTacToeSession::rematch()
{
    score_ = {};
    roundNumber_ = 1;
    clearBoard();
}

bool TicTacToeSession::matchFinished() const noexcept
{
    const auto needed = winsNeeded();
    return score_.xWins >= needed || score_.oWins >= needed;
}

bool TicTacToeSession::isComputerTurn() const noexcept
{
    return config_.mode == GameMode::HumanVsComputer && currentTurn_ == computerMark() &&
           board_.status() == GameStatus::InProgress && !matchFinished();
}

Cell TicTacToeSession::computerMark() const noexcept
{
    return config_.mode == GameMode::HumanVsComputer ? opposite(config_.humanMark) : Cell::Empty;
}

std::string TicTacToeSession::playerName(Cell mark) const
{
    if (config_.mode == GameMode::HumanVsComputer) {
        return mark == config_.humanMark ? config_.playerOneName : config_.playerTwoName;
    }
    return mark == Cell::X ? config_.playerOneName : config_.playerTwoName;
}

unsigned int TicTacToeSession::winsNeeded() const noexcept
{
    const auto games = static_cast<unsigned int>(config_.bestOf);
    return games / 2u + 1u;
}

void TicTacToeSession::recordRoundOnce()
{
    if (roundRecorded_) return;
    switch (board_.status()) {
    case GameStatus::XWon: ++score_.xWins; break;
    case GameStatus::OWon: ++score_.oWins; break;
    case GameStatus::Draw: ++score_.draws; break;
    case GameStatus::InProgress: return;
    }
    roundRecorded_ = true;
}

void TicTacToeSession::clearBoard()
{
    board_.reset();
    currentTurn_ = Cell::X;
    roundRecorded_ = false;
}

} // namespace classic_ttt
