#pragma once

#include <algorithm>
#include <cmath>
#include "../../rasgo-synth-core/src/engine/RenderProgress.hpp"

namespace rasgo::ui {

// Message-thread state, independent of JUCE and DSP. The renderer supplies
// counted work; wall time and completed-frame percentage are not an ETA.
struct RenderActivity {
    static constexpr double indeterminate = -1.0;
    double startedMs = 0.0;
    double audioSeconds = 0.0;
    bool album = false;
    bool calibration = false;
    int trackIndex = 0;
    int trackCount = 1;
    std::uint64_t completedFrames = 0, totalFrames = 0;
    PieceRenderPhase phase = PieceRenderPhase::Composition;

    void begin(double nowMs, bool isAlbum = false, int tracks = 1) noexcept {
        startedMs = nowMs;
        audioSeconds = 0.0;
        album = isAlbum;
        calibration = isAlbum;
        trackIndex = 0;
        trackCount = std::max(1, tracks);
        completedFrames = totalFrames = 0;
        phase = PieceRenderPhase::Composition;
    }

    void observe(double seconds, int index = 0, int count = 1,
                 bool isCalibration = false) noexcept {
        audioSeconds = std::isfinite(seconds) ? std::max(0.0, seconds) : 0.0;
        trackCount = std::max(1, count);
        trackIndex = std::clamp(index, 0, trackCount - 1);
        calibration = isCalibration;
    }

    double elapsedSeconds(double nowMs) const noexcept {
        return std::isfinite(nowMs) ? std::max(0.0, (nowMs - startedMs) / 1000.0) : 0.0;
    }

    void observeWork(std::uint64_t completed, std::uint64_t total,
                     PieceRenderPhase currentPhase) noexcept {
        completedFrames = completed;
        totalFrames = total;
        phase = currentPhase;
    }

    double compositionFraction() const noexcept {
        return totalFrames > 0 ? std::min(1.0, static_cast<double>(completedFrames) / totalFrames)
                               : indeterminate;
    }

    int compositionPercent() const noexcept {
        // Floor, not rounding: never display 100% while samples remain.
        return totalFrames > 0 ? static_cast<int>(std::floor(100.0 * compositionFraction())) : 0;
    }
};

} // namespace rasgo::ui
