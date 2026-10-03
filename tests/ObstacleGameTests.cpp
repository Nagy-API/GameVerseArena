#include "ObstacleGame.hpp"
#include "TurnBasedTestSupport.hpp"

#include <algorithm>
#include <set>

using obstacle::Cell;
using obstacle::ObstacleGame;
using turn_based::Seat;
using turn_based_test::check;

namespace {
int count(const ObstacleGame& game, Cell value)
{
    int total = 0;
    for (int index = 0; index < 36; ++index) {
        if (game.cell(index) == value) ++total;
    }
    return total;
}

void testObstacles()
{
    ObstacleGame game(11);
    check(game.legalMoves().size() == 36, "all 36 cells are open at the start");
    check(game.play(0) && count(game, Cell::Blocked) == 0, "no obstacle after the first move");
    check(game.play(35) && count(game, Cell::Blocked) == 2 && game.lastObstacles().size() == 2,
          "two obstacles appear after every second move");
    const auto firstObstacles = game.lastObstacles();
    for (const int cell : firstObstacles) {
        check(cell != 0 && cell != 35 && game.cell(cell) == Cell::Blocked && !game.isLegal(cell),
              "obstacles fall on empty cells and can never be used");
    }
    const auto moves = game.legalMoves();
    check(game.play(moves.front()) && count(game, Cell::Blocked) == 2 && game.lastObstacles().empty(),
          "the third move adds no obstacle");
    check(game.play(game.legalMoves().front()) && count(game, Cell::Blocked) == 4, "the fourth move adds two more");
    check(count(game, Cell::X) == 2 && count(game, Cell::O) == 2, "obstacles never replace a mark");

    ObstacleGame same(11);
    same.play(0);
    same.play(35);
    check(same.lastObstacles() == firstObstacles, "the same seed places the same obstacles");
    std::set<std::vector<int>> patterns;
    for (std::uint32_t seed = 1; seed <= 10; ++seed) {
        ObstacleGame seeded(seed);
        seeded.play(0);
        seeded.play(35);
        patterns.insert(seeded.lastObstacles());
    }
    check(patterns.size() > 1, "different seeds place different obstacles");
    game.reset();
    check(count(game, Cell::Blocked) == 0 && game.movesPlayed() == 0, "reset clears marks and obstacles");
}

// Finds a seed whose obstacles leave the scripted cells free, plays X's four cells (in the given
// order) against O's three, and checks that the last X move wins with `line`.
void expectWin(const std::vector<int>& xCells, const std::vector<int>& oCells, const ObstacleGame::Line& line,
               const std::string& name)
{
    bool tested = false;
    for (std::uint32_t seed = 1; seed <= 300 && !tested; ++seed) {
        ObstacleGame game(seed);
        bool blocked = false;
        for (std::size_t step = 0; step < 7 && !blocked; ++step) {
            const int cell = step % 2 == 0 ? xCells[step / 2] : oCells[step / 2];
            blocked = !game.play(cell);
        }
        if (blocked) continue;
        tested = true;
        check(game.outcome().winner == Seat::First && game.winningLine() && *game.winningLine() == line, name + " wins");
    }
    check(tested, "a seed leaves the scripted cells free for " + name);
}

void testWin()
{
    expectWin({0, 1, 2, 3}, {30, 31, 32}, {0, 1, 2, 3}, "four across a row");
    expectWin({0, 6, 12, 18}, {5, 11, 17}, {0, 6, 12, 18}, "four down a column");
    expectWin({0, 7, 14, 21}, {5, 11, 17}, {0, 7, 14, 21}, "four along a falling diagonal");
    expectWin({5, 10, 15, 20}, {0, 6, 12}, {5, 10, 15, 20}, "four along a rising diagonal");
    expectWin({0, 1, 3, 2}, {30, 31, 32}, {0, 1, 2, 3}, "a four completed in the middle");
}

void testFullBoardIsADraw()
{
    // Play many seeded random games to the end: a game without a winner ends only when no empty
    // cell is left (the console waited forever there).
    int draws = 0;
    for (std::uint32_t seed = 1; seed <= 150; ++seed) {
        ObstacleGame game(seed);
        std::mt19937 random(seed);
        while (!game.outcome().finished()) {
            const auto moves = game.legalMoves();
            std::uniform_int_distribution<std::size_t> pick(0, moves.size() - 1);
            game.play(moves[pick(random)]);
        }
        if (game.outcome().status == turn_based::OutcomeStatus::Draw) {
            ++draws;
            check(count(game, Cell::Empty) == 0, "a drawn game has no empty cell left");
        }
        check(game.movesPlayed() < 36, "obstacles end every game before 36 moves");
    }
    check(draws > 0, "some random games end in a full-board draw");
}

void testComputer()
{
    turn_based::CancelToken cancel;
    std::set<turn_based::MoveId> seen;
    for (std::uint32_t seed = 1; seed <= 200; ++seed) {
        ObstacleGame game(3);
        std::mt19937 random(seed);
        const auto move = game.chooseComputerMove(random, cancel);
        check(move && game.isLegal(*move), "the computer plays a legal cell");
        if (move) seen.insert(*move);
    }
    check(seen.size() > 25, "the computer's random choice spreads over the grid");
}
} // namespace

int main()
{
    testObstacles();
    testWin();
    testFullBoardIsADraw();
    testComputer();
    turn_based_test::checkContract("Obstacle", [] { return std::make_unique<ObstacleGame>(7); }, 36);
    return turn_based_test::finish("Obstacle Tic-Tac-Toe");
}
