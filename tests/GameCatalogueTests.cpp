// Registry integrity: the graphical catalogue is internally consistent, uses the persistence
// game keys, and describes exactly the 14 games of the console application (read from
// XO_Demo.cpp itself) plus Ping Pong.
#include "GameCatalogue.hpp"
#include "MatchTypes.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifndef GVA_CONSOLE_MENU_SOURCE
#error "GVA_CONSOLE_MENU_SOURCE must point at XO_Demo.cpp"
#endif

namespace {
int failures = 0;
int checks = 0;

void check(bool condition, const std::string& message)
{
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::string lower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return text;
}

bool trimmed(const std::string& text)
{
    return !text.empty() && !std::isspace(static_cast<unsigned char>(text.front())) &&
           !std::isspace(static_cast<unsigned char>(text.back()));
}

std::string readConsoleSource()
{
    std::ifstream input(GVA_CONSOLE_MENU_SOURCE, std::ios::binary);
    std::ostringstream content;
    content << input.rdbuf();
    return content.str();
}

// Returns the text of the first function body that follows `signature`.
std::string functionBody(const std::string& source, const std::string& signature)
{
    const auto start = source.find(signature);
    if (start == std::string::npos) return {};
    const auto open = source.find('{', start);
    if (open == std::string::npos) return {};
    int depth = 0;
    for (std::size_t index = open; index < source.size(); ++index) {
        if (source[index] == '{') ++depth;
        else if (source[index] == '}' && --depth == 0) return source.substr(open, index - open + 1);
    }
    return {};
}

void testIdentityAndKeys()
{
    const auto& games = catalogue::all();
    check(games.size() == 15, "the graphical catalogue lists 15 games");
    check(std::count_if(games.begin(), games.end(),
                        [](const auto& game) { return game.category == catalogue::GameCategory::Board; }) == 14,
          "14 entries are board games");
    const auto* pong = catalogue::find("ping_pong");
    check(std::count_if(games.begin(), games.end(),
                        [](const auto& game) { return game.category == catalogue::GameCategory::Arcade; }) == 1 &&
              pong != nullptr && pong->category == catalogue::GameCategory::Arcade,
          "Ping Pong is the only arcade game");

    std::set<std::string> keys;
    std::set<std::string> names;
    const std::regex keyPattern("^[a-z0-9]+(_[a-z0-9]+)*$");
    for (const auto& game : games) {
        keys.insert(game.key);
        names.insert(lower(game.displayName));
        check(std::regex_match(game.key, keyPattern), "key '" + game.key + "' is a stable snake_case identifier");
        check(catalogue::find(game.key) == &game, "find() returns the entry for '" + game.key + "'");
        bool knownToPersistence = true;
        try {
            knownToPersistence = persistence::toStorage(persistence::gameKeyFromStorage(game.key)) == game.key;
        } catch (const std::exception&) {
            knownToPersistence = false;
        }
        check(knownToPersistence, "key '" + game.key + "' is a persistence game key");
    }
    check(keys.size() == games.size(), "every key is unique");
    check(names.size() == games.size(), "every display name is unique regardless of case");

    std::set<std::string> persistenceKeys;
    for (const auto key : persistence::allGameKeys()) persistenceKeys.insert(persistence::toStorage(key));
    check(persistenceKeys == keys, "catalogue keys and persistence game keys are the same set");
    check(catalogue::find("chess") == nullptr, "unknown keys are not found");
}

void testDescriptorsAreComplete()
{
    for (const auto& game : catalogue::all()) {
        const std::string name = game.displayName;
        check(trimmed(game.displayName) && game.displayName.size() <= 28, name + " has a valid display name");
        check(trimmed(game.shortDescription) && game.shortDescription.size() <= 60,
              name + " has a one-line description");
        check(trimmed(game.rules) && game.rules.size() >= 80, name + " has rules text");
        check(trimmed(game.boardSummary), name + " has a board summary");
        check(!game.firstSeatLabel.empty() && !game.secondSeatLabel.empty(), name + " names both seats");
        check(game.humanVsHuman || game.humanVsComputer, name + " supports at least one player mode");
        check(!game.humanVsComputer || trimmed(game.computerStrategy),
              name + " describes its computer opponent truthfully");
        check(!game.selectableDifficulty || game.humanVsComputer, name + ": difficulty implies a computer mode");
        check(!game.tournamentEligible || game.humanVsHuman,
              name + ": tournament games must support local human-vs-human play");
        check(game.historyEligible, name + " records completed matches");
        check((game.launch == catalogue::LaunchKind::BoardGame) == static_cast<bool>(game.createGame),
              name + ": a game factory exists exactly for shared board-game launches");
        const bool dedicated = game.key == "classic_tic_tac_toe" || game.key == "ping_pong";
        check((game.launch == catalogue::LaunchKind::DedicatedScene) == dedicated,
              name + ": only Classic Tic-Tac-Toe and Ping Pong use dedicated scenes");
    }
}

// Every game the library calls playable must have a real way to start: a dedicated scene the
// GUI binds (Classic Tic-Tac-Toe and Ping Pong, the same two keys Application.cpp binds and
// verifies at startup) or a board-game factory that creates a playable game.
void testEveryPlayableGameCanStart()
{
    const std::set<std::string> dedicatedScenes{"classic_tic_tac_toe", "ping_pong"};
    std::size_t launchable = 0;
    std::size_t factories = 0;
    std::mt19937 random(7);
    for (const auto& game : catalogue::all()) {
        const bool hasDedicatedScene =
            game.launch == catalogue::LaunchKind::DedicatedScene && dedicatedScenes.count(game.key) == 1;
        const bool hasFactory = game.launch == catalogue::LaunchKind::BoardGame && static_cast<bool>(game.createGame);
        check(game.playableInGui() == (hasDedicatedScene || hasFactory),
              game.displayName + " is playable exactly when it has a scene or a factory");
        if (hasDedicatedScene || hasFactory) ++launchable;
        if (!game.createGame) continue;
        ++factories;
        auto instance = game.createGame(1234u);
        check(instance != nullptr, game.displayName + " factory creates a game");
        if (!instance) continue;
        check(instance->currentSeat() == turn_based::Seat::First, game.displayName + " starts with the first seat");
        check(!instance->outcome().finished() && !instance->legalMoves().empty(),
              game.displayName + " starts in progress with legal moves");
        turn_based::CancelToken cancel;
        const auto move = instance->chooseComputerMove(random, cancel);
        check(move && instance->isLegal(*move), game.displayName + " computer returns a legal opening move");
    }
    check(launchable == catalogue::playableCount(), "every playable game has a launch path and vice versa");
    check(launchable >= dedicatedScenes.size(), "at least the two dedicated games can start");
    check(factories + dedicatedScenes.size() == launchable, "every non-dedicated playable game has a factory");
}

void testConsoleCatalogueMatches()
{
    const std::string source = readConsoleSource();
    check(!source.empty(), "XO_Demo.cpp can be read");
    const std::string menu = functionBody(source, "void show_games_menu()");
    const std::regex entryPattern(R"re(cout << "(\d+)\.\s+([^"]*?)\\n";)re");
    std::map<int, std::string> consoleMenu;
    for (std::sregex_iterator it(menu.begin(), menu.end(), entryPattern), end; it != end; ++it) {
        const int number = std::stoi((*it)[1].str());
        if (number != 0) consoleMenu[number] = (*it)[2].str();
    }
    check(consoleMenu.size() == 14, "the console games menu lists 14 games");

    const std::string dispatcher = functionBody(source, "MatchResult play_selected_game(");
    std::map<int, const catalogue::GameDescriptor*> byNumber;
    for (const auto& game : catalogue::all()) {
        if (game.consoleMenuNumber == 0) continue;
        byNumber[game.consoleMenuNumber] = &game;
    }
    check(byNumber.size() == 14, "exactly 14 catalogue entries carry a console menu number");
    const auto* pong = catalogue::find("ping_pong");
    check(pong != nullptr && pong->consoleMenuNumber == 0, "Ping Pong is not part of the console catalogue");
    const auto& games = catalogue::all();
    for (std::size_t index = 0; index < 14 && index < games.size(); ++index) {
        check(games[index].consoleMenuNumber == static_cast<int>(index) + 1,
              games[index].displayName + " keeps its console menu position in the library order");
    }

    // Seat labels follow the symbols the console assigns: SUS gives Player 1 'S' and Player 2 'U';
    // the standard games give 'X' and 'O'.
    const std::string sus = functionBody(source, "MatchResult run_sus_game_with_result(");
    const auto* susGame = catalogue::find("sus");
    check(susGame != nullptr && sus.find("create_char_players(ui, score, 'S', 'U')") != std::string::npos &&
              susGame->firstSeatLabel == "S" && susGame->secondSeatLabel == "U",
          "SUS seat labels match the console's S (Player 1) and U (Player 2)");
    const std::string standard = functionBody(source, "MatchResult run_standard_char_game(");
    const auto* misere = catalogue::find("misere_tic_tac_toe");
    check(misere != nullptr && standard.find("create_char_players(ui, score, 'X', 'O')") != std::string::npos &&
              misere->firstSeatLabel == "X" && misere->secondSeatLabel == "O",
          "standard board games use the console's X (Player 1) and O (Player 2)");

    for (const auto& [number, label] : consoleMenu) {
        const auto found = byNumber.find(number);
        if (found == byNumber.end()) {
            check(false, "console menu entry " + std::to_string(number) + " has a graphical entry");
            continue;
        }
        const auto& game = *found->second;
        check(game.consoleMenuLabel == label,
              "console entry " + std::to_string(number) + " '" + label + "' matches " + game.displayName);

        // Follow play_selected_game's case to the legacy board class it constructs.
        const std::regex casePattern("case " + std::to_string(number) + R"(:\s*return ([^;]+);)");
        std::smatch match;
        bool classMatches = false;
        if (std::regex_search(dispatcher, match, casePattern)) {
            const std::string statement = match[1].str();
            if (statement.find("<" + game.consoleBoardClass + ",") != std::string::npos) {
                classMatches = true;
            } else {
                const auto paren = statement.find('(');
                const std::string helper = statement.substr(0, paren);
                const std::string body = functionBody(source, "MatchResult " + helper + "(");
                classMatches = body.find("new " + game.consoleBoardClass + "(") != std::string::npos;
            }
        }
        check(classMatches, game.displayName + " maps to the console board class " + game.consoleBoardClass);
    }
}

void testSearchAndFilter()
{
    using catalogue::GameCategory;
    check(catalogue::filter("", std::nullopt).size() == 15, "an empty search shows every game");
    check(catalogue::filter("   ", std::nullopt).size() == 15, "a blank search shows every game");
    check(catalogue::filter("", GameCategory::Board).size() == 14, "the Board filter shows the 14 board games");
    check(catalogue::filter("", GameCategory::Arcade).size() == 1, "the Arcade filter shows Ping Pong");
    const auto pong = catalogue::filter("PiNg", std::nullopt);
    check(pong.size() == 1 && pong.front()->key == "ping_pong", "search ignores letter case");
    check(catalogue::filter(" xo ", std::nullopt).size() == 3, "search trims spaces and matches part of a name");
    check(catalogue::filter("tic-tac-toe", GameCategory::Board).size() == 8,
          "search and category combine (eight games are named Tic-Tac-Toe)");
    check(catalogue::filter("tic-tac-toe", GameCategory::Arcade).empty(), "a category can exclude every match");
    check(catalogue::filter("zzzz", std::nullopt).empty(), "a search with no match returns nothing");
    const auto ordered = catalogue::filter("", std::nullopt);
    check(ordered.front()->key == "classic_tic_tac_toe" && ordered.back()->key == "ping_pong",
          "the library keeps console order with Ping Pong last");
}

void testPlayableCount()
{
    const auto& games = catalogue::all();
    const auto playable = static_cast<std::size_t>(
        std::count_if(games.begin(), games.end(), [](const auto& game) { return game.playableInGui(); }));
    check(catalogue::playableCount() == playable, "playableCount matches the entries");
    const auto* classic = catalogue::find("classic_tic_tac_toe");
    const auto* pong = catalogue::find("ping_pong");
    check(classic != nullptr && pong != nullptr && classic->playableInGui() && pong->playableInGui(),
          "Classic Tic-Tac-Toe and Ping Pong are playable in the GUI");
}
} // namespace

int main()
{
    testIdentityAndKeys();
    testDescriptorsAreComplete();
    testEveryPlayableGameCanStart();
    testConsoleCatalogueMatches();
    testSearchAndFilter();
    testPlayableCount();

    if (failures == 0) {
        std::cout << "Game catalogue tests passed: " << checks << " checks (" << catalogue::playableCount()
                  << " of " << catalogue::all().size() << " games playable in the GUI)\n";
        return 0;
    }
    std::cerr << failures << " of " << checks << " catalogue checks failed\n";
    return 1;
}
