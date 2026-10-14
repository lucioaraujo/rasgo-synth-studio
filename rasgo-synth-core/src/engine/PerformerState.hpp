#pragma once

#include <algorithm>
#include <cmath>

namespace rasgo {

// Slow latent state shared by consecutive gestures. These values do not map
// directly to gain, pitch or timing; an instrument-specific interpreter must
// translate them into bounded destinations appropriate to its own material.
struct PerformerState {
    double energy = 0.65;
    double effort = 0.45;
    double confidence = 0.60;
    double precision = 0.70;
    double tension = 0.35;
    double fatigue = 0.10;
    double risk = 0.35;
    double expressivity = 0.55;
    double attention = 0.75;

    bool valid() const noexcept {
        return bounded(energy) && bounded(effort) && bounded(confidence)
            && bounded(precision) && bounded(tension) && bounded(fatigue)
            && bounded(risk) && bounded(expressivity) && bounded(attention);
    }

private:
    static bool bounded(double value) noexcept {
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    }
};

struct PerformerObservation {
    double noteDurationSeconds = 0.5;
    double restSeconds = 0.0;
    double density = 0.5;
    double requestedEffort = 0.5;
    double difficulty = 0.25;
    double realizedIntegrity = 1.0;
    bool phraseBoundary = false;
    bool recoveredFromFailure = false;
};

// Deterministic, bounded state evolution. It carries consequence from one
// note to the next without deciding notes or synthesizing a performer persona.
class PerformerStateModel {
public:
    static PerformerState advance(const PerformerState &previous,
                                  const PerformerObservation &observation) noexcept {
        PerformerState out = sanitize(previous);
        const double duration = clampNonnegative(observation.noteDurationSeconds, 12.0);
        const double rest = clampNonnegative(observation.restSeconds, 12.0);
        const double density = clamp01(observation.density);
        const double requestedEffort = clamp01(observation.requestedEffort);
        const double difficulty = clamp01(observation.difficulty);
        const double integrity = clamp01(observation.realizedIntegrity);

        const double work = std::clamp(duration / 4.0, 0.0, 1.0)
            * (0.25 + 0.75 * requestedEffort)
            * (0.45 + 0.55 * density);
        const double recovery = std::clamp(rest / 3.0, 0.0, 1.0)
            + (observation.phraseBoundary ? 0.12 : 0.0);

        out.effort = smooth(out.effort, requestedEffort, 0.24);
        out.energy = clamp01(out.energy - work * 0.16 + recovery * 0.24);
        out.fatigue = clamp01(out.fatigue + work * (0.13 + difficulty * 0.09)
                              - recovery * 0.28);

        const double confidenceTarget = clamp01(
            integrity * 0.58 + out.attention * 0.22
            + (observation.recoveredFromFailure ? 0.12 : 0.0)
            - difficulty * 0.18 - out.fatigue * 0.12);
        out.confidence = smooth(out.confidence, confidenceTarget,
                                observation.recoveredFromFailure ? 0.28 : 0.14);

        const double attentionTarget = clamp01(
            0.72 + recovery * 0.22 - out.fatigue * 0.42
            - density * 0.12 + (1.0 - difficulty) * 0.08);
        out.attention = smooth(out.attention, attentionTarget, 0.18);

        const double precisionTarget = clamp01(
            out.confidence * 0.48 + out.attention * 0.38
            + (1.0 - out.fatigue) * 0.24 - difficulty * 0.18);
        out.precision = smooth(out.precision, precisionTarget, 0.16);

        const double tensionTarget = clamp01(
            difficulty * 0.42 + requestedEffort * 0.25
            + out.risk * 0.20 + out.fatigue * 0.18);
        out.tension = smooth(out.tension, tensionTarget, 0.17);

        const double riskTarget = clamp01(
            previous.risk * 0.64 + out.confidence * 0.20
            + out.expressivity * 0.16 - out.fatigue * 0.12);
        out.risk = smooth(out.risk, riskTarget, 0.10);

        const double expressionTarget = clamp01(
            previous.expressivity * 0.62 + out.energy * 0.18
            + out.risk * 0.12 + out.tension * 0.08);
        out.expressivity = smooth(out.expressivity, expressionTarget, 0.10);
        return out;
    }

private:
    static double clamp01(double value) noexcept {
        return std::clamp(std::isfinite(value) ? value : 0.0, 0.0, 1.0);
    }

    static double clampNonnegative(double value, double maximum) noexcept {
        return std::clamp(std::isfinite(value) ? value : 0.0, 0.0, maximum);
    }

    static double smooth(double current, double target, double rate) noexcept {
        return clamp01(current + (target - current) * rate);
    }

    static PerformerState sanitize(const PerformerState &state) noexcept {
        PerformerState out = state;
        out.energy = clamp01(out.energy);
        out.effort = clamp01(out.effort);
        out.confidence = clamp01(out.confidence);
        out.precision = clamp01(out.precision);
        out.tension = clamp01(out.tension);
        out.fatigue = clamp01(out.fatigue);
        out.risk = clamp01(out.risk);
        out.expressivity = clamp01(out.expressivity);
        out.attention = clamp01(out.attention);
        return out;
    }
};

} // namespace rasgo
