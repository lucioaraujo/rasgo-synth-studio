#pragma once

#include <algorithm>

namespace rasgo {

// Phase 168: a deterministic accent translator. It describes a bounded
// expressive plan; it does not write directly to a voice or audio buffer.
struct RasgarAccentInput {
    double metric = 0.0;
    double structural = 0.0;
    double relational = 0.0;
    double phrase = 0.0;
    double density = 0.0;
};

struct RasgarAccentPlan {
    double velocity = 0.0;
    double gate = 1.0;
    double microPitchSemitones = 0.0;
    double timbre = 0.0;
};

class RasgarAccentField {
public:
    RasgarAccentPlan evaluate(const RasgarAccentInput &input,
                              double baseVelocity = 0.7,
                              double baseGate = 0.75,
                              double timbreBudget = 0.25) const noexcept {
        const double metric = clampUnit(input.metric);
        const double structural = clampUnit(input.structural);
        const double relational = clampUnit(input.relational);
        const double phrase = clampUnit(input.phrase);
        const double density = clampUnit(input.density);
        const double accent = clampUnit(
            metric * 0.35 + structural * 0.25 + relational * 0.20
            + phrase * 0.20);

        RasgarAccentPlan result;
        result.velocity = clampUnit(baseVelocity + (accent - 0.5) * 0.35
                                    + (density - 0.5) * 0.10);
        result.gate = std::clamp(baseGate + (accent - 0.5) * 0.20,
                                 0.05, 1.0);
        result.microPitchSemitones = std::clamp(
            (relational - metric) * 0.12 + (phrase - 0.5) * 0.06,
            -0.25, 0.25);
        result.timbre = std::clamp(
            (structural * 0.6 + phrase * 0.4) * std::max(0.0, timbreBudget),
            0.0, std::max(0.0, timbreBudget));
        return result;
    }

private:
    static double clampUnit(double value) noexcept {
        return std::clamp(value, 0.0, 1.0);
    }
};

} // namespace rasgo
