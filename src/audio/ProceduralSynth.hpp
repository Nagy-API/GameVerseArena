#pragma once

#include "SoundTypes.hpp"

#include <cstdint>
#include <vector>

namespace audio {

inline constexpr unsigned int defaultSampleRate = 44100;
// Peak amplitude ceiling as a fraction of 16-bit full scale; leaves headroom so overlapping
// voices are less likely to clip.
inline constexpr double peakLimit = 0.8;
inline constexpr double maximumSoundSeconds = 1.2;

// Generates a short, original mono 16-bit PCM effect for `id`. The output is deterministic,
// starts and ends at silence (no clicks), never exceeds `peakLimit`, and needs no audio
// device or file.
std::vector<std::int16_t> synthesize(SoundId id, unsigned int sampleRate = defaultSampleRate);

} // namespace audio
