#include "AudioPolicy.hpp"
#include "ProceduralSynth.hpp"
#include "SoundTypes.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

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

std::vector<audio::SoundId> allSounds()
{
    std::vector<audio::SoundId> sounds;
    for (std::size_t index = 0; index < audio::soundCount; ++index) sounds.push_back(static_cast<audio::SoundId>(index));
    return sounds;
}

int peakOf(const std::vector<std::int16_t>& samples)
{
    int peak = 0;
    for (const auto sample : samples) peak = std::max(peak, std::abs(static_cast<int>(sample)));
    return peak;
}

void testCatalogueAndCategories()
{
    using audio::SoundCategory;
    using audio::SoundId;
    check(audio::soundCount == 15, "the procedural catalogue has 15 effects");
    check(audio::categoryOf(SoundId::UiFocus) == SoundCategory::Ui && audio::categoryOf(SoundId::UiConfirm) == SoundCategory::Ui &&
              audio::categoryOf(SoundId::UiBack) == SoundCategory::Ui && audio::categoryOf(SoundId::UiError) == SoundCategory::Ui,
          "focus, confirm, back, and error are UI sounds");
    for (const auto id : {SoundId::MovePrimary, SoundId::MoveSecondary, SoundId::SpecialEvent, SoundId::RoundWin,
                          SoundId::RoundDraw, SoundId::RoundLoss, SoundId::PaddleHit, SoundId::WallHit,
                          SoundId::PointScored, SoundId::MatchWin}) {
        check(audio::categoryOf(id) == SoundCategory::Gameplay, std::string(audio::soundName(id)) + " is a gameplay sound");
    }
    check(audio::categoryOf(SoundId::AchievementUnlocked) == SoundCategory::Achievement,
          "achievement unlock uses the achievement category");

    std::set<std::string> names;
    for (const auto id : allSounds()) names.insert(audio::soundName(id));
    check(names.size() == audio::soundCount && names.count("unknown") == 0, "every sound has a unique name");
}

void testSynthesis()
{
    const int ceiling = static_cast<int>(std::lround(audio::peakLimit * 32767.0));
    std::set<std::vector<std::int16_t>> distinct;
    for (const auto id : allSounds()) {
        const std::string name = audio::soundName(id);
        const auto samples = audio::synthesize(id);
        const double seconds = static_cast<double>(samples.size()) / audio::defaultSampleRate;
        check(!samples.empty(), name + " produces samples");
        check(seconds >= 0.03 && seconds <= audio::maximumSoundSeconds, name + " is a short effect (30 ms to 1.2 s)");
        const int peak = peakOf(samples);
        check(peak <= ceiling + 1, name + " never exceeds the peak limit (no clipping headroom lost)");
        check(peak >= static_cast<int>(0.15 * 32767.0), name + " is clearly audible");
        check(std::abs(static_cast<int>(samples.front())) <= 200 && std::abs(static_cast<int>(samples.back())) <= 200,
              name + " starts and ends at silence so playback does not click");
        check(audio::synthesize(id) == samples, name + " is deterministic");
        const auto halfRate = audio::synthesize(id, 22050);
        const double ratio = static_cast<double>(halfRate.size()) / static_cast<double>(samples.size());
        check(ratio > 0.48 && ratio < 0.52, name + " respects the requested sample rate");
        distinct.insert(samples);
    }
    check(distinct.size() == audio::soundCount, "every effect is a different sound");

    bool rejected = false;
    try { (void)audio::synthesize(audio::SoundId::UiFocus, 1000); }
    catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "unsupported sample rates are rejected");
    rejected = false;
    try { (void)audio::synthesize(audio::SoundId::Count); }
    catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "the Count sentinel is not a playable sound");
}

void testMixing()
{
    using audio::SoundCategory;
    audio::MixLevels levels;  // defaults 80 / 70 / 80 / 85
    check(std::abs(audio::effectiveVolume(levels, SoundCategory::Ui) - 56.f) < 0.001f, "default UI volume is 80% x 70%");
    check(std::abs(audio::effectiveVolume(levels, SoundCategory::Gameplay) - 64.f) < 0.001f,
          "default gameplay volume is 80% x 80%");
    check(std::abs(audio::effectiveVolume(levels, SoundCategory::Achievement) - 68.f) < 0.001f,
          "default achievement volume is 80% x 85%");
    levels.muted = true;
    check(audio::effectiveVolume(levels, SoundCategory::Ui) == 0.f &&
              audio::effectiveVolume(levels, SoundCategory::Gameplay) == 0.f &&
              audio::effectiveVolume(levels, SoundCategory::Achievement) == 0.f,
          "Mute All silences every category");
    levels.muted = false;
    levels.master = 0;
    check(audio::effectiveVolume(levels, SoundCategory::Achievement) == 0.f, "master volume 0 silences everything");
    levels.master = 100;
    levels.gameplay = 100;
    check(audio::effectiveVolume(levels, SoundCategory::Gameplay) == 100.f, "100% master and category is full volume");
    levels.gameplay = 0;
    check(audio::effectiveVolume(levels, SoundCategory::Gameplay) == 0.f &&
              audio::effectiveVolume(levels, SoundCategory::Ui) == 70.f,
          "a zero category does not affect the other categories");
    levels.master = 250;
    levels.ui = -10;
    check(audio::effectiveVolume(levels, SoundCategory::Ui) == 0.f &&
              audio::effectiveVolume(levels, SoundCategory::Achievement) == 85.f,
          "out-of-range levels are clamped to 0..100");
}

void testVoicePool()
{
    check(audio::chooseVoice({false, false, false}, {0, 0, 0}) == 0, "the first idle voice is used");
    check(audio::chooseVoice({true, false, true}, {5, 1, 2}) == 1, "an idle voice is preferred over stealing");
    check(audio::chooseVoice({true, true, true}, {7, 3, 9}) == 1, "when every voice is busy the oldest is reused");
    bool rejected = false;
    try { (void)audio::chooseVoice({}, {}); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "an empty voice pool is rejected");
    rejected = false;
    try { (void)audio::chooseVoice({true}, {1, 2}); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "inconsistent voice bookkeeping is rejected");

    // The pool never grows: 100 rapid requests on a 12-voice pool always choose an index < 12.
    std::vector<bool> playing(12, true);
    std::vector<std::uint64_t> order(12, 0);
    std::uint64_t sequence = 0;
    bool bounded = true;
    for (int request = 0; request < 100; ++request) {
        const auto voice = audio::chooseVoice(playing, order);
        bounded = bounded && voice < playing.size();
        order[voice] = ++sequence;
    }
    check(bounded, "a saturated pool keeps reusing its bounded voices");
}

void testThrottle()
{
    audio::SoundThrottle throttle(0.05);
    check(throttle.allow(audio::SoundId::UiFocus, 1.0), "first play is allowed");
    check(!throttle.allow(audio::SoundId::UiFocus, 1.02), "an immediate duplicate is suppressed");
    check(throttle.allow(audio::SoundId::UiConfirm, 1.02), "different sounds are throttled independently");
    check(throttle.allow(audio::SoundId::UiFocus, 1.06), "the same sound plays again after the interval");
    check(!throttle.allow(audio::SoundId::Count, 5.0), "the sentinel id never plays");
}
} // namespace

int main()
{
    testCatalogueAndCategories();
    testSynthesis();
    testMixing();
    testVoicePool();
    testThrottle();

    if (failures == 0) {
        std::cout << "Audio tests passed: " << checks << " checks\n";
        return 0;
    }
    std::cerr << failures << " of " << checks << " audio checks failed\n";
    return 1;
}
