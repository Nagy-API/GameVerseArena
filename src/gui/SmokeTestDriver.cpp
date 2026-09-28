#include "SmokeTestDriver.hpp"

#include "Application.hpp"
#include "AudioEngine.hpp"
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
    scenarioTicTacToe();
    scenarioPingPong();

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

void SmokeTestDriver::scenarioTicTacToe()
{
    log_ << "\n[Classic Tic-Tac-Toe]\n";
    click(mainMenuButton(0));
    if (!expectScene(SceneId::GameLibrary, "Play opens the game library")) return;
    captureBoth("library");
    click({72.f + 265.f, 190.f + 59.f});
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
    if (activeScene() != SceneId::GameLibrary) {
        goToMainMenu();
        click(mainMenuButton(0));
    }
    click({678.f + 265.f, 190.f + 59.f});
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
