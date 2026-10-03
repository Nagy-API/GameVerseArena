#include "TurnBasedTestSupport.hpp"
#include "WordGame.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <string>

#ifndef GVA_WORD_DICTIONARY_SOURCE
#error "GVA_WORD_DICTIONARY_SOURCE must point at the console's dic.txt"
#endif

using turn_based::Seat;
using turn_based_test::check;
using word_ttt::WordGame;

namespace {
void play(WordGame& game, int cell, char letter)
{
    check(game.play(WordGame::encode(cell, letter)),
          std::string("setup move ") + letter + " at cell " + std::to_string(cell) + " is legal");
}

void testDictionaryMatchesConsole()
{
    // The console reads dic.txt at run time, keeping three-letter words in upper case.
    std::ifstream input(GVA_WORD_DICTIONARY_SOURCE);
    check(static_cast<bool>(input), "the console dictionary dic.txt can be read");
    std::set<std::string> console;
    std::string word;
    while (input >> word) {
        std::transform(word.begin(), word.end(), word.begin(),
                       [](unsigned char character) { return static_cast<char>(std::toupper(character)); });
        if (word.size() == 3) console.insert(word);
    }
    check(!console.empty() && console == WordGame::dictionary(), "the built-in words are exactly the console's dic.txt");
}

void testRules()
{
    WordGame game;
    check(game.legalMoves().size() == 9 * 26, "any letter may go in any cell at the start");
    play(game, 4, 'Q');
    check(game.cell(4) == 'Q' && game.currentSeat() == Seat::Second, "a letter is written and the turn passes");
    check(!game.play(WordGame::encode(4, 'A')), "an occupied cell is illegal");
    check(!game.play(-1) && !game.play(9 * 26), "moves outside the grid are illegal");
    check(WordGame::cellOf(WordGame::encode(7, 'Z')) == 7 && WordGame::letterOf(WordGame::encode(7, 'Z')) == 'Z',
          "moves encode a cell and a letter");
}

void testWords()
{
    WordGame row;
    play(row, 0, 'C');
    play(row, 8, 'Q');
    play(row, 1, 'A');
    play(row, 7, 'Z');
    play(row, 2, 'T');
    check(row.outcome().winner == Seat::First && row.winningWord() == "CAT" &&
              row.winningLine() == WordGame::Line{0, 1, 2},
          "a word read left to right along a row wins");

    WordGame secondPlayer;
    play(secondPlayer, 0, 'D');
    play(secondPlayer, 1, 'O');
    play(secondPlayer, 8, 'Z');
    play(secondPlayer, 2, 'G');
    check(secondPlayer.outcome().winner == Seat::Second && secondPlayer.winningWord() == "DOG",
          "the player whose letter completes the word wins, even the second player");

    WordGame column;
    play(column, 0, 'S');
    play(column, 8, 'Q');
    play(column, 3, 'U');
    play(column, 7, 'Z');
    play(column, 6, 'N');
    check(column.outcome().winner == Seat::First && column.winningWord() == "SUN", "a word read down a column wins");

    WordGame antiDiagonal;
    play(antiDiagonal, 2, 'T');
    play(antiDiagonal, 4, 'W');
    play(antiDiagonal, 6, 'O');
    check(antiDiagonal.outcome().winner == Seat::First && antiDiagonal.winningWord() == "TWO",
          "a word read downward along the rising diagonal wins");

    WordGame backwards;
    play(backwards, 0, 'T');
    play(backwards, 1, 'A');
    play(backwards, 2, 'C');
    check(!backwards.outcome().finished(), "a word spelled backwards (right to left) does not count");
}

void testDraw()
{
    WordGame game;
    for (int cell = 0; cell < 9; ++cell) play(game, cell, cell % 2 == 0 ? 'Q' : 'Z');
    check(game.outcome().status == turn_based::OutcomeStatus::Draw && !game.winningLine(),
          "nine letters without a word are a draw");
    game.reset();
    check(game.movesPlayed() == 0 && game.cell(0) == '.', "reset clears the grid");
}

void testComputer()
{
    std::mt19937 random(5);
    turn_based::CancelToken cancel;

    // C A _ : the first letter (A-Z) that completes a word at cell 2 is R (CAR), before T (CAT).
    WordGame win;
    play(win, 0, 'C');
    play(win, 8, 'Q');
    play(win, 1, 'A');
    play(win, 7, 'Z');
    check(win.chooseComputerMove(random, cancel) == WordGame::encode(2, 'R'),
          "the computer completes a word with the first winning letter");

    // Cells are scanned before letters: B _ D (cell 1 makes BED) comes before C A _ (cell 8).
    WordGame firstCell;
    play(firstCell, 0, 'B');
    play(firstCell, 6, 'C');
    play(firstCell, 2, 'D');
    play(firstCell, 7, 'A');
    check(firstCell.chooseComputerMove(random, cancel) == WordGame::encode(1, 'E'),
          "the computer takes the first cell (row by row) where a letter wins");

    // Corners are tried in the console's order: 0, 2, 6, then 8.
    WordGame corners;
    play(corners, 4, 'Q');
    play(corners, 0, 'Q');
    auto choice = corners.chooseComputerMove(random, cancel);
    check(choice && WordGame::cellOf(*choice) == 2, "with corner 0 taken it writes in corner 2");
    play(corners, 2, 'Q');
    choice = corners.chooseComputerMove(random, cancel);
    check(choice && WordGame::cellOf(*choice) == 6, "then corner 6");
    play(corners, 6, 'Q');
    choice = corners.chooseComputerMove(random, cancel);
    check(choice && WordGame::cellOf(*choice) == 8, "then corner 8");

    WordGame empty;
    const auto opening = empty.chooseComputerMove(random, cancel);
    check(opening && WordGame::cellOf(*opening) == 4, "with no word to make it writes a random letter in the centre");

    WordGame corner;
    play(corner, 4, 'Q');
    const auto second = corner.chooseComputerMove(random, cancel);
    check(second && WordGame::cellOf(*second) == 0, "with the centre taken it writes in the first free corner");

    WordGame edges;
    for (const int cell : {4, 0, 2, 6, 8}) play(edges, cell, 'Q');
    for (int attempt = 0; attempt < 20; ++attempt) {
        const auto move = edges.chooseComputerMove(random, cancel);
        const int cell = move ? WordGame::cellOf(*move) : -1;
        check(move && edges.isLegal(*move) && (cell == 1 || cell == 3 || cell == 5 || cell == 7),
              "with centre and corners full it writes in a random free cell");
    }
}
} // namespace

int main()
{
    testDictionaryMatchesConsole();
    testRules();
    testWords();
    testDraw();
    testComputer();
    turn_based_test::checkContract("Word", [] { return std::make_unique<WordGame>(); }, 9);
    return turn_based_test::finish("Word Tic-Tac-Toe");
}
