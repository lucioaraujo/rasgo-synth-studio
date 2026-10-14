#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace rasgo {

enum class PieceRenderPhase { Composition, Finalizing, Complete };

// The exact first coda frame satisfying the existing renderer's strict
// thermal-stop predicate. This only counts work; it does not change the DSP.
inline std::uint64_t abruptCodaFrameLimit(double sampleRate, double tau) {
    if (!std::isfinite(sampleRate) || !std::isfinite(tau) || sampleRate <= 0.0 || tau <= 0.0)
        return std::numeric_limits<std::uint64_t>::max();
    const double bound = std::ceil(std::max(0.2, tau * std::log(100.0)) * sampleRate) + 2.0;
    if (!std::isfinite(bound) || bound >= static_cast<double>(std::numeric_limits<std::uint64_t>::max()))
        return std::numeric_limits<std::uint64_t>::max();
    std::uint64_t low = 1, high = static_cast<std::uint64_t>(bound);
    auto ends = [=](std::uint64_t frames) {
        const double seconds = static_cast<double>(frames) / sampleRate;
        return seconds > 0.2 && std::exp(-seconds / tau) < 0.01;
    };
    while (low < high) {
        const auto middle = low + (high - low) / 2;
        if (ends(middle)) high = middle;
        else low = middle + 1;
    }
    return low;
}

} // namespace rasgo
