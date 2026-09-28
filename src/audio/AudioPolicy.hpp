#pragma once

#include "SoundTypes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace audio {

// Voice-pool policy: reuse the first idle voice; when every voice is busy, steal the one
// that started earliest so new feedback is never dropped and the pool never grows.
std::size_t chooseVoice(const std::vector<bool>& playing, const std::vector<std::uint64_t>& startOrder);

// Suppresses the same sound retriggering faster than `minimumIntervalSeconds`, which keeps
// repeated events (for example several hover changes in one frame) from stacking.
class SoundThrottle {
public:
    explicit SoundThrottle(double minimumIntervalSeconds = 0.045);

    bool allow(SoundId id, double nowSeconds);

private:
    std::array<double, soundCount> lastPlayed_;
    double interval_;
};

} // namespace audio
