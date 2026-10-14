#pragma once
#include "../sequencer/GenerativeArc.hpp"
#include <cmath>
#include <cstdint>
#include <limits>

namespace rasgo {

// Fit by counted frames in the same arc, including its existing thermal
// ending. Recompute only copies; apply one chosen scale before live ticks.
// Returns the closest bounded result; it does not cut audio to a stopwatch.
inline double applyDurationTarget(GenerativeArc& arc, double targetSeconds, double sampleRate,
                                 std::uint64_t codaLimit) {
    if (!std::isfinite(targetSeconds) || targetSeconds <= 0.0
        || !std::isfinite(sampleRate) || sampleRate <= 0.0) return 1.0;
    const double targetFrames = targetSeconds * sampleRate;
    if (!std::isfinite(targetFrames)) return 1.0;
    const auto original = arc;
    double low = 1.0 / 256.0, high = 64.0;
    double bestFactor = 1.0, bestDistance = std::numeric_limits<double>::infinity();
    for (int iteration = 0; iteration < 32; ++iteration) {
        const double factor = (low + high) * 0.5;
        auto preview = original;
        if (!preview.scaleDurationsBeforeRender(factor)) break;
        const auto frames = preview.remainingRenderFrames(codaLimit);
        if (frames == 0) break;
        const double distance = std::abs(static_cast<double>(frames) - targetFrames);
        if (distance < bestDistance) { bestDistance = distance; bestFactor = factor; }
        if (distance <= 0.5) break;
        if (frames < targetFrames) low = factor;
        else high = factor;
    }
    arc = original;
    arc.scaleDurationsBeforeRender(bestFactor);
    return bestFactor;
}

} // namespace rasgo
