#pragma once

#include <algorithm>
#include <cmath>

#include "PerformerState.hpp"

namespace rasgo {

enum class PitchApproach {
    Direct,
    Scoop,
    Overshoot,
    Undershoot,
    UnstableAcquisition
};

enum class NoteReleaseGesture {
    Clean,
    Breath,
    Fall,
    Choked,
    Resonant
};

// SCORE: what the compositional layer prescribes.
struct NotePrescription {
    double targetMidi = 60.0;
    double durationSeconds = 0.5;
    double dynamic = 0.5;
    double phrasePosition = 0.0;
    bool phraseBoundary = false;
};

struct NotePerformanceContext {
    double previousMidi = 60.0;
    double connection = 0.5;
    double phraseEnergy = 0.5;
    double difficulty = 0.25;
};

// INTERPRETATION: what the performer is trying to do. It remains abstract and
// instrument-neutral; no field is an oscillator or gain command.
struct NoteIntention {
    double targetMidi = 60.0;
    double durationSeconds = 0.5;
    double effort = 0.5;
    double projection = 0.5;
    double connection = 0.5;
    double expressiveRange = 0.5;
    double intendedIntegrity = 1.0;

    bool valid() const noexcept {
        return finiteRange(targetMidi, 0.0, 127.0)
            && finiteRange(durationSeconds, 0.0, 30.0)
            && unit(effort) && unit(projection) && unit(connection)
            && unit(expressiveRange) && unit(intendedIntegrity);
    }

private:
    static bool finiteRange(double value, double low, double high) noexcept {
        return std::isfinite(value) && value >= low && value <= high;
    }
    static bool unit(double value) noexcept { return finiteRange(value, 0.0, 1.0); }
};

// PERFORMANCE PLAN: a bounded trajectory requested from an instrument voice.
// Actual realization must be observed separately after DSP; this plan never
// claims that an intended gesture really happened.
struct NoteMorphologyPlan {
    double preparation = 0.0;
    double onsetCertainty = 1.0;
    double attackSpeed = 0.5;
    double attackHardness = 0.5;
    double attackNoise = 0.0;
    PitchApproach pitchApproach = PitchApproach::Direct;
    double approachCents = 0.0;
    double overshootCents = 0.0;
    double settlingSeconds = 0.0;
    double initialIntensity = 0.5;
    double peakIntensity = 0.5;
    double peakPosition = 0.5;
    double finalIntensity = 0.5;
    double spectralBrightness = 0.5;
    double spectralNoise = 0.0;
    double vibratoDelaySeconds = 0.0;
    double vibratoRateHz = 5.2;
    double vibratoDepthCents = 0.0;
    double vibratoAsymmetry = 0.0;
    double connection = 0.5;
    double overlap = 0.0;
    NoteReleaseGesture releaseGesture = NoteReleaseGesture::Clean;
    double releaseSeconds = 0.1;
    double integrity = 1.0;
    double failureLikelihood = 0.0;
    double reactionLatencySeconds = 0.0;
    double recoveryStrength = 0.0;

    bool valid() const noexcept {
        return unit(preparation) && unit(onsetCertainty) && unit(attackSpeed)
            && unit(attackHardness) && unit(attackNoise)
            && finiteRange(approachCents, -240.0, 240.0)
            && finiteRange(overshootCents, -120.0, 120.0)
            && finiteRange(settlingSeconds, 0.0, 2.0)
            && unit(initialIntensity) && unit(peakIntensity) && unit(peakPosition)
            && unit(finalIntensity) && unit(spectralBrightness) && unit(spectralNoise)
            && finiteRange(vibratoDelaySeconds, 0.0, 4.0)
            && finiteRange(vibratoRateHz, 0.0, 12.0)
            && finiteRange(vibratoDepthCents, 0.0, 80.0)
            && finiteRange(vibratoAsymmetry, -1.0, 1.0)
            && unit(connection) && finiteRange(overlap, -1.0, 1.0)
            && finiteRange(releaseSeconds, 0.0, 4.0)
            && unit(integrity) && unit(failureLikelihood)
            && finiteRange(reactionLatencySeconds, 0.0, 2.0)
            && unit(recoveryStrength);
    }

private:
    static bool finiteRange(double value, double low, double high) noexcept {
        return std::isfinite(value) && value >= low && value <= high;
    }
    static bool unit(double value) noexcept { return finiteRange(value, 0.0, 1.0); }
};

// The edge between two events has its own morphology. It is intentionally
// separate from either note so legato, rupture and all intermediate states can
// be expressed without forcing a binary articulation preset.
struct TransitionPlan {
    double temporalContinuity = 0.5;
    double pitchContinuity = 0.5;
    double dynamicContinuity = 0.5;
    double timbralContinuity = 0.5;
    double overlap = 0.0;
    double glideCents = 0.0;
    double crossfade = 0.0;

    bool valid() const noexcept {
        return unit(temporalContinuity) && unit(pitchContinuity)
            && unit(dynamicContinuity) && unit(timbralContinuity)
            && finiteRange(overlap, -1.0, 1.0)
            && finiteRange(glideCents, -240.0, 240.0)
            && unit(crossfade);
    }

private:
    static bool finiteRange(double value, double low, double high) noexcept {
        return std::isfinite(value) && value >= low && value <= high;
    }
    static bool unit(double value) noexcept { return finiteRange(value, 0.0, 1.0); }
};

class NoteMorphology {
public:
    static NoteIntention intend(const NotePrescription &prescription,
                                const NotePerformanceContext &context,
                                const PerformerState &performer) noexcept {
        const double dynamic = clamp01(prescription.dynamic);
        const double phraseEnergy = clamp01(context.phraseEnergy);
        const double difficulty = clamp01(context.difficulty);
        NoteIntention out;
        out.targetMidi = std::clamp(finite(prescription.targetMidi, 60.0), 0.0, 127.0);
        out.durationSeconds = std::clamp(
            finite(prescription.durationSeconds, 0.5), 0.0, 30.0);
        out.effort = clamp01(dynamic * 0.52 + performer.effort * 0.28
                             + phraseEnergy * 0.20);
        out.projection = clamp01(dynamic * 0.45 + out.effort * 0.32
                                 + performer.confidence * 0.23);
        out.connection = clamp01(context.connection * 0.66
                                  + performer.attention * 0.18
                                  + (1.0 - difficulty) * 0.16);
        out.expressiveRange = clamp01(performer.expressivity * 0.52
                                      + performer.risk * 0.24
                                      + phraseEnergy * 0.24);
        out.intendedIntegrity = clamp01(performer.precision * 0.44
                                        + performer.attention * 0.30
                                        + performer.confidence * 0.26
                                        - difficulty * 0.16);
        return out;
    }

    static NoteMorphologyPlan plan(const NotePrescription &prescription,
                                   const NotePerformanceContext &context,
                                   const PerformerState &performer,
                                   const NoteIntention &intention) noexcept {
        const double difficulty = clamp01(context.difficulty);
        const double phrase = clamp01(prescription.phrasePosition);
        const double interval = intention.targetMidi
            - std::clamp(finite(context.previousMidi, intention.targetMidi), 0.0, 127.0);
        const double intervalMagnitude = std::min(std::fabs(interval) / 12.0, 1.0);
        const double direction = interval < 0.0 ? -1.0 : 1.0;
        const double fragility = clamp01((1.0 - performer.confidence) * 0.55
                                         + performer.fatigue * 0.45);

        NoteMorphologyPlan out;
        out.preparation = clamp01(difficulty * 0.48 + intervalMagnitude * 0.24
                                  + fragility * 0.28);
        out.onsetCertainty = clamp01(intention.intendedIntegrity * 0.58
                                     + performer.confidence * 0.24
                                     + performer.attention * 0.18
                                     - performer.fatigue * 0.14);
        out.attackSpeed = clamp01(0.24 + intention.effort * 0.48
                                  + performer.confidence * 0.16
                                  - out.preparation * 0.10);
        out.attackHardness = clamp01(intention.effort * 0.52
                                     + intention.projection * 0.30
                                     + performer.tension * 0.18);
        out.attackNoise = clamp01((1.0 - out.onsetCertainty) * 0.46
                                  + intention.effort * 0.16
                                  + performer.fatigue * 0.24
                                  + performer.risk * 0.14);

        const double acquisitionInstability = clamp01(
            difficulty * 0.30 + intervalMagnitude * 0.26
            + (1.0 - performer.precision) * 0.30 + performer.risk * 0.14);
        if (acquisitionInstability < 0.20) {
            out.pitchApproach = PitchApproach::Direct;
        } else if (performer.confidence < 0.35) {
            out.pitchApproach = PitchApproach::UnstableAcquisition;
        } else if (intervalMagnitude > 0.55 && performer.risk > 0.55) {
            out.pitchApproach = PitchApproach::Overshoot;
        } else if (direction > 0.0) {
            out.pitchApproach = PitchApproach::Scoop;
        } else {
            out.pitchApproach = PitchApproach::Undershoot;
        }
        const double approachMagnitude = std::min(
            8.0 + 54.0 * acquisitionInstability + 42.0 * intervalMagnitude, 160.0);
        out.approachCents = out.pitchApproach == PitchApproach::Direct ? 0.0
                          : out.pitchApproach == PitchApproach::Overshoot
                              ? direction * approachMagnitude * 0.35
                              : -direction * approachMagnitude;
        out.overshootCents = out.pitchApproach == PitchApproach::Overshoot
            ? direction * std::min(8.0 + performer.risk * 34.0, 48.0) : 0.0;
        out.settlingSeconds = std::clamp(
            0.015 + acquisitionInstability * 0.28
            + performer.fatigue * 0.12, 0.0, 2.0);

        const double dynamic = clamp01(prescription.dynamic);
        out.initialIntensity = clamp01(dynamic * (0.54 + intention.effort * 0.28));
        out.peakIntensity = clamp01(out.initialIntensity
                                    + intention.expressiveRange * (0.16 + intention.effort * 0.18));
        out.peakPosition = clamp01(0.30 + phrase * 0.38
                                   + (1.0 - performer.energy) * 0.10);
        const double endingRelaxation = prescription.phraseBoundary ? 0.42 : 0.16;
        out.finalIntensity = clamp01(out.peakIntensity * (1.0 - endingRelaxation)
                                     - performer.fatigue * 0.08);
        out.spectralBrightness = clamp01(intention.effort * 0.46
                                         + intention.projection * 0.30
                                         + performer.tension * 0.16
                                         + performer.energy * 0.08);
        out.spectralNoise = clamp01(out.attackNoise * 0.58
                                    + performer.fatigue * 0.24
                                    + performer.risk * 0.18);

        const double duration = std::clamp(intention.durationSeconds, 0.0, 30.0);
        out.vibratoDelaySeconds = std::min(duration * (0.18 + fragility * 0.20), 4.0);
        out.vibratoRateHz = std::clamp(4.3 + performer.energy * 1.4
                                      + performer.tension * 0.8
                                      - performer.fatigue * 0.6, 0.0, 12.0);
        out.vibratoDepthCents = std::clamp(
            intention.expressiveRange * (7.0 + performer.tension * 18.0)
            * std::min(duration / 0.45, 1.0), 0.0, 80.0);
        out.vibratoAsymmetry = std::clamp(
            (performer.tension - performer.precision) * 0.35, -1.0, 1.0);

        out.connection = clamp01(intention.connection);
        out.overlap = std::clamp((intention.connection - 0.5) * 1.6
                                 - difficulty * 0.20, -1.0, 1.0);
        if (prescription.phraseBoundary && performer.fatigue > 0.55) {
            out.releaseGesture = NoteReleaseGesture::Breath;
        } else if (prescription.phraseBoundary && performer.risk > 0.62) {
            out.releaseGesture = NoteReleaseGesture::Fall;
        } else if (out.connection < 0.28) {
            out.releaseGesture = NoteReleaseGesture::Choked;
        } else if (out.connection > 0.72) {
            out.releaseGesture = NoteReleaseGesture::Resonant;
        }
        out.releaseSeconds = std::clamp(
            duration * (0.08 + out.connection * 0.24)
            + (prescription.phraseBoundary ? 0.12 : 0.0), 0.0, 4.0);

        out.integrity = clamp01(out.onsetCertainty * 0.40
                                + performer.precision * 0.30
                                + performer.attention * 0.20
                                + intention.intendedIntegrity * 0.10
                                - acquisitionInstability * 0.14);
        out.failureLikelihood = clamp01((1.0 - out.integrity) * 0.62
                                        + difficulty * 0.20
                                        + performer.risk * intention.effort * 0.18);
        out.reactionLatencySeconds = std::clamp(
            0.015 + (1.0 - performer.attention) * 0.24
            + performer.fatigue * 0.16, 0.0, 2.0);
        out.recoveryStrength = clamp01(performer.attention * 0.46
                                       + performer.confidence * 0.30
                                       + intention.effort * 0.24);
        return out;
    }

    static TransitionPlan transition(const NoteMorphologyPlan &previous,
                                     const NoteMorphologyPlan &next,
                                     double intervalMidi,
                                     double connection) noexcept {
        const double interval = std::clamp(std::isfinite(intervalMidi)
                                               ? std::fabs(intervalMidi) / 12.0 : 1.0,
                                           0.0, 1.0);
        const double link = clamp01(connection);
        TransitionPlan out;
        out.temporalContinuity = clamp01(link * 0.62
                                         + previous.connection * 0.18
                                         + next.connection * 0.20);
        out.pitchContinuity = clamp01(link * 0.48
                                      + (1.0 - interval) * 0.32
                                      + next.onsetCertainty * 0.20);
        out.dynamicContinuity = clamp01(link * 0.44
                                        + (1.0 - std::fabs(previous.finalIntensity
                                                           - next.initialIntensity)) * 0.36
                                        + next.connection * 0.20);
        out.timbralContinuity = clamp01(link * 0.40
                                        + (1.0 - std::fabs(previous.spectralBrightness
                                                           - next.spectralBrightness)) * 0.42
                                        + next.connection * 0.18);
        out.overlap = std::clamp((out.temporalContinuity - 0.5) * 1.4
                                 + next.connection * 0.18, -1.0, 1.0);
        out.glideCents = std::clamp(intervalMidi * (out.pitchContinuity - 0.5) * 18.0,
                                    -240.0, 240.0);
        out.crossfade = clamp01(out.timbralContinuity * 0.62
                                + out.dynamicContinuity * 0.38);
        return out;
    }

private:
    static double finite(double value, double fallback) noexcept {
        return std::isfinite(value) ? value : fallback;
    }
    static double clamp01(double value) noexcept {
        return std::clamp(finite(value, 0.0), 0.0, 1.0);
    }
};

} // namespace rasgo
