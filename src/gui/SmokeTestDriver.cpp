#include "SmokeTestDriver.hpp"

#include "Application.hpp"
#include "AudioEngine.hpp"
#include "BoardGameScene.hpp"
#include "BoardGameSetupScene.hpp"
#include "CellBoardView.hpp"
#include "GameLibraryScene.hpp"
#include "NumericalGame.hpp"
#include "PingPongGameScene.hpp"

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <system_error>
#include <utility>

namespace {
constexpr float frameSeconds = 1.f / 60.f;

// Logical-space centres of shared shell controls (the 1280 x 720 design canvas).
sf::Vector2f mainMenuButton(int index) { return {1000.f, 206.f + static_cast<float>(index) * 78.f}; }
} // namespace

SmokeTestDriver::SmokeTestDriver(Application& application, std::filesystem::path outputDirectory)
    : app_(application), output_(std::move(outputDirectory))
{
    std::error_code error;
    std::filesystem::create_directories(output_, error);
    if (error) {
        std::cerr << "SMOKE FAIL: cannot create output directory: " << error.message() << '\n';
        ++failures_;
    }
    log_.open(output_ / "smoke-test.log", std::ios::trunc);
    if (!log_) {
        std::cerr << "SMOKE FAIL: cannot write smoke-test.log in the output directory\n";
        ++failures_;
    }
}

int SmokeTestDriver::run()
{
    log_ << "GameVerseArena GUI smoke test\n";
    setWindowSize({1280u, 720u});
    expect(app_.audio_->available(), "audio runs on the silent device during the smoke test");
    expect(app_.audio_->voiceCount() == AudioEngine::voiceLimit, "the sound voice pool is bounded at 12 voices");
    scenarioMainMenu();
    scenarioSettings();
    scenarioAbout();
    scenarioProfiles();
    scenarioLibrary();
    scenarioTicTacToe();
    scenarioPingPong();
    scenarioBoardGames();

    expect(app_.audio_->voiceCount() == AudioEngine::voiceLimit, "the voice pool never grew while sounds played");
    log_ << "\nChecks: " << checks_ << "  Failures: " << failures_ << "  Captures: " << captures_ << '\n';
    std::cout << "GUI smoke test: " << checks_ << " checks, " << failures_ << " failures, " << captures_
              << " captures\n";
    return failures_ == 0 ? 0 : 1;
}

// ---------------------------------------------------------------------------------------
// Primitives
// ---------------------------------------------------------------------------------------

void SmokeTestDriver::pumpRealEvents()
{
    while (const auto event = app_.window_.pollEvent()) {
        if (event->is<sf::Event::Resized>() || event->is<sf::Event::Closed>()) app_.dispatch(*event);
    }
}

void SmokeTestDriver::frames(int count)
{
    for (int index = 0; index < count && app_.window_.isOpen(); ++index) {
        pumpRealEvents();
        app_.advanceFrame(sf::seconds(frameSeconds));
    }
}

void SmokeTestDriver::wait(float seconds)
{
    frames(static_cast<int>(std::ceil(seconds / frameSeconds)));
}

void SmokeTestDriver::key(sf::Keyboard::Key code, bool shift)
{
    sf::Event::KeyPressed pressed{};
    pressed.code = code;
    pressed.shift = shift;
    app_.dispatch(sf::Event(pressed));
    sf::Event::KeyReleased released{};
    released.code = code;
    released.shift = shift;
    app_.dispatch(sf::Event(released));
    frames(1);
}

void SmokeTestDriver::keyWithText(sf::Keyboard::Key code, char32_t character)
{
    // Real typing delivers KeyPressed, then TextEntered, then KeyReleased.
    sf::Event::KeyPressed pressed{};
    pressed.code = code;
    app_.dispatch(sf::Event(pressed));
    sf::Event::TextEntered entered{};
    entered.unicode = character;
    app_.dispatch(sf::Event(entered));
    sf::Event::KeyReleased released{};
    released.code = code;
    app_.dispatch(sf::Event(released));
    frames(1);
}

void SmokeTestDriver::type(const std::string& text)
{
    for (const char character : text) {
        sf::Event::TextEntered entered{};
        entered.unicode = static_cast<char32_t>(static_cast<unsigned char>(character));
        app_.dispatch(sf::Event(entered));
    }
    frames(1);
}

void SmokeTestDriver::move(sf::Vector2f logical)
{
    sf::Event::MouseMoved moved{};
    moved.position = app_.window_.mapCoordsToPixel(logical);
    app_.dispatch(sf::Event(moved));
}

void SmokeTestDriver::click(sf::Vector2f logical)
{
    move(logical);
    const auto pixel = app_.window_.mapCoordsToPixel(logical);
    sf::Event::MouseButtonPressed pressed{};
    pressed.button = sf::Mouse::Button::Left;
    pressed.position = pixel;
    app_.dispatch(sf::Event(pressed));
    sf::Event::MouseButtonReleased released{};
    released.button = sf::Mouse::Button::Left;
    released.position = pixel;
    app_.dispatch(sf::Event(released));
    frames(1);
}

void SmokeTestDriver::focusLost()
{
    app_.dispatch(sf::Event(sf::Event::FocusLost{}));
    frames(1);
}

void SmokeTestDriver::focusGained()
{
    app_.dispatch(sf::Event(sf::Event::FocusGained{}));
    frames(1);
}

void SmokeTestDriver::setWindowSize(sf::Vector2u size)
{
    app_.window_.setSize(size);
    frames(3);
    app_.updateView(app_.window_.getSize());
    app_.scenes_.active().onResize(app_.window_.getSize());
    frames(2);
}

void SmokeTestDriver::capture(const std::string& name)
{
    const auto size = app_.window_.getSize();
    std::ostringstream fileName;
    fileName << std::setw(2) << std::setfill('0') << ++captures_ << '_' << name << '_' << size.x << 'x' << size.y
             << ".png";
    const auto path = output_ / fileName.str();
    std::error_code error;
    std::filesystem::remove(path, error);  // A stale file from an earlier run must not count as a capture.
    pumpRealEvents();
    app_.advanceFrame(sf::seconds(frameSeconds), &path);
    const bool saved = std::filesystem::exists(path, error) && std::filesystem::file_size(path, error) > 0;
    expect(saved, "capture " + fileName.str() + " was written");
    log_ << "CAPTURE " << fileName.str() << '\n';
}

void SmokeTestDriver::captureBoth(const std::string& name)
{
    capture(name);
    setWindowSize({960u, 540u});
    capture(name);
    setWindowSize({1280u, 720u});
}

bool SmokeTestDriver::expect(bool condition, const std::string& message)
{
    ++checks_;
    if (!condition) ++failures_;
    log_ << (condition ? "PASS " : "FAIL ") << message << '\n';
    if (!condition) std::cerr << "SMOKE FAIL: " << message << '\n';
    return condition;
}

SceneId SmokeTestDriver::activeScene() const
{
    return app_.scenes_.activeId();
}

bool SmokeTestDriver::expectScene(SceneId id, const std::string& message)
{
    return expect(activeScene() == id, message);
}

std::int64_t SmokeTestDriver::historyCount()
{
    const auto active = app_.profileService_->activeProfile();
    return active ? app_.matchRepository_->count(active->id, {}) : -1;
}

void SmokeTestDriver::openLibrary()
{
    if (activeScene() != SceneId::GameLibrary) {
        goToMainMenu();
        click(mainMenuButton(0));
    }
    // The library keeps its search and scroll position between visits; start from the top.
    if (auto* library = dynamic_cast<GameLibraryScene*>(&app_.scenes_.active())) {
        if (!library->query().empty()) key(sf::Keyboard::Key::Escape);
        if (library->focusZone() == GameLibraryScene::Zone::Grid) key(sf::Keyboard::Key::Home);
    }
}

void SmokeTestDriver::clickLibraryCard(std::size_t visibleIndex)
{
    // Cards are laid out three per row from (72, 196) with 384 x 142 steps; 368 x 128 each.
    const float x = 72.f + static_cast<float>(visibleIndex % 3) * 384.f + 184.f;
    const float y = 196.f + static_cast<float>(visibleIndex / 3) * 142.f + 64.f;
    click({x, y});
}

void SmokeTestDriver::goToMainMenu()
{
    for (int attempt = 0; attempt < 6 && activeScene() != SceneId::MainMenu; ++attempt) {
        key(sf::Keyboard::Key::Escape);
    }
    expectScene(SceneId::MainMenu, "Escape navigation returns to the main menu");
}

// ---------------------------------------------------------------------------------------
// Scenarios
// ---------------------------------------------------------------------------------------

void SmokeTestDriver::scenarioMainMenu()
{
    log_ << "\n[Main menu]\n";
    frames(10);
    expectScene(SceneId::MainMenu, "application starts on the main menu");
    captureBoth("main_menu");
}

void SmokeTestDriver::scenarioSettings()
{
    log_ << "\n[Settings]\n";
    click(mainMenuButton(2));
    if (!expectScene(SceneId::Settings, "Settings opens from the main menu")) return;
    captureBoth("settings_defaults");

    auto& settings = *app_.settings_;
    const auto soundsBefore = app_.audio_->startedCount();
    key(sf::Keyboard::Key::Right);
    key(sf::Keyboard::Key::Right);
    expect(settings.current().masterVolume == 90, "Right twice raises master volume 80 -> 90 immediately");
    expect(app_.settingsService_->load().settings.masterVolume == 90, "master volume change is persisted");
    expect(app_.audio_->mix().master == 90, "master volume change reaches the audio mixer");
    expect(app_.audio_->startedCount() > soundsBefore, "changing a volume plays an audible preview");
    key(sf::Keyboard::Key::Left, true);
    expect(settings.current().masterVolume == 89, "Shift+Left makes a fine 1% adjustment");

    for (int step = 0; step < 5; ++step) key(sf::Keyboard::Key::Down);
    key(sf::Keyboard::Key::Enter);
    expect(settings.current().reducedMotion, "Enter on Reduced Motion turns it on");
    expect(app_.settingsService_->load().settings.reducedMotion, "Reduced Motion is persisted");
    captureBoth("settings_reduced_motion");

    // Mouse: click a quarter of the way along the UI Volume track (row 1).
    click({372.f + 330.f * 0.25f, 176.f + 68.f + 29.f});
    expect(settings.current().uiVolume == 25, "clicking the UI Volume track sets 25%");
    expect(app_.settingsService_->load().settings.uiVolume == 25, "mouse slider change is persisted on release");

    click({110.f, 176.f + 4.f * 68.f + 29.f});
    expect(settings.current().audioMuted && app_.audio_->mix().muted, "clicking Mute All mutes immediately");
    wait(0.1f);
    const auto mutedBefore = app_.audio_->startedCount();
    key(sf::Keyboard::Key::Up);
    key(sf::Keyboard::Key::Up);
    expect(app_.audio_->startedCount() == mutedBefore, "no sound starts while Mute All is on");
    expect(app_.settingsService_->load().settings.audioMuted, "Mute All is persisted");

    click({72.f + 130.f, 620.f + 27.f});
    expect(settings.current() == persistence::defaultSettings(), "Reset to Defaults restores every default");
    expect(app_.settingsService_->load().settings == persistence::defaultSettings(), "defaults are persisted");

    key(sf::Keyboard::Key::Escape);
    expectScene(SceneId::MainMenu, "Escape leaves Settings");
}

void SmokeTestDriver::scenarioAbout()
{
    log_ << "\n[About]\n";
    click(mainMenuButton(3));
    if (!expectScene(SceneId::About, "About opens from the main menu")) return;
    captureBoth("about");
    key(sf::Keyboard::Key::Escape);
    expectScene(SceneId::MainMenu, "Escape leaves About");
}

void SmokeTestDriver::scenarioProfiles()
{
    log_ << "\n[Profiles]\n";
    click(mainMenuButton(1));
    if (!expectScene(SceneId::Profiles, "Profiles opens from the main menu")) return;
    const auto before = app_.profileService_->listProfiles().size();
    click({890.f + 159.f, 160.f + 28.f});
    type("Smoke Tester");
    key(sf::Keyboard::Key::Enter);
    expect(app_.profileService_->listProfiles().size() == before + 1, "a profile can be created with the keyboard");

    click({890.f + 159.f, 160.f + 28.f});
    type("smoke tester");
    key(sf::Keyboard::Key::Enter);
    expect(app_.profileService_->listProfiles().size() == before + 1,
           "a case-insensitive duplicate name is rejected");
    captureBoth("profiles_duplicate_error");
    key(sf::Keyboard::Key::Escape);
    expectScene(SceneId::Profiles, "Escape closes the profile overlay before leaving the scene");
    captureBoth("profiles");
    key(sf::Keyboard::Key::Escape);
    expectScene(SceneId::MainMenu, "Escape leaves Profiles");
}

void SmokeTestDriver::scenarioLibrary()
{
    log_ << "\n[Game library]\n";
    click(mainMenuButton(0));
    if (!expectScene(SceneId::GameLibrary, "Play opens the game library")) return;
    auto* library = dynamic_cast<GameLibraryScene*>(&app_.scenes_.active());
    if (!expect(library != nullptr, "the active scene is the game library")) return;
    expect(library->resultCount() == 15, "the library lists all 15 games");
    captureBoth("library");

    type("xo");
    expect(library->query() == "xo" && library->resultCount() == 3, "typing searches by name (3 XO games)");
    captureBoth("library_search");
    key(sf::Keyboard::Key::Escape);
    expect(library->query().empty() && library->resultCount() == 15, "Escape clears the search first");
    expectScene(SceneId::GameLibrary, "clearing the search stays in the library");

    click({772.f + 70.f, 128.f + 24.f});
    expect(library->resultCount() == 1, "the Arcade filter shows only Ping Pong");
    click({632.f + 65.f, 128.f + 24.f});
    expect(library->resultCount() == 14, "the Board filter shows the 14 board games");
    click({512.f + 55.f, 128.f + 24.f});
    expect(library->resultCount() == 15, "the All filter shows every game");

    sf::Event::MouseWheelScrolled wheel{};
    wheel.delta = -1.f;
    wheel.position = app_.window_.mapCoordsToPixel({640.f, 400.f});
    app_.dispatch(sf::Event(wheel));
    app_.dispatch(sf::Event(wheel));
    app_.dispatch(sf::Event(wheel));
    frames(2);
    expect(library->firstVisibleRow() == 2, "the grid scrolls and stops at its last row");
    captureBoth("library_scrolled");
    wheel.delta = 1.f;
    app_.dispatch(sf::Event(wheel));
    app_.dispatch(sf::Event(wheel));
    frames(2);
    expect(library->firstVisibleRow() == 0, "the grid scrolls back to the top");

    // A board game that is not migrated yet must not start a scene and must explain why.
    type("ultimate");
    key(sf::Keyboard::Key::Enter);
    key(sf::Keyboard::Key::Enter);
    expectScene(SceneId::GameLibrary, "a console-only game does not open a scene");
    expect(library->hasMessage(), "choosing a console-only game explains that it is console-only for now");
    captureBoth("library_console_only");
    key(sf::Keyboard::Key::Escape);

    using Key = sf::Keyboard::Key;
    using Zone = GameLibraryScene::Zone;
    keyWithText(Key::T, U't');
    keyWithText(Key::I, U'i');
    keyWithText(Key::C, U'c');
    keyWithText(Key::Space, U' ');
    expect(library->query() == "tic " && activeScene() == SceneId::GameLibrary,
           "Space typed in the search box adds a space instead of launching a game");
    for (int count = 0; count < 4; ++count) key(Key::Backspace);
    expect(library->query().empty() && library->resultCount() == 15, "Backspace edits the search");

    expect(library->focusZone() == Zone::Search, "typing focuses the search box");
    key(Key::Tab);
    expect(library->focusZone() == Zone::Categories, "Tab moves from search to the filters");
    key(Key::Tab);
    expect(library->focusZone() == Zone::Grid, "Tab moves from the filters to the games");
    key(Key::Tab);
    expect(library->focusZone() == Zone::Back, "Tab moves from the games to Back");
    key(Key::Tab);
    expect(library->focusZone() == Zone::Search, "Tab wraps from Back to search");
    key(Key::Tab, true);
    expect(library->focusZone() == Zone::Back, "Shift+Tab moves backwards");
    key(Key::Tab, true);
    expect(library->focusZone() == Zone::Grid, "Shift+Tab reaches the games again");

    key(Key::Home);
    expect(library->selectedIndex() == 0 && library->firstVisibleRow() == 0, "Home focuses the first game");
    key(Key::PageDown);
    key(Key::PageDown);
    expect(library->selectedIndex() == 14 && library->firstVisibleRow() == 2,
           "repeated Page Down reaches the last game and scrolls to the last row");
    key(Key::PageUp);
    expect(library->selectedIndex() == 5 && library->firstVisibleRow() == 1, "Page Up moves the focus back a screen");
    key(Key::End);
    expect(library->selectedIndex() == 14 && library->firstVisibleRow() == 2, "End focuses the last game");

    clickLibraryCard(8);  // bottom-right visible card while scrolled two rows: Ping Pong
    expectScene(SceneId::PingPongSetup, "clicking a card in a scrolled grid opens the right game");
    key(Key::Escape);
    expectScene(SceneId::GameLibrary, "Escape in a setup returns to the library");

    // Keyboard zones: Tab from the grid reaches Back; Enter there returns to the main menu.
    key(sf::Keyboard::Key::Tab);
    key(sf::Keyboard::Key::Enter);
    expectScene(SceneId::MainMenu, "Tab reaches Back and Enter leaves the library");
}

void SmokeTestDriver::scenarioTicTacToe()
{
    log_ << "\n[Classic Tic-Tac-Toe]\n";
    openLibrary();
    if (!expectScene(SceneId::GameLibrary, "Play opens the game library")) return;
    clickLibraryCard(0);
    if (!expectScene(SceneId::TicTacToeSetup, "Classic Tic-Tac-Toe opens its setup")) return;
    click({72.f + 274.f, 190.f + 41.f});  // Game mode row -> Human vs Computer
    captureBoth("tictactoe_setup");
    const auto historyBefore = historyCount();
    click({958.f + 125.f, 580.f + 28.f});
    if (!expectScene(SceneId::TicTacToeGame, "Start Match opens the board")) return;

    auto& session = app_.ticTacToeSession_;
    const auto soundsBeforeMatch = app_.audio_->startedCount();
    for (int turn = 0; turn < 12 && session.board().status() == classic_ttt::GameStatus::InProgress; ++turn) {
        if (session.isComputerTurn()) {
            wait(0.6f);
            continue;
        }
        for (const auto& position : session.board().legalMoves()) {
            click({445.f + 65.f + 130.f * static_cast<float>(position.column),
                   205.f + 65.f + 130.f * static_cast<float>(position.row)});
            break;
        }
    }
    expect(session.board().status() != classic_ttt::GameStatus::InProgress, "a complete game can be played to a result");
    expect(session.matchFinished(), "a single game finishes the match");
    expect(app_.audio_->startedCount() > soundsBeforeMatch, "moves and the result play gameplay sounds");
    captureBoth("tictactoe_result");
    expect(historyCount() == historyBefore + 1, "the completed match writes exactly one history row");
    frames(30);
    expect(historyCount() == historyBefore + 1, "the result overlay never writes a second row");

    click({435.f + 205.f, 355.f + 128.f + 27.f});
    expectScene(SceneId::GameLibrary, "Return to Library leaves the finished match");
}

void SmokeTestDriver::scenarioPingPong()
{
    log_ << "\n[Ping Pong]\n";
    openLibrary();
    type("ping");
    clickLibraryCard(0);
    if (!expectScene(SceneId::PingPongSetup, "Ping Pong opens its setup")) return;
    click({72.f + 310.f, 190.f + 36.f});  // Game mode row -> Human vs Computer
    const auto historyBefore = historyCount();
    click({958.f + 125.f, 590.f + 28.f});
    if (!expectScene(SceneId::PingPongGame, "Start Match opens the arena")) return;
    const auto soundsBeforeRally = app_.audio_->startedCount();
    wait(1.0f);
    captureBoth("pingpong_countdown");
    wait(3.2f);

    auto* scene = dynamic_cast<PingPongGameScene*>(&app_.scenes_.active());
    if (!expect(scene != nullptr, "the active scene is the Ping Pong arena")) return;

    sf::Event::KeyPressed hold{};
    hold.code = sf::Keyboard::Key::W;
    app_.dispatch(sf::Event(hold));
    wait(0.25f);
    focusLost();
    expect(scene->paused(), "losing window focus pauses the match");
    const double paddleAtFocusLoss = scene->simulationState().leftPaddle.position.y;
    focusGained();
    key(sf::Keyboard::Key::Escape);  // Resume from the pause overlay.
    expect(!scene->paused(), "Escape resumes from the pause overlay");
    wait(0.5f);
    expect(std::abs(scene->simulationState().leftPaddle.position.y - paddleAtFocusLoss) < 0.5,
           "a key held before focus loss does not keep moving the paddle");
    wait(3.0f);
    expect(app_.audio_->startedCount() > soundsBeforeRally, "the rally plays contact or point sounds");

    key(sf::Keyboard::Key::Escape);
    expect(scene->paused(), "Escape pauses the match");
    captureBoth("pingpong_pause");
    key(sf::Keyboard::Key::Down);
    key(sf::Keyboard::Key::Down);
    key(sf::Keyboard::Key::Down);
    key(sf::Keyboard::Key::Enter);
    expectScene(SceneId::GameLibrary, "Return to Library leaves the paused match");
    expect(historyCount() == historyBefore, "an abandoned Ping Pong match writes no history");
    goToMainMenu();
}

// ---------------------------------------------------------------------------------------
// Shared board games
// ---------------------------------------------------------------------------------------

namespace {
// Logical centres of the board-game setup rows and buttons, and of the board scene's controls.
sf::Vector2f setupRow(int index) { return {72.f + 274.f, 180.f + static_cast<float>(index) * 96.f + 40.f}; }
constexpr sf::Vector2f setupStart{958.f + 125.f, 580.f + 28.f};
constexpr sf::Vector2f boardBack{55.f + 95.f, 30.f + 25.f};
constexpr sf::Vector2f boardRestart{935.f + 145.f, 500.f + 25.f};
constexpr sf::Vector2f exitLeave{425.f + 95.f, 425.f + 26.f};
sf::Vector2f resultButton(int index) { return {435.f + 205.f, 340.f + static_cast<float>(index) * 62.f + 26.f}; }
} // namespace

bool SmokeTestDriver::startBoardGame(const std::string& search, bool againstComputer, bool humanFirst,
                                     const std::string& setupCapture)
{
    openLibrary();
    type(search);
    clickLibraryCard(0);
    if (!expectScene(SceneId::BoardGameSetup, "'" + search + "' opens the shared board-game setup")) return false;
    auto* setup = dynamic_cast<BoardGameSetupScene*>(&app_.scenes_.active());
    if (!expect(setup != nullptr, "the active scene is the board-game setup")) return false;
    if (setup->humanVsComputer() != againstComputer) click(setupRow(0));
    if (againstComputer && setup->humanPlaysFirst() != humanFirst) click(setupRow(3));
    expect(setup->humanVsComputer() == againstComputer && (!againstComputer || setup->humanPlaysFirst() == humanFirst),
           "clicking the setup rows chooses the mode and side");
    if (!setupCapture.empty()) captureBoth(setupCapture);
    click(setupStart);
    return expectScene(SceneId::BoardGame, "Start Game opens the board");
}

void SmokeTestDriver::playBoardMove(turn_based::MoveId move, bool keyboard)
{
    auto& host = app_.boardGameHost_;
    auto* view = dynamic_cast<board_view::CellBoardView*>(host.view.get());
    if (!view) return;
    int cell = move;
    if (host.game->key == "numerical_tic_tac_toe") {
        const int number = numerical_ttt::NumericalGame::numberOf(move);
        cell = numerical_ttt::NumericalGame::cellOf(move);
        keyWithText(static_cast<sf::Keyboard::Key>(static_cast<int>(sf::Keyboard::Key::Num0) + number),
                    static_cast<char32_t>(U'0' + number));
    }
    if (!keyboard) {
        if (const auto center = view->cellCenter(cell)) click(*center);
        return;
    }
    for (int step = 0; step < 24 && view->cursor() != cell; ++step) {
        const auto from = view->cellCenter(view->cursor()).value_or(sf::Vector2f{});
        const auto to = view->cellCenter(cell).value_or(sf::Vector2f{});
        if (to.x > from.x + 1.f) key(sf::Keyboard::Key::Right);
        else if (to.x < from.x - 1.f) key(sf::Keyboard::Key::Left);
        else if (to.y > from.y + 1.f) key(sf::Keyboard::Key::Down);
        else key(sf::Keyboard::Key::Up);
    }
    key(sf::Keyboard::Key::Enter);
}

bool SmokeTestDriver::playBoardGameToEnd(bool keyboard)
{
    auto& session = app_.boardGameHost_.session;
    for (int guard = 0; guard < 400 && !session.game().outcome().finished(); ++guard) {
        if (session.isComputerTurn()) {
            wait(0.1f);
            continue;
        }
        const auto moves = session.game().legalMoves();
        if (moves.empty()) break;
        const int before = session.game().movesPlayed();
        playBoardMove(moves.front(), keyboard);
        if (!expect(session.game().movesPlayed() == before + 1, "each human move reaches the board")) return false;
    }
    frames(2);
    return session.game().outcome().finished();
}

void SmokeTestDriver::scenarioBoardGames()
{
    log_ << "\n[Shared board games]\n";
    using persistence::GameKey;
    auto& host = app_.boardGameHost_;
    const auto active = app_.profileService_->activeProfile();
    const auto latest = [&]() -> std::optional<persistence::CompletedMatch> {
        if (!active) return std::nullopt;
        const auto rows = app_.matchRepository_->recent(active->id, {}, 1, 0);
        if (rows.empty()) return std::nullopt;
        return rows.front();
    };
    const auto boardScene = [&] { return dynamic_cast<BoardGameScene*>(&app_.scenes_.active()); };

    // SUS against the computer, playing S: mouse play, background computer moves, one recorded row.
    auto before = historyCount();
    if (startBoardGame("sus", true, true, "board_setup_sus")) {
        auto* scene = boardScene();
        expect(scene != nullptr && host.session.game().movesPlayed() == 0, "the SUS board starts empty");
        playBoardMove(host.session.game().legalMoves().front(), false);
        expect(host.session.game().movesPlayed() == 1, "clicking a cell places the human's S");
        wait(0.1f);
        expect(scene != nullptr && scene->computerThinking() && host.session.game().movesPlayed() == 1,
               "the computer searches in the background and waits a moment before moving");
        wait(0.6f);
        expect(host.session.game().movesPlayed() == 2, "the computer answers after its short thinking pause");
        captureBoth("board_sus_in_progress");
        expect(playBoardGameToEnd(false), "a SUS game against the computer can be played to the end");
        expect(scene != nullptr && scene->resultVisible(), "the result panel appears when the game ends");
        captureBoth("board_sus_result");
        expect(historyCount() == before + 1, "the finished SUS game writes exactly one history row");
        frames(30);
        expect(historyCount() == before + 1, "the result panel never writes a second row");
        const auto row = latest();
        expect(row && row->game == GameKey::Sus && row->mode == persistence::MatchMode::HumanVsComputer &&
                   row->difficulty == persistence::DifficultyKey::Standard &&
                   row->profileSideOrMark == std::optional<std::string>{"S"} && row->profileScore.has_value(),
               "the SUS row stores the side, the standard computer opponent, and the points");
        const auto points = host.session.game().scores();
        const auto winner = host.session.game().outcome().winner;
        const auto expected = !winner ? persistence::MatchResult::Draw
            : *winner == turn_based::Seat::First ? persistence::MatchResult::Win : persistence::MatchResult::Loss;
        expect(row && points && row->profileScore == points->first && row->opponentScore == points->second &&
                   row->result == expected,
               "the SUS row stores the human's result and points from the human's side (S)");
        click(resultButton(3));
        expectScene(SceneId::GameLibrary, "Return to Library leaves the finished game");
    }

    // 5x5 as O: the computer opens, then the final board can be studied behind the result panel.
    before = historyCount();
    if (startBoardGame("5x5", true, false)) {
        auto* scene = boardScene();
        expect(host.session.isComputerTurn(), "playing O, the computer (X) moves first");
        wait(0.6f);
        expect(host.session.game().movesPlayed() == 1, "the computer opens the 5x5 game on its own");
        for (int move = 0; move < 3; ++move) {
            playBoardMove(host.session.game().legalMoves().front(), false);
            wait(0.6f);
        }
        captureBoth("board_5x5_in_progress");
        expect(playBoardGameToEnd(false), "a 5x5 game against the computer can be played to the end");
        expect(host.session.game().movesPlayed() == 24, "5x5 ends after exactly 24 moves");
        key(sf::Keyboard::Key::Escape);
        expect(scene != nullptr && scene->reviewingBoard() && !scene->resultVisible(),
               "Escape on the result panel shows the final board");
        captureBoth("board_5x5_final_board");
        key(sf::Keyboard::Key::Enter);
        expect(scene != nullptr && scene->resultVisible(), "Enter shows the result again");
        expect(historyCount() == before + 1, "the 5x5 game is recorded once");
        const auto row = latest();
        expect(row && row->game == GameKey::FiveByFiveTicTacToe && row->profileSideOrMark == std::optional<std::string>{"O"} &&
                   row->profileScore.has_value() && row->opponentScore.has_value(),
               "the 5x5 row stores the human's O side and both players' three-in-a-row counts");
        const auto points = host.session.game().scores();
        const auto winner = host.session.game().outcome().winner;
        const auto expected = !winner ? persistence::MatchResult::Draw
            : *winner == turn_based::Seat::Second ? persistence::MatchResult::Win : persistence::MatchResult::Loss;
        expect(row && points && row->profileScore == points->second && row->opponentScore == points->first &&
                   row->result == expected,
               "the 5x5 row stores the human's result and points from the human's side (O, the second seat)");
        click(resultButton(3));
        expectScene(SceneId::GameLibrary, "Return to Library leaves the finished 5x5 game");
    }

    // Misere, two players, keyboard only: rules panel, Tab focus, rematch, and New Setup.
    before = historyCount();
    if (startBoardGame("misere", false, true)) {
        auto* scene = boardScene();
        using Focus = BoardGameScene::Focus;
        key(sf::Keyboard::Key::F1);
        expect(scene != nullptr && scene->rulesVisible(), "F1 opens the rules panel");
        captureBoth("board_rules_misere");
        key(sf::Keyboard::Key::Escape);
        expect(scene != nullptr && !scene->rulesVisible() && activeScene() == SceneId::BoardGame,
               "Escape closes the rules panel and stays in the game");
        key(sf::Keyboard::Key::Tab);
        expect(scene != nullptr && scene->focus() == Focus::Restart, "Tab moves the focus from the board to Restart");
        key(sf::Keyboard::Key::Tab);
        key(sf::Keyboard::Key::Enter);
        expect(scene != nullptr && scene->focus() == Focus::Rules && scene->rulesVisible(),
               "Enter on the focused Rules button opens the rules");
        key(sf::Keyboard::Key::Escape);
        key(sf::Keyboard::Key::Tab);
        key(sf::Keyboard::Key::Tab);
        expect(scene != nullptr && scene->focus() == Focus::Board, "Tab cycles through Back to the board");
        expect(playBoardGameToEnd(true), "a Misere game can be played with the keyboard alone");
        expect(historyCount() == before + 1, "the finished Misere game is recorded");
        const auto row = latest();
        expect(row && row->game == GameKey::MisereTicTacToe && row->mode == persistence::MatchMode::HumanVsHuman &&
                   !row->profileScore && row->profileSideOrMark == std::optional<std::string>{"X"},
               "the Misere row stores Player 1 as X without points");
        click(resultButton(0));
        expect(host.session.gameNumber() == 2 && host.session.game().movesPlayed() == 0 && scene != nullptr &&
                   !scene->resultVisible(),
               "Rematch starts game 2 with the same players");
        expect(historyCount() == before + 1, "a rematch records nothing until it finishes");
        expect(playBoardGameToEnd(false), "the rematch can be played to the end");
        expect(historyCount() == before + 2, "the rematch is recorded as its own game");
        click(resultButton(2));
        expectScene(SceneId::BoardGameSetup, "New Setup returns to the same game's setup");
        key(sf::Keyboard::Key::Escape);
        expectScene(SceneId::GameLibrary, "Escape leaves the setup for the library");
    }

    // Numerical, two players: number keys, restart, and leaving an unfinished game unrecorded.
    before = historyCount();
    if (startBoardGame("numerical", false, true)) {
        auto* scene = boardScene();
        auto* view = dynamic_cast<board_view::CellBoardView*>(host.view.get());
        const auto& numerical = dynamic_cast<const numerical_ttt::NumericalGame&>(host.session.game());
        keyWithText(sf::Keyboard::Key::Num5, U'5');
        if (view) click(view->cellCenter(4).value_or(sf::Vector2f{}));
        expect(numerical.cell(4) == 5, "a number key chooses the number and a click places it");
        keyWithText(sf::Keyboard::Key::Num3, U'3');
        if (view) click(view->cellCenter(0).value_or(sf::Vector2f{}));
        expect(host.session.game().movesPlayed() == 2 && numerical.cell(0) == 2,
               "Player 2 cannot choose an odd number, so the preselected 2 is placed");
        captureBoth("board_numerical_in_progress");
        key(sf::Keyboard::Key::Escape);
        expect(scene != nullptr && scene->exitConfirmationVisible(), "Escape during a game asks before leaving");
        key(sf::Keyboard::Key::Enter);
        expect(scene != nullptr && !scene->exitConfirmationVisible() && activeScene() == SceneId::BoardGame,
               "Keep Playing (the default) returns to the game");
        click(boardRestart);
        expect(host.session.game().movesPlayed() == 0 && host.session.gameNumber() == 2,
               "Restart Game clears the board for a new game");
        playBoardMove(host.session.game().legalMoves().front(), false);
        click(boardBack);
        expect(scene != nullptr && scene->exitConfirmationVisible(), "Back to Library also asks first");
        click(exitLeave);
        expectScene(SceneId::GameLibrary, "Leave Game returns to the library");
        expect(historyCount() == before, "restarted and abandoned games write no history");
    }
    if (startBoardGame("numerical", false, true)) {
        expect(playBoardGameToEnd(false), "a Numerical game can be finished with number keys and clicks");
        expect(historyCount() == before + 1, "the finished Numerical game is recorded");
        const auto row = latest();
        expect(row && row->game == GameKey::NumericalTicTacToe && row->profileSideOrMark == std::optional<std::string>{"Odd"} &&
                   !row->profileScore,
               "the Numerical row stores Player 1's odd side without points");
        click(resultButton(3));
        expectScene(SceneId::GameLibrary, "Return to Library leaves the finished Numerical game");
    }
    goToMainMenu();
}
