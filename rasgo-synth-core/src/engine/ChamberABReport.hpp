#pragma once

#include <cmath>

#include "ChamberResponse.hpp"

namespace rasgo {

// Structural A/B report. It compares the response policy only; it does not
// claim that a parameter delta is perceptually salient or survives the mix.
struct ChamberABReport {
    double fluteGainDelta = 0.0;
    double tubaPressureDelta = 0.0;
    double voiceGainDelta = 0.0;
    double spatialPolarityDelta = 0.0;
    bool baselineValid = true;
    bool variantValid = true;

    bool changed(double epsilon = 1.0e-12) const noexcept {
        return std::fabs(fluteGainDelta) > epsilon
            || std::fabs(tubaPressureDelta) > epsilon
            || std::fabs(voiceGainDelta) > epsilon
            || std::fabs(spatialPolarityDelta) > epsilon;
    }

    bool valid() const noexcept {
        return baselineValid && variantValid
            && std::isfinite(fluteGainDelta)
            && std::isfinite(tubaPressureDelta)
            && std::isfinite(voiceGainDelta)
            && std::isfinite(spatialPolarityDelta);
    }

    static ChamberABReport compare(const ChamberResponse &baseline,
                                   const ChamberResponse &variant) noexcept {
        ChamberABReport report;
        report.fluteGainDelta = variant.fluteGain - baseline.fluteGain;
        report.tubaPressureDelta = variant.tubaPressure - baseline.tubaPressure;
        report.voiceGainDelta = variant.voiceGain - baseline.voiceGain;
        report.spatialPolarityDelta =
            variant.spatialPolarity - baseline.spatialPolarity;
        report.baselineValid = baseline.valid();
        report.variantValid = variant.valid();
        return report;
    }
};

} // namespace rasgo
