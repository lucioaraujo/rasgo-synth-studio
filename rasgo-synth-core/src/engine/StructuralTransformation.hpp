#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace rasgo {

enum class StructuralTransformationKind : std::uint8_t { Variation, Glitch, Rupture };

struct StructuralTransformationInput {
    StructuralTransformationKind kind = StructuralTransformationKind::Variation;
    double continuityBefore = 0.5;
    double continuityAfter = 0.5;
    double magnitude = 0.0;
    double residual = 0.0;
    bool declared = true;
};

struct StructuralTransformationPlan {
    StructuralTransformationKind kind = StructuralTransformationKind::Variation;
    double continuityBefore = 0.5;
    double continuityAfter = 0.5;
    double magnitude = 0.0;
    double residual = 0.0;
    bool declared = false;

    bool valid() const noexcept {
        return declared && std::isfinite(continuityBefore)
            && continuityBefore >= 0.0 && continuityBefore <= 1.0
            && std::isfinite(continuityAfter) && continuityAfter >= 0.0
            && continuityAfter <= 1.0 && std::isfinite(magnitude)
            && magnitude >= 0.0 && magnitude <= 1.0 && std::isfinite(residual)
            && residual >= 0.0 && residual <= 1.0
            && (kind != StructuralTransformationKind::Rupture
                || (magnitude >= 0.35 && residual >= 0.10));
    }
};

// Structural history contract. It records what changed and what remained
// afterward; it does not itself mutate a sequence or choose a sound event.
class StructuralTransformation {
public:
    StructuralTransformationPlan interpret(const StructuralTransformationInput &input) const noexcept {
        StructuralTransformationPlan result;
        result.kind = input.kind;
        result.continuityBefore = clamp01(input.continuityBefore);
        result.continuityAfter = clamp01(input.continuityAfter);
        result.magnitude = clamp01(input.magnitude);
        result.residual = clamp01(input.residual);
        result.declared = input.declared;

        if (result.kind == StructuralTransformationKind::Variation)
            result.residual = std::min(result.residual, 0.35);
        if (result.kind == StructuralTransformationKind::Glitch)
            result.residual = std::max(result.residual, result.magnitude * 0.15);
        return result;
    }

private:
    static double clamp01(double value) noexcept {
        return std::clamp(std::isfinite(value) ? value : 0.0, 0.0, 1.0);
    }
};

} // namespace rasgo
