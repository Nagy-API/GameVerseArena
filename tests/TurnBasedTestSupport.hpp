#pragma once

#include "TurnBasedGame.hpp"

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace turn_based_test {

inline int failures = 0;
inline int checks = 0;

inline void check(bool condition, const std::string& message)
{
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

inline int finish(const char* suite)
{
    if (failures == 0) {
        std::cout << suite << " tests passed: " << checks << " checks\n";
        return 0;
    }
    std::cerr << failures << " of " << checks << ' ' << suite << " checks failed\n";
    return 1;
}

using Factory = std::function<std::unique_ptr<turn_based::TurnBasedGame>()>;

// Contract checks every graphical board game must satisfy: seeded random games and
// computer-vs-computer games only play legal moves and alternate turns, and (for games whose
// length is bounded, `gamesAlwaysEnd`) finish within `maximumMoves`; finished games refuse moves;
// reset restores the opening position; clones are independent; play is deterministic.
inline void checkContract(const std::string& name, const Factory& create, int maximumMoves, int randomGames = 60,
                          bool gamesAlwaysEnd = true)
{
    using turn_based::MoveId;
    {
        auto game = create();
        check(!game->outcome().finished() && game->currentSeat() == turn_based::Seat::First &&
                  game->movesPlayed() == 0 && !game->legalMoves().empty(),
              name + ": a new game starts in progress with the first seat to move");
        check(!game->isLegal(-1) && !game->play(-1), name + ": a negative move id is illegal");
        check(!game->play(1000000), name + ": an out-of-range move id is illegal");
    }

    for (int seed = 1; seed <= randomGames; ++seed) {
        auto game = create();
        std::mt19937 random(static_cast<std::uint32_t>(seed));
        int moves = 0;
        bool legal = true;
        while (!game->outcome().finished() && moves < maximumMoves) {
            const auto options = game->legalMoves();
            if (options.empty()) { legal = false; break; }
            std::uniform_int_distribution<std::size_t> pick(0, options.size() - 1);
            const MoveId move = options[pick(random)];
            const auto mover = game->currentSeat();
            legal = legal && game->isLegal(move) && game->play(move);
            ++moves;
            if (!game->outcome().finished()) legal = legal && game->currentSeat() == turn_based::otherSeat(mover);
        }
        check(legal, name + ": random game " + std::to_string(seed) + " only plays legal moves and alternates turns");
        if (gamesAlwaysEnd) {
            check(game->outcome().finished(), name + ": random game " + std::to_string(seed) + " ends within " +
                                                  std::to_string(maximumMoves) + " moves");
        }
        if (game->outcome().finished()) {
            const auto outcome = game->outcome();
            check((outcome.status == turn_based::OutcomeStatus::Won) == outcome.winner.has_value(),
                  name + ": a finished game has a winner exactly when it is won");
            check(game->legalMoves().empty(), name + ": a finished game offers no moves");
            auto clone = game->clone();
            check(!game->play(0) && !game->play(1) && clone->outcome().status == game->outcome().status,
                  name + ": a finished game refuses further moves");
            std::mt19937 dummy(1);
            turn_based::CancelToken cancel;
            check(!game->chooseComputerMove(dummy, cancel), name + ": the computer does not move in a finished game");
        }
    }

    for (int seed = 1; seed <= 12; ++seed) {
        auto game = create();
        std::mt19937 random(static_cast<std::uint32_t>(seed * 7919));
        turn_based::CancelToken cancel;
        int moves = 0;
        bool legal = true;
        while (!game->outcome().finished() && moves < maximumMoves) {
            const auto move = game->chooseComputerMove(random, cancel);
            const auto mover = game->currentSeat();
            legal = legal && move.has_value() && game->isLegal(*move) && game->play(*move);
            if (!legal) break;
            ++moves;
            if (!game->outcome().finished()) legal = game->currentSeat() == turn_based::otherSeat(mover);
        }
        check(legal, name + ": computer-vs-computer game " + std::to_string(seed) +
                         " only plays legal moves and alternates turns");
        if (gamesAlwaysEnd) {
            check(game->outcome().finished(), name + ": computer-vs-computer game " + std::to_string(seed) + " ends");
        }
    }

    {
        auto game = create();
        const auto opening = game->legalMoves();
        check(game->play(opening.front()), name + ": the first legal move can be played");
        auto snapshot = game->clone();
        const auto second = game->legalMoves();
        if (!second.empty()) game->play(second.front());
        check(snapshot->movesPlayed() == 1, name + ": a clone is unaffected by later moves on the original");
        game->reset();
        check(game->movesPlayed() == 0 && game->currentSeat() == turn_based::Seat::First &&
                  !game->outcome().finished() && game->legalMoves() == opening,
              name + ": reset restores the opening position");
    }

    {
        auto first = create();
        auto second = create();
        std::mt19937 randomA(99);
        std::mt19937 randomB(99);
        turn_based::CancelToken cancel;
        bool same = true;
        for (int step = 0; step < maximumMoves && !first->outcome().finished(); ++step) {
            const auto a = first->chooseComputerMove(randomA, cancel);
            const auto b = second->chooseComputerMove(randomB, cancel);
            same = same && a == b;
            if (!a || !b) break;
            first->play(*a);
            second->play(*b);
        }
        check(same, name + ": computer play is deterministic for a given seed");
    }

    {
        auto game = create();
        std::mt19937 random(5);
        turn_based::CancelToken cancel;
        cancel.cancel();
        check(!game->chooseComputerMove(random, cancel), name + ": a cancelled computer search returns no move");
    }
}

} // namespace turn_based_test
