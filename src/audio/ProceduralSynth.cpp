#include "ProceduralSynth.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace audio {
namespace {
constexpr double pi = 3.14159265358979323846;

enum class Wave { Sine, Triangle, Soft, Bell, Noise };

struct Partial {
    Wave wave;
    double start;   // seconds from the beginning of the effect
    double length;  // seconds
    double fromHz;
    double toHz;    // exponential sweep target
    double level;
    double attack;  // linear attack time, seconds
    double decay;   // exponential decay time constant, seconds (0 = none)
};

struct Recipe {
    std::vector<Partial> partials;
    double gain;  // final peak = peakLimit * gain
};

Partial tone(Wave wave, double start, double length, double hz, double decay, double level = 1.0,
             double attack = 0.003)
{
    return {wave, start, length, hz, hz, level, attack, decay};
}

Partial sweep(Wave wave, double start, double length, double fromHz, double toHz, double decay,
              double level = 1.0, double attack = 0.002)
{
    return {wave, start, length, fromHz, toHz, level, attack, decay};
}

Recipe recipeFor(SoundId id)
{
    using W = Wave;
    switch (id) {
    case SoundId::UiFocus:
        return {{sweep(W::Sine, 0.0, 0.045, 1480.0, 1400.0, 0.018)}, 0.42};
    case SoundId::UiConfirm:
        return {{tone(W::Triangle, 0.0, 0.05, 660.0, 0.05), tone(W::Triangle, 0.045, 0.07, 990.0, 0.05),
                 tone(W::Sine, 0.045, 0.05, 1980.0, 0.02, 0.25)}, 0.55};
    case SoundId::UiBack:
        return {{tone(W::Triangle, 0.0, 0.05, 784.0, 0.05), tone(W::Triangle, 0.045, 0.08, 523.25, 0.06)}, 0.5};
    case SoundId::UiError:
        return {{sweep(W::Soft, 0.0, 0.17, 185.0, 165.0, 0.12, 1.0, 0.004),
                 sweep(W::Soft, 0.0, 0.17, 196.0, 174.0, 0.12, 0.8, 0.004)}, 0.55};
    case SoundId::MovePrimary:
        return {{sweep(W::Triangle, 0.0, 0.075, 620.0, 470.0, 0.035),
                 {W::Noise, 0.0, 0.012, 1.0, 1.0, 0.18, 0.0005, 0.004}}, 0.62};
    case SoundId::MoveSecondary:
        return {{sweep(W::Sine, 0.0, 0.09, 392.0, 587.33, 0.05, 1.0, 0.004),
                 sweep(W::Sine, 0.0, 0.06, 784.0, 1174.66, 0.03, 0.2, 0.004)}, 0.62};
    case SoundId::SpecialEvent:
        return {{tone(W::Sine, 0.0, 0.07, 880.0, 0.05), tone(W::Sine, 0.06, 0.07, 1108.73, 0.05),
                 tone(W::Sine, 0.12, 0.12, 1318.51, 0.08)}, 0.58};
    case SoundId::RoundWin:
        return {{tone(W::Triangle, 0.0, 0.12, 523.25, 0.15), tone(W::Triangle, 0.1, 0.12, 659.25, 0.15),
                 tone(W::Triangle, 0.2, 0.12, 783.99, 0.15), tone(W::Triangle, 0.3, 0.28, 1046.5, 0.25),
                 tone(W::Sine, 0.3, 0.2, 2093.0, 0.1, 0.15)}, 0.78};
    case SoundId::RoundDraw:
        return {{tone(W::Triangle, 0.0, 0.16, 587.33, 0.15), tone(W::Triangle, 0.17, 0.2, 587.33, 0.18),
                 tone(W::Sine, 0.17, 0.2, 440.0, 0.18, 0.5)}, 0.62};
    case SoundId::RoundLoss:
        return {{tone(W::Triangle, 0.0, 0.14, 493.88, 0.15), tone(W::Triangle, 0.13, 0.14, 440.0, 0.15),
                 tone(W::Triangle, 0.26, 0.3, 369.99, 0.25)}, 0.6};
    case SoundId::PaddleHit:
        return {{sweep(W::Soft, 0.0, 0.05, 460.0, 420.0, 0.02, 1.0, 0.001),
                 {W::Noise, 0.0, 0.008, 1.0, 1.0, 0.2, 0.0005, 0.003}}, 0.6};
    case SoundId::WallHit:
        return {{sweep(W::Soft, 0.0, 0.045, 260.0, 230.0, 0.018, 1.0, 0.001)}, 0.48};
    case SoundId::PointScored:
        return {{tone(W::Triangle, 0.0, 0.1, 659.25, 0.1), tone(W::Triangle, 0.08, 0.18, 987.77, 0.15)}, 0.66};
    case SoundId::MatchWin:
        return {{tone(W::Triangle, 0.0, 0.14, 523.25, 0.15), tone(W::Triangle, 0.12, 0.14, 659.25, 0.15),
                 tone(W::Triangle, 0.24, 0.14, 783.99, 0.15), tone(W::Triangle, 0.36, 0.5, 1046.5, 0.45),
                 tone(W::Triangle, 0.36, 0.5, 783.99, 0.45, 0.6), tone(W::Triangle, 0.36, 0.5, 659.25, 0.45, 0.5)},
                0.8};
    case SoundId::AchievementUnlocked:
        return {{tone(W::Bell, 0.0, 0.35, 1046.5, 0.25), tone(W::Bell, 0.09, 0.35, 1318.51, 0.25),
                 tone(W::Bell, 0.18, 0.45, 1567.98, 0.3), tone(W::Bell, 0.27, 0.5, 2093.0, 0.35, 0.8)},
                0.78};
    case SoundId::Count:
        break;
    }
    throw std::invalid_argument("Unknown sound identifier");
}

double oscillate(Wave wave, double phase, std::uint32_t& noiseState)
{
    switch (wave) {
    case Wave::Sine: return std::sin(phase);
    case Wave::Triangle: return 2.0 / pi * std::asin(std::sin(phase));
    case Wave::Soft: return std::tanh(2.5 * std::sin(phase)) / std::tanh(2.5);
    case Wave::Bell: return 0.62 * std::sin(phase) + 0.26 * std::sin(2.76 * phase) + 0.12 * std::sin(5.4 * phase);
    case Wave::Noise:
        noiseState = noiseState * 1664525u + 1013904223u;
        return static_cast<double>(noiseState >> 8) / static_cast<double>(1u << 24) * 2.0 - 1.0;
    }
    return 0.0;
}

// Raised-cosine gain that is 0 at `position == 0` and 1 once `position >= width`.
double edge(double position, double width)
{
    if (position >= width) return 1.0;
    if (position <= 0.0) return 0.0;
    return 0.5 - 0.5 * std::cos(pi * position / width);
}
} // namespace

std::vector<std::int16_t> synthesize(SoundId id, unsigned int sampleRate)
{
    if (sampleRate < 8000 || sampleRate > 192000) throw std::invalid_argument("Unsupported sample rate");
    const Recipe recipe = recipeFor(id);
    double totalSeconds = 0.0;
    for (const auto& partial : recipe.partials) totalSeconds = std::max(totalSeconds, partial.start + partial.length);
    const double rate = static_cast<double>(sampleRate);
    const auto totalSamples = static_cast<std::size_t>(std::ceil(totalSeconds * rate)) + 1;
    std::vector<double> mix(totalSamples, 0.0);

    std::uint32_t partialIndex = 0;
    for (const auto& partial : recipe.partials) {
        const auto first = static_cast<std::size_t>(std::lround(partial.start * rate));
        const auto count = static_cast<std::size_t>(std::lround(partial.length * rate));
        std::uint32_t noiseState = 0x9E3779B9u ^ (++partialIndex * 0x85EBCA6Bu);
        double phase = 0.0;
        for (std::size_t sample = 0; sample < count && first + sample < mix.size(); ++sample) {
            const double time = static_cast<double>(sample) / rate;
            const double progress = time / partial.length;
            const double frequency = partial.fromHz * std::pow(partial.toHz / partial.fromHz, progress);
            double envelope = partial.attack > 0.0 ? std::min(1.0, time / partial.attack) : 1.0;
            if (partial.decay > 0.0) envelope *= std::exp(-time / partial.decay);
            envelope *= edge(partial.length - time, 0.006);
            mix[first + sample] += partial.level * envelope * oscillate(partial.wave, phase, noiseState);
            phase += 2.0 * pi * frequency / rate;
        }
    }

    double peak = 0.0;
    for (const double value : mix) peak = std::max(peak, std::abs(value));
    const double scale = peak > 0.0 ? peakLimit * recipe.gain / peak : 0.0;
    std::vector<std::int16_t> samples(mix.size());
    const double fadeIn = 0.002 * rate;
    const double fadeOut = 0.004 * rate;
    for (std::size_t index = 0; index < mix.size(); ++index) {
        const double position = static_cast<double>(index);
        const double remaining = static_cast<double>(mix.size() - 1 - index);
        const double value = mix[index] * scale * edge(position, fadeIn) * edge(remaining, fadeOut);
        samples[index] = static_cast<std::int16_t>(std::lround(std::clamp(value, -1.0, 1.0) * 32767.0));
    }
    return samples;
}

} // namespace audio
