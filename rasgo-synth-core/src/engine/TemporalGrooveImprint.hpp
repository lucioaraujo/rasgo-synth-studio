#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace rasgo {

struct TemporalGrooveStep {
    double timingOffsetSteps = 0.0;
    double accentDelta = 0.0;
    double gateScale = 1.0;
    double attackDelta = 0.0;

    bool valid() const noexcept {
        return finiteRange(timingOffsetSteps, -0.25, 0.25)
            && finiteRange(accentDelta, -1.0, 1.0)
            && finiteRange(gateScale, 0.25, 1.75)
            && finiteRange(attackDelta, -1.0, 1.0);
    }

    bool neutral() const noexcept {
        return timingOffsetSteps == 0.0 && accentDelta == 0.0
            && gateScale == 1.0 && attackDelta == 0.0;
    }

private:
    static bool finiteRange(double value, double minimum, double maximum) noexcept {
        return std::isfinite(value) && value >= minimum && value <= maximum;
    }
};

// A fixed-capacity hand-off from the global temporal architecture. It does not
// invent swing or humanization for a voice: the renderer fills this imprint
// from the piece's grid, swing, microtiming, accent, gate and memory layers.
struct TemporalGrooveImprint {
    static constexpr std::size_t kMaxSteps = 16;

    bool enabled = false;
    double strength = 0.0;
    double swing = 0.0;
    double microtiming = 0.0;
    double humanization = 0.0;
    double accent = 0.0;
    double gate = 0.0;
    double memory = 0.0;
    std::array<TemporalGrooveStep, kMaxSteps> steps{};

    bool validFor(std::size_t stepCount) const noexcept {
        if (stepCount == 0 || stepCount > kMaxSteps
            || !unit(strength) || !unit(swing) || !unit(microtiming)
            || !unit(humanization) || !unit(accent) || !unit(gate)
            || !unit(memory))
            return false;
        for (std::size_t i = 0; i < stepCount; ++i) {
            if (!steps[i].valid()) return false;
            if (!enabled && !steps[i].neutral()) return false;
        }
        return enabled || strength == 0.0;
    }

private:
    static bool unit(double value) noexcept {
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    }
};

} // namespace rasgo
