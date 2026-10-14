#pragma once

#include <algorithm>
#include <cmath>

#include "PerformerState.hpp"

namespace rasgo {

// A hit has a physical envelope rather than a pitched-note morphology.
struct HitGesturePlan {
    double preparation = 0.0;
    double impact = 0.5;
    double body = 0.5;
    double decay = 0.5;
    double residue = 0.0;
    double integrity = 1.0;
    double failureLikelihood = 0.0;
    double recoveryStrength = 0.0;

    bool valid() const noexcept {
        return unit(preparation) && unit(impact) && unit(body) && unit(decay)
            && unit(residue) && unit(integrity) && unit(failureLikelihood)
            && unit(recoveryStrength);
    }

private:
    static bool unit(double value) noexcept {
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    }
};

// A texture has emergence, mass, spectral evolution and dissipation. It is
// not a note stretched in time and it does not receive pitch-centric rules.
struct TextureGesturePlan {
    double emergence = 0.0;
    double mass = 0.5;
    double spectralBrightness = 0.5;
    double instability = 0.0;
    double drift = 0.0;
    double memory = 0.0;
    double dissipation = 0.5;
    double integrity = 1.0;

    bool valid() const noexcept {
        return unit(emergence) && unit(mass) && unit(spectralBrightness)
            && unit(instability) && unit(drift) && unit(memory)
            && unit(dissipation) && unit(integrity);
    }

private:
    static bool unit(double value) noexcept {
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    }
};

class PerformanceGesturePlanner {
public:
    static HitGesturePlan planHit(const PerformerState &state,
                                  double metric,
                                  double density,
                                  double difficulty,
                                  double previousIntegrity = 1.0) noexcept {
        const PerformerState safe = sanitize(state);
        const double metricValue = unit(metric);
        const double densityValue = unit(density);
        const double difficultyValue = unit(difficulty);
        const double integrity = unit(previousIntegrity);
        HitGesturePlan out;
        out.preparation = clamp01(difficultyValue * 0.34
                                   + safe.tension * 0.22
                                   + safe.fatigue * 0.22
                                   + (1.0 - safe.confidence) * 0.22);
        out.impact = clamp01(metricValue * 0.42 + safe.effort * 0.34
                             + safe.tension * 0.14 + safe.risk * 0.10);
        out.body = clamp01(out.impact * 0.45 + safe.energy * 0.30
                           + densityValue * 0.15 + safe.confidence * 0.10);
        out.decay = clamp01(0.30 + safe.fatigue * 0.20
                            + (1.0 - out.impact) * 0.28
                            + difficultyValue * 0.22);
        out.residue = clamp01(safe.expressivity * 0.28 + safe.tension * 0.26
                              + densityValue * 0.20 + safe.risk * 0.26);
        out.integrity = clamp01(integrity * 0.60 + safe.precision * 0.24
                                + safe.attention * 0.16 - difficultyValue * 0.12);
        out.failureLikelihood = clamp01((1.0 - out.integrity) * 0.52
                                         + safe.fatigue * 0.22
                                         + difficultyValue * 0.16
                                         + safe.risk * 0.10);
        out.recoveryStrength = clamp01((1.0 - out.integrity) * 0.32
                                       + safe.attention * 0.32
                                       + safe.confidence * 0.22
                                       + safe.expressivity * 0.14);
        return out;
    }

    static TextureGesturePlan planTexture(const PerformerState &state,
                                          double density,
                                          double spectralPressure,
                                          double sectionMemory,
                                          double difficulty) noexcept {
        const PerformerState safe = sanitize(state);
        const double densityValue = unit(density);
        const double pressure = unit(spectralPressure);
        const double memory = unit(sectionMemory);
        const double difficultyValue = unit(difficulty);
        TextureGesturePlan out;
        out.emergence = clamp01(safe.attention * 0.24 + safe.energy * 0.26
                                + pressure * 0.22 + safe.risk * 0.14
                                + (1.0 - memory) * 0.14);
        out.mass = clamp01(densityValue * 0.42 + safe.energy * 0.28
                           + safe.effort * 0.18 + memory * 0.12);
        out.spectralBrightness = clamp01(pressure * 0.42 + safe.effort * 0.24
                                         + safe.tension * 0.18 + safe.energy * 0.16);
        out.instability = clamp01(safe.risk * 0.28 + safe.tension * 0.24
                                  + difficultyValue * 0.24
                                  + (1.0 - safe.precision) * 0.24);
        out.drift = clamp01(out.instability * 0.44 + safe.fatigue * 0.22
                            + memory * 0.18 + safe.expressivity * 0.16);
        out.memory = clamp01(memory * 0.58 + safe.attention * 0.18
                             + safe.confidence * 0.14 + safe.expressivity * 0.10);
        out.dissipation = clamp01((1.0 - safe.energy) * 0.32
                                  + safe.fatigue * 0.28
                                  + (1.0 - densityValue) * 0.20
                                  + (1.0 - safe.attention) * 0.20);
        out.integrity = clamp01(safe.precision * 0.34 + safe.attention * 0.28
                                + out.memory * 0.20 + (1.0 - difficultyValue) * 0.18);
        return out;
    }

private:
    static double unit(double value) noexcept {
        return clamp01(std::isfinite(value) ? value : 0.0);
    }

    static double clamp01(double value) noexcept {
        return std::clamp(value, 0.0, 1.0);
    }

    static PerformerState sanitize(const PerformerState &state) noexcept {
        PerformerState out = state;
        out.energy = unit(out.energy);
        out.effort = unit(out.effort);
        out.confidence = unit(out.confidence);
        out.precision = unit(out.precision);
        out.tension = unit(out.tension);
        out.fatigue = unit(out.fatigue);
        out.risk = unit(out.risk);
        out.expressivity = unit(out.expressivity);
        out.attention = unit(out.attention);
        return out;
    }
};

} // namespace rasgo
