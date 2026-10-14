#pragma once
#include <algorithm>
#include <cmath>

namespace rasgo {
struct KitInterpretation {
    double decay = 1.0, brightness = 1.0, attack = 1.0, body = 1.0, noise = 1.0;
    static KitInterpretation interpret(double material, double articulation, double velocity) {
        const auto finite = [](double value, double fallback) { return std::isfinite(value) ? value : fallback; };
        const double color = std::clamp(finite(material, 0), -0.8, 0.8);
        const double gesture = std::clamp(finite(articulation, 0.5), 0.0, 1.0);
        const double strength = std::clamp(finite(velocity, 0.5), 0.0, 1.0);
        KitInterpretation out;
        out.decay = std::clamp(1.40 - gesture * 0.85 + color * 0.25, 0.45, 1.75);
        out.brightness = std::clamp(0.90 + color * 0.40 + strength * 0.14, 0.55, 1.40);
        out.attack = std::clamp(0.70 + gesture * 0.65 + strength * 0.12, 0.55, 1.50);
        out.body = std::clamp(1.08 - color * 0.25 + strength * 0.12, 0.75, 1.45);
        out.noise = std::clamp(0.72 + color * 0.32 + gesture * 0.10, 0.40, 1.10);
        return out;
    }
};
} // namespace rasgo
