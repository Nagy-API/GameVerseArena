#include "AudioEngine.hpp"

#include "ProceduralSynth.hpp"

#include <SFML/Audio/PlaybackDevice.hpp>
#include <SFML/Audio/SoundChannel.hpp>

#include <algorithm>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

AudioEngine::AudioEngine(Output output) : epoch_(std::chrono::steady_clock::now())
{
    try {
        if (output == Output::NullDevice) {
            if (!sf::PlaybackDevice::setDeviceToNull()) {
                status_ = "The silent audio device could not be selected. Sound is disabled.";
                return;
            }
        } else {
            // SFML 3.1 falls back to miniaudio's null backend when no hardware exists and then
            // reports only its "NULL Playback Device"; treat that the same as no device at all.
            const auto devices = sf::PlaybackDevice::getAvailableDevices();
            const bool onlyNullDevice = std::all_of(devices.begin(), devices.end(), [](const std::string& name) {
                return name.rfind("NULL Playback Device", 0) == 0;
            });
            if (devices.empty() || onlyNullDevice) {
                status_ = "No audio output device was found. Sound is disabled.";
                std::cerr << "GameVerseArenaGUI: " << status_ << '\n';
                return;
            }
        }

        for (std::size_t index = 0; index < audio::soundCount; ++index) {
            const auto samples = audio::synthesize(static_cast<audio::SoundId>(index), audio::defaultSampleRate);
            auto& buffer = buffers_[index].emplace();
            if (!buffer.loadFromSamples(samples.data(), samples.size(), 1, audio::defaultSampleRate,
                                        {sf::SoundChannel::Mono})) {
                throw std::runtime_error(std::string("could not create the '") +
                                         audio::soundName(static_cast<audio::SoundId>(index)) + "' sound");
            }
        }
        voices_.resize(voiceLimit);
        voiceSounds_.assign(voiceLimit, audio::SoundId::UiFocus);
        startOrder_.assign(voiceLimit, 0);
        available_ = true;
        status_ = output == Output::NullDevice ? "Audio routed to the silent device." : "Audio ready.";
    } catch (const std::exception& error) {
        voices_.clear();
        voiceSounds_.clear();
        startOrder_.clear();
        for (auto& buffer : buffers_) buffer.reset();
        available_ = false;
        status_ = std::string("Audio is unavailable: ") + error.what();
        std::cerr << "GameVerseArenaGUI: " << status_ << '\n';
    }
}

AudioEngine::~AudioEngine()
{
    stopAll();
    voices_.clear();
}

void AudioEngine::play(audio::SoundId id)
{
    const auto index = static_cast<std::size_t>(id);
    if (!available_ || index >= buffers_.size() || !buffers_[index]) return;
    const float volume = audio::effectiveVolume(mix_, audio::categoryOf(id));
    if (volume <= 0.f) return;
    if (!throttle_.allow(id, secondsSinceStart())) return;

    try {
        std::vector<bool> playing(voices_.size(), false);
        for (std::size_t voice = 0; voice < voices_.size(); ++voice) {
            playing[voice] = voices_[voice] && voices_[voice]->getStatus() == sf::Sound::Status::Playing;
        }
        const std::size_t voice = audio::chooseVoice(playing, startOrder_);
        if (voices_[voice]) {
            voices_[voice]->stop();
            voices_[voice]->setBuffer(*buffers_[index]);
        } else {
            voices_[voice].emplace(*buffers_[index]);
        }
        voices_[voice]->setVolume(volume);
        voices_[voice]->play();
        voiceSounds_[voice] = id;
        startOrder_[voice] = ++sequence_;
        ++started_;
    } catch (const std::exception& error) {
        std::cerr << "GameVerseArenaGUI: sound playback failed: " << error.what() << '\n';
    }
}

void AudioEngine::setMix(const audio::MixLevels& levels)
{
    mix_ = levels;
    for (std::size_t voice = 0; voice < voices_.size(); ++voice) {
        if (!voices_[voice] || voices_[voice]->getStatus() != sf::Sound::Status::Playing) continue;
        const float volume = audio::effectiveVolume(mix_, audio::categoryOf(voiceSounds_[voice]));
        if (volume <= 0.f) voices_[voice]->stop();
        else voices_[voice]->setVolume(volume);
    }
}

std::size_t AudioEngine::playingVoices() const
{
    return static_cast<std::size_t>(std::count_if(voices_.begin(), voices_.end(), [](const auto& voice) {
        return voice && voice->getStatus() == sf::Sound::Status::Playing;
    }));
}

void AudioEngine::stopAll()
{
    for (auto& voice : voices_) {
        if (voice) voice->stop();
    }
}

double AudioEngine::secondsSinceStart() const
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - epoch_).count();
}
