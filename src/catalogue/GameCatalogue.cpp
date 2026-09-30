#include "GameCatalogue.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace catalogue {
namespace {

std::string lowercase(std::string_view text)
{
    std::string result(text);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return result;
}

GameDescriptor board(std::string key, std::string name, std::string summary, std::string description,
                     std::string rules, std::string computer, std::string firstSeat, std::string secondSeat,
                     int menuNumber, std::string menuLabel, std::string boardClass)
{
    GameDescriptor game;
    game.key = std::move(key);
    game.displayName = std::move(name);
    game.boardSummary = std::move(summary);
    game.shortDescription = std::move(description);
    game.rules = std::move(rules);
    game.category = GameCategory::Board;
    game.computerStrategy = std::move(computer);
    game.firstSeatLabel = std::move(firstSeat);
    game.secondSeatLabel = std::move(secondSeat);
    game.consoleMenuNumber = menuNumber;
    game.consoleMenuLabel = std::move(menuLabel);
    game.consoleBoardClass = std::move(boardClass);
    game.launch = LaunchKind::ConsoleOnly;
    return game;
}

std::vector<GameDescriptor> buildCatalogue()
{
    std::vector<GameDescriptor> games;

    auto classic = board(
        "classic_tic_tac_toe", "Classic Tic-Tac-Toe", "3x3", "Line up three marks on a 3x3 grid.",
        "Players alternate placing X and O on a 3x3 grid; X always moves first. Three marks in a row, column, "
        "or diagonal win. A full grid without a line is a draw. Play a single game or a best-of-3 or best-of-5 "
        "match.",
        "Choose Easy (random legal move), Medium (win, block, then center and corners), or Hard (unbeatable "
        "alpha-beta minimax).",
        "X", "O", 1, "Play X-O (Tic-Tac-Toe)", "X_O_Board");
    classic.selectableDifficulty = true;
    classic.launch = LaunchKind::DedicatedScene;
    games.push_back(std::move(classic));

    games.push_back(board(
        "numerical_tic_tac_toe", "Numerical Tic-Tac-Toe", "3x3", "Make any line add up to exactly 15.",
        "Player 1 places the odd numbers 1, 3, 5, 7, and 9; Player 2 places the even numbers 2, 4, 6, and 8. Each "
        "number can be used only once. Whoever completes a full row, column, or diagonal whose three numbers add "
        "up to exactly 15 wins. Nine placements without such a line is a draw.",
        "Plays a random legal move: a random empty cell with one of its unused numbers, as the console game does.",
        "Odd numbers", "Even numbers", 2, "Play Numerical Tic-Tac-Toe", "NumericalTTT_Board"));

    games.push_back(board(
        "sus", "SUS", "3x3", "Spell S-U-S in lines to score points.",
        "Player 1 always places S and Player 2 always places U, starting with Player 1. Each S-U-S line (row, "
        "column, or diagonal) completed by the letter just placed scores one point for the player who placed it. "
        "When all nine cells are filled, the higher score wins; equal scores draw.",
        "Uses the console game's scoring heuristic: prefers cells that complete S-U-S lines or block the "
        "opponent's, then the center and corners.",
        "S", "U", 3, "Play SUS", "SUS_Board"));

    games.push_back(board(
        "five_by_five_tic_tac_toe", "5x5 Tic-Tac-Toe", "5x5", "Score the most three-in-a-rows on a 5x5 board.",
        "X and O alternate on a 5x5 grid, X first. The game ends after 24 moves, leaving one cell empty. Every run "
        "of three of the same mark in a row, column, or diagonal scores a point, and overlapping runs count "
        "separately. The higher score wins; equal scores draw.",
        "The console game's priority heuristic: look for a cell that leaves it holding a three-in-a-row, then one "
        "that would leave the opponent holding one, then take the center, a corner, or the first free cell. It "
        "checks whole-board totals, so once either player holds any three it simply takes the first free cell.",
        "X", "O", 4, "Play 5x5 Tic Tac Toe", "Five_TTT_Board"));

    games.push_back(board(
        "misere_tic_tac_toe", "Misere Tic-Tac-Toe", "3x3", "Avoid completing three in a row.",
        "X and O alternate on a 3x3 grid, X first. Whoever completes a line of three of their own marks loses. A "
        "full grid without a line is a draw.",
        "Full-depth minimax search, as in the console game: it never completes a line if it can avoid it.",
        "X", "O", 5, "Play Misere", "MISERE_Board"));

    games.push_back(board(
        "four_in_a_row", "Four-in-a-Row", "6x7 drop", "Drop discs and connect four.",
        "Players drop X and O discs into a grid of 6 rows and 7 columns, X first; a disc falls to the lowest empty "
        "cell of its column. Four in a row horizontally, vertically, or diagonally wins. A full grid is a draw.",
        "The console game's strategy: opens in the center column, takes an immediate win, creates a double "
        "threat, blocks an immediate win, blocks a double threat, then searches deeper with alpha-beta pruning "
        "for up to 1.5 seconds.",
        "X", "O", 6, "Play Four-in-a-Row", "FourInRow_Board"));

    games.push_back(board(
        "four_by_four_tic_tac_toe", "4x4 Tic-Tac-Toe", "4x4 slide", "Slide your tokens to make three in a row.",
        "Each player starts with four tokens: the top row reads O X O X and the bottom row X O X O. X moves first. "
        "On your turn, slide one of your tokens one cell up, down, left, or right into an empty cell. Three of your "
        "tokens in a row, column, or diagonal win.",
        "Three-move minimax lookahead with the console game's line evaluation.",
        "X", "O", 7, "Play 4x4 Tic-Tac-Toe", "T4x4_Board"));

    games.push_back(board(
        "word_tic_tac_toe", "Word Tic-Tac-Toe", "3x3 letters", "Complete a three-letter word to win.",
        "Players take turns writing any letter A-Z in an empty cell of a 3x3 grid. Whoever completes a valid "
        "three-letter word, read left to right along a row, top to bottom along a column, or downward along a "
        "diagonal, wins. Words come from the bundled dictionary file dic.txt. Nine letters without a word is a draw.",
        "The console game's strategy: plays a winning letter when one exists; otherwise a random letter in the "
        "center, then in a corner, then in any free cell.",
        "Any letter", "Any letter", 8, "Play Word Tic-Tac-Toe", "Word_Tic_Tac_Toe_Board"));

    games.push_back(board(
        "pyramid_tic_tac_toe", "Pyramid Tic-Tac-Toe", "pyramid", "Three in a line on a nine-cell pyramid.",
        "The board is a pyramid of 5, 3, and 1 cells. X and O alternate, X first. Three marks in a line win: along "
        "the bottom row (three possible lines), across the middle row, straight up the center, or along either "
        "sloped edge. A full pyramid without a line is a draw.",
        "The console game's strategy: win if possible, otherwise block, otherwise take the first free cell from "
        "the bottom row up.",
        "X", "O", 9, "Play Pyramid Tic-Tac-Toe", "PyramidBoard"));

    games.push_back(board(
        "diamond", "Diamond", "diamond", "Make a line of three and a line of four at once.",
        "X and O alternate on a diamond-shaped board of 25 cells, X first. You win by holding a line of exactly "
        "three and a line of exactly four of your marks at the same time, in different directions (horizontal, "
        "vertical, or either diagonal); the two lines may share a cell. A full board without a winner is a draw.",
        "The console game's strategy: win if possible, block the opponent's winning cell, otherwise choose the "
        "cell that most strengthens its own lines, preferring the center.",
        "X", "O", 10, "Play Diamond", "DIAMOND_Board"));

    games.push_back(board(
        "infinity_xo", "Infinity XO", "3x3", "The oldest mark vanishes as you play.",
        "X and O alternate on a 3x3 grid, X first. After the 6th move the oldest mark on the board disappears, and "
        "after the 9th move the next-oldest disappears too. Three in a row wins. If the 9th move does not win, the "
        "game is a draw.",
        "Plays a random legal move, as the console game does.",
        "X", "O", 11, "Play Infinity XO", "Infinity_XO_Board"));

    games.push_back(board(
        "ultimate_xo", "Ultimate XO", "9 boards", "Win small boards to claim the big board.",
        "The board holds nine small tic-tac-toe boards. Winning a small board claims that square of the big board; "
        "a full small board without a line becomes a tie square. After a move, the next player must play in the "
        "same small board while it is still open; once it is won or full, the next player may choose any open "
        "board. Three claimed squares in a line on the big board win. If every small board closes without such a "
        "line, the game is a draw.",
        "Alpha-beta minimax four moves deep with the console game's board evaluation.",
        "X", "O", 12, "Play Ultimate XO", "UltimateTTT_Board"));

    games.push_back(board(
        "memory_xo", "Memory XO", "3x3 hidden", "Every mark is hidden, so remember where they are.",
        "X and O alternate on a 3x3 grid, X first, but each mark is hidden as soon as it is placed. Choosing a cell "
        "that is already taken is refused and you must choose again. Three in a row wins. Nine marks without a "
        "line is a draw.",
        "The console game's strategy: it always takes the first free cell, reading the hidden board.",
        "X", "O", 13, "Play Memory XO", "Memory_XO_Board"));

    games.push_back(board(
        "obstacle_tic_tac_toe", "Obstacle Tic-Tac-Toe", "6x6", "Four in a row while obstacles appear.",
        "X and O alternate on a 6x6 grid, X first. After every second move, two obstacles appear on random empty "
        "cells and can never be used. Four in a row horizontally, vertically, or diagonally wins.",
        "Plays a random legal move, as the console game does.",
        "X", "O", 14, "Play Obstacle", "Obstacle_Board"));

    GameDescriptor pong;
    pong.key = "ping_pong";
    pong.displayName = "Ping Pong";
    pong.boardSummary = "real time";
    pong.shortDescription = "Real-time paddles, first to five points.";
    pong.rules =
        "Real-time arcade game. Each player moves a paddle to return the ball, which speeds up with every return. "
        "When the ball gets past your paddle, your opponent scores. The first player to 5 points wins.";
    pong.category = GameCategory::Arcade;
    pong.computerStrategy = "Choose Easy, Medium, or Hard: the computer reacts faster, moves faster, and aims more "
                            "precisely at higher levels.";
    pong.selectableDifficulty = true;
    pong.firstSeatLabel = "Left paddle";
    pong.secondSeatLabel = "Right paddle";
    pong.launch = LaunchKind::DedicatedScene;
    games.push_back(std::move(pong));

    return games;
}
} // namespace

const std::vector<GameDescriptor>& all()
{
    static const std::vector<GameDescriptor> games = buildCatalogue();
    return games;
}

const GameDescriptor* find(std::string_view key)
{
    for (const auto& game : all()) {
        if (game.key == key) return &game;
    }
    return nullptr;
}

bool matchesSearch(const GameDescriptor& game, std::string_view query)
{
    std::string needle = lowercase(query);
    const auto first = needle.find_first_not_of(' ');
    if (first == std::string::npos) return true;
    needle = needle.substr(first, needle.find_last_not_of(' ') - first + 1);
    return lowercase(game.displayName).find(needle) != std::string::npos;
}

std::vector<const GameDescriptor*> filter(std::string_view query, std::optional<GameCategory> category)
{
    std::vector<const GameDescriptor*> result;
    for (const auto& game : all()) {
        if (category && game.category != *category) continue;
        if (!matchesSearch(game, query)) continue;
        result.push_back(&game);
    }
    return result;
}

std::size_t playableCount()
{
    return static_cast<std::size_t>(
        std::count_if(all().begin(), all().end(), [](const GameDescriptor& game) { return game.playableInGui(); }));
}

} // namespace catalogue
