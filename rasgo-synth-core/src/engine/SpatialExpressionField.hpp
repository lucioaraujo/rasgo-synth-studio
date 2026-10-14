#pragma once

#include <algorithm>
#include <cmath>

namespace rasgo {

struct SpatialExpressionInput {
    double polarity = 0.0;
    double depth = 0.0;
    double drift = 0.0;
    double tension = 0.0;
    double monoSafety = 1.0;
};

struct SpatialFrame {
    double left = 0.0;
    double right = 0.0;
};

struct SpatialExpressionPlan {
    double pan = 0.0;
    double width = 0.0;
    double depth = 0.0;
    double driftRate = 0.0;

    bool valid() const noexcept {
        return std::isfinite(pan) && pan >= -1.0 && pan <= 1.0
            && std::isfinite(width) && width >= 0.0 && width <= 0.85
            && std::isfinite(depth) && depth >= 0.0 && depth <= 1.0
            && std::isfinite(driftRate) && driftRate >= -0.12 && driftRate <= 0.12;
    }
};

// Spatial intent contract. It is mono-conscious and produces destinations for
// a later renderer; it never performs panning, delay, allocation, or I/O.
class SpatialExpressionField {
public:
    static SpatialFrame applyWidth(SpatialFrame frame, double width) noexcept {
        const double boundedWidth = std::clamp(std::isfinite(width) ? width : 0.0, 0.0, 0.85);
        const double mid = (frame.left + frame.right) * 0.5;
        const double side = (frame.left - frame.right) * 0.5 * (1.0 + boundedWidth * 0.35);
        return {mid + side, mid - side};
    }

    SpatialExpressionPlan interpret(const SpatialExpressionInput &input) const noexcept {
        const double polarity = clampSigned(input.polarity);
        const double depth = clamp01(input.depth);
        const double drift = clamp01(input.drift);
        const double tension = clamp01(input.tension);
        const double monoSafety = clamp01(input.monoSafety);

        SpatialExpressionPlan result;
        const double safety = 0.25 + monoSafety * 0.75;
        result.pan = std::clamp(polarity * (0.45 + tension * 0.25) * safety, -1.0, 1.0);
        result.width = std::clamp((0.18 + depth * 0.48 + tension * 0.12) * safety, 0.0, 0.85);
        result.depth = depth;
        result.driftRate = std::clamp(polarity * drift * (0.025 + tension * 0.095) * safety,
                                      -0.12, 0.12);
        return result;
    }

private:
    static double clamp01(double value) noexcept {
        return std::clamp(std::isfinite(value) ? value : 0.0, 0.0, 1.0);
    }

    static double clampSigned(double value) noexcept {
        return std::clamp(std::isfinite(value) ? value : 0.0, -1.0, 1.0);
    }
};

} // namespace rasgo
