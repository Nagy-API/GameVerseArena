#include "AudioPolicy.hpp"

#include <stdexcept>

namespace audio {

std::size_t chooseVoice(const std::vector<bool>& playing, const std::vector<std::uint64_t>& startOrder)
{
    if (playing.empty() || playing.size() != startOrder.size()) {
        throw std::invalid_argument("Voice pool state is inconsistent");
    }
    for (std::size_t index = 0; index < playing.size(); ++index) {
        if (!playing[index]) return index;
    }
    std::size_t oldest = 0;
    for (std::size_t index = 1; index < startOrder.size(); ++index) {
        if (startOrder[index] < startOrder[oldest]) oldest = index;
    }
    return oldest;
}

SoundThrottle::SoundThrottle(double minimumIntervalSeconds) : interval_(minimumIntervalSeconds)
{
    lastPlayed_.fill(-std::numeric_limits<double>::infinity());
}

bool SoundThrottle::allow(SoundId id, double nowSeconds)
{
    const auto index = static_cast<std::size_t>(id);
    if (index >= lastPlayed_.size()) return false;
    if (nowSeconds - lastPlayed_[index] < interval_) return false;
    lastPlayed_[index] = nowSeconds;
    return true;
}

} // namespace audio
