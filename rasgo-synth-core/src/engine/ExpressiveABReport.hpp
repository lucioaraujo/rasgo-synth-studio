#pragma once

#include <cmath>
#include <cstdint>

#include "ExpressiveInterpretation.hpp"

namespace rasgo {

struct ExpressiveABReport {
    bool sameSeed = false;
    bool baselineValid = false;
    bool variantValid = false;
    bool variantEnabled = false;
    bool changed = false;
    double livingPitchDelta = 0.0;
    double materialDelta = 0.0;
    double spatialDelta = 0.0;

    bool valid() const noexcept {
        return baselineValid && variantValid && std::isfinite(livingPitchDelta)
            && std::isfinite(materialDelta) && std::isfinite(spatialDelta);
    }
};

inline ExpressiveABReport compareExpressiveInterpretations(
    std::uint32_t baselineSeed,
    const ExpressiveInterpretation &baseline,
    std::uint32_t variantSeed,
    const ExpressiveInterpretation &variant) noexcept {
    ExpressiveABReport report;
    report.sameSeed = baselineSeed == variantSeed;
    report.baselineValid = baseline.valid();
    report.variantValid = variant.valid();
    report.variantEnabled = variant.enabled;
    report.livingPitchDelta = variant.melodic.livingMidi - baseline.melodic.livingMidi;
    report.materialDelta = variant.material.spectralEnergy - baseline.material.spectralEnergy
        + variant.material.noiseAmount - baseline.material.noiseAmount
        + variant.material.degradation - baseline.material.degradation;
    report.spatialDelta = variant.spatial.pan - baseline.spatial.pan
        + variant.spatial.width - baseline.spatial.width
        + variant.spatial.driftRate - baseline.spatial.driftRate;
    report.changed = std::fabs(report.livingPitchDelta) > 1.0e-9
        || std::fabs(report.materialDelta) > 1.0e-9
        || std::fabs(report.spatialDelta) > 1.0e-9;
    return report;
}

} // namespace rasgo
