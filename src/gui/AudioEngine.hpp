#pragma once

#include "AudioPolicy.hpp"
#include "SoundTypes.hpp"

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Plays the procedurally generated effects through a bounded pool of SFML voices.
// Construction never throws: when no output device exists, or buffers cannot be created,
// the engine stays silent and reports why, and the rest of the application is unaffected.
class AudioEngine {
public:
    enum class Output { DefaultDevice, NullDevice };

    static constexpr std::size_t voiceLimit = 12;

    explicit AudioEngine(Output output = Output::DefaultDevice);
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    // Applies immediately, including to sounds already playing: a voice whose category is now
    // silent (Mute All or a 0% level) stops, and the others change volume.
    void setMix(const audio::MixLevels& levels);
    const audio::MixLevels& mix() const noexcept { return mix_; }

    // Starts `id` on a pooled voice at the current category volume. Silent (and cheap)
    // when audio is unavailable, the effective volume is zero, or the same sound was
    // started within the throttle interval.
    void play(audio::SoundId id);
    void stopAll();

    bool available() const noexcept { return available_; }
    const std::string& status() const noexcept { return status_; }
    // Diagnostics for tests and the smoke harness.
    std::size_t voiceCount() const noexcept { return voices_.size(); }
    std::uint64_t startedCount() const noexcept { return started_; }
    std::size_t playingVoices() const;

private:
    double secondsSinceStart() const;

    // Buffers are declared before voices so every sf::Sound is destroyed before the
    // sf::SoundBuffer it references.
    std::array<std::optional<sf::SoundBuffer>, audio::soundCount> buffers_{};
    std::vector<std::optional<sf::Sound>> voices_;
    std::vector<audio::SoundId> voiceSounds_;
    std::vector<std::uint64_t> startOrder_;
    audio::SoundThrottle throttle_;
    audio::MixLevels mix_{};
    std::chrono::steady_clock::time_point epoch_;
    std::uint64_t sequence_{};
    std::uint64_t started_{};
    bool available_{false};
    std::string status_;
};
