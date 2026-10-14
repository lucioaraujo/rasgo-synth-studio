#pragma once

#include <algorithm>
#include <cmath>

namespace rasgo {

// A bounded per-event duration window. It owns no audio signal; tick() only
// answers whether the currently armed event is still articulated.
class RasgarGateWindow {
public:
    void arm(int stepSamples, double gate) noexcept {
        const int safeStepSamples = std::max(1, stepSamples);
        const double safeGate = std::clamp(gate, 0.05, 1.0);
        remainingSamples_ = std::max(
            1, static_cast<int>(std::lround(safeStepSamples * safeGate)));
    }

    bool tick() noexcept {
        if (remainingSamples_ <= 0) return false;
        --remainingSamples_;
        return true;
    }

    void clear() noexcept { remainingSamples_ = 0; }
    int remainingSamples() const noexcept { return remainingSamples_; }

private:
    int remainingSamples_ = 0;
};

} // namespace rasgo
