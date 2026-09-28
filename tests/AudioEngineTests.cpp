// Exercises the SFML-backed AudioEngine and the SettingsController on SFML's silent (null)
// playback device, so no sound hardware is needed.
#include "AudioEngine.hpp"
#include "Database.hpp"
#include "SettingsController.hpp"
#include "SettingsService.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

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

struct TemporaryDirectory {
    TemporaryDirectory()
    {
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() / ("GameVerseArena-audio-engine-tests-" + std::to_string(stamp));
        std::filesystem::create_directories(path);
    }
    ~TemporaryDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
    std::filesystem::path path;
};

void pause(int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void testEngineOnSilentDevice()
{
    AudioEngine engine(AudioEngine::Output::NullDevice);
    check(engine.available(), "the engine is available on the silent device: " + engine.status());
    check(engine.voiceCount() == AudioEngine::voiceLimit, "the voice pool has exactly 12 voices");

    engine.play(audio::SoundId::UiConfirm);
    check(engine.startedCount() == 1, "a sound starts at the default mix");
    engine.play(audio::SoundId::UiConfirm);
    check(engine.startedCount() == 1, "an immediate duplicate of the same sound is throttled");
    engine.play(audio::SoundId::RoundWin);
    check(engine.startedCount() == 2, "a different sound is not throttled");

    // Start more long sounds than there are voices: stealing keeps the pool bounded.
    for (int round = 0; round < 2; ++round) {
        for (std::size_t index = 0; index < audio::soundCount; ++index) {
            engine.play(static_cast<audio::SoundId>(index));
        }
        pause(60);
    }
    check(engine.voiceCount() == AudioEngine::voiceLimit, "many overlapping sounds never grow the pool");
    check(engine.playingVoices() <= AudioEngine::voiceLimit, "no more than 12 sounds play at once");

    engine.play(audio::SoundId::AchievementUnlocked);
    pause(60);
    check(engine.playingVoices() > 0, "sounds are playing before muting");
    audio::MixLevels muted = engine.mix();
    muted.muted = true;
    engine.setMix(muted);
    check(engine.playingVoices() == 0, "Mute All stops sounds that are already playing");
    const auto beforeMutedPlay = engine.startedCount();
    pause(60);
    engine.play(audio::SoundId::MatchWin);
    check(engine.startedCount() == beforeMutedPlay, "no sound starts while muted");

    audio::MixLevels silentGameplay;
    silentGameplay.gameplay = 0;
    engine.setMix(silentGameplay);
    pause(60);
    const auto beforeCategory = engine.startedCount();
    engine.play(audio::SoundId::PaddleHit);
    check(engine.startedCount() == beforeCategory, "a 0% category starts no sound");
    engine.play(audio::SoundId::UiBack);
    check(engine.startedCount() == beforeCategory + 1, "other categories still play");
    engine.stopAll();
    check(engine.playingVoices() == 0, "stopAll silences every voice");
}

void testSettingsController(const std::filesystem::path& directory)
{
    AudioEngine engine(AudioEngine::Output::NullDevice);
    persistence::Database database(directory / "controller.db");
    persistence::SettingsService service(database);
    SettingsController controller(service, engine);
    controller.load();
    check(controller.current() == persistence::defaultSettings() && controller.loadIssues().empty(),
          "a fresh database loads default settings");
    check(engine.mix().master == 80 && engine.mix().ui == 70 && !engine.mix().muted,
          "loading pushes the defaults to the audio mix");

    persistence::AppSettings custom;
    custom.masterVolume = 55;
    custom.achievementVolume = 10;
    custom.reducedMotion = true;
    check(controller.apply(custom), "valid settings apply and save");
    check(engine.mix().master == 55 && engine.mix().achievement == 10, "apply updates the mix immediately");
    check(controller.reducedMotion(), "apply updates Reduced Motion immediately");
    check(service.load().settings == custom, "apply persists the settings");

    auto invalid = custom;
    invalid.uiVolume = 150;
    check(!controller.apply(invalid) && !controller.lastError().empty(), "out-of-range settings are refused");
    check(controller.current() == custom, "a refused change leaves the active settings untouched");

    database.execute("DROP TABLE app_settings;");
    auto unsaved = custom;
    unsaved.audioMuted = true;
    check(!controller.apply(unsaved), "a failed save is reported");
    check(controller.current().audioMuted && engine.mix().muted,
          "a failed save keeps the new values active for this session");
    check(controller.lastError() == "Settings are active but could not be saved.",
          "a failed save explains that the change is active but unsaved");

    SettingsController reloaded(service, engine);
    reloaded.load();
    check(reloaded.current() == persistence::defaultSettings() && !reloaded.loadIssues().empty(),
          "an unreadable settings table falls back to defaults and reports it");
}
} // namespace

int main()
{
    TemporaryDirectory temporary;
    testEngineOnSilentDevice();
    testSettingsController(temporary.path);

    if (failures == 0) {
        std::cout << "Audio engine tests passed: " << checks << " checks\n";
        return 0;
    }
    std::cerr << failures << " of " << checks << " audio engine checks failed\n";
    return 1;
}
