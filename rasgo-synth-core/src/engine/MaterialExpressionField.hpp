#pragma once

#include <algorithm>
#include <cmath>

namespace rasgo {

struct MaterialExpressionInput {
    double brightness = 0.5;
    double harmonicity = 0.5;
    double noise = 0.0;
    double degradation = 0.0;
    double tension = 0.0;
};

struct MaterialExpressionPlan {
    double spectralEnergy = 1.0;
    double noiseAmount = 0.0;
    double harmonicity = 0.5;
    double degradation = 0.0;
    double air = 0.0;

    bool valid() const noexcept {
        return std::isfinite(spectralEnergy) && spectralEnergy >= 0.72
            && spectralEnergy <= 1.18 && std::isfinite(noiseAmount)
            && noiseAmount >= 0.0 && noiseAmount <= 0.42
            && std::isfinite(harmonicity) && harmonicity >= 0.0
            && harmonicity <= 1.0 && std::isfinite(degradation)
            && degradation >= 0.0 && degradation <= 0.35
            && std::isfinite(air) && air >= 0.0 && air <= 0.45;
    }
};

// Instrument-neutral translation of material causes into bounded destinations.
// It does not generate sound and never acts as a global gain control.
class MaterialExpressionField {
public:
    MaterialExpressionPlan interpret(const MaterialExpressionInput &input) const noexcept {
        const double brightness = clamp01(input.brightness);
        const double harmonicity = clamp01(input.harmonicity);
        const double noise = clamp01(input.noise);
        const double degradation = clamp01(input.degradation);
        const double tension = clamp01(input.tension);

        MaterialExpressionPlan result;
        result.spectralEnergy = std::clamp(0.90 + brightness * 0.12
                                                + tension * 0.10 - degradation * 0.16,
                                            0.72, 1.18);
        result.noiseAmount = std::clamp(noise * 0.30 + tension * 0.12, 0.0, 0.42);
        result.harmonicity = std::clamp(harmonicity * (1.0 - degradation * 0.35), 0.0, 1.0);
        result.degradation = std::clamp(degradation * (0.55 + tension * 0.45), 0.0, 0.35);
        result.air = std::clamp(brightness * 0.22 + noise * 0.16 + tension * 0.07, 0.0, 0.45);
        return result;
    }

private:
    static double clamp01(double value) noexcept {
        return std::clamp(std::isfinite(value) ? value : 0.0, 0.0, 1.0);
    }
};

} // namespace rasgo
