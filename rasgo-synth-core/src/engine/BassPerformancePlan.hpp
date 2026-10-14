#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "BassPhrasePlan.hpp"
#include "NoteMorphology.hpp"
#include "TemporalGrooveImprint.hpp"

namespace rasgo {

enum class BassArticulationIntent : std::uint8_t {
    Rest,
    Rearticulate,
    Connected,
    Ghost,
    Dead,
    Slide,
    Accent,
    Count
};

enum class BassImperfectionIntent : std::uint8_t {
    None,
    Partial,
    Choked,
    MissedAttack,
    BrokenPitch,
    DoubleTrigger,
    Count
};

struct BassPerformanceRequest {
    BassPhrasePlan phrase{};
    TemporalGrooveImprint groove{};
    PerformerState performer{};
    double stepDurationSeconds = 0.12;
    double previousSemitoneOffset = 0.0;
    double controlledImperfection = 0.25;
    bool hasObservedFeedback = false;
    PerformerObservation previousObservation{};
};

struct BassPerformanceStep {
    BassStepIntent sourceIntent = BassStepIntent::Rest;
    int semitoneOffset = 0;
    BassRegisterIntent registerIntent = BassRegisterIntent::Low;
    BassKickRelation kickRelation = BassKickRelation::Independent;
    BassArticulationIntent articulation = BassArticulationIntent::Rest;
    BassImperfectionIntent imperfection = BassImperfectionIntent::None;
    double timingOffsetSteps = 0.0;
    double gate = 0.0;
    double attackRenewal = 0.0;
    double slide = 0.0;
    double pitchGestureCents = 0.0;
    double dynamicStart = 0.0;
    double dynamicPeak = 0.0;
    double dynamicPeakPosition = 0.5;
    double dynamicEnd = 0.0;
    double pressure = 0.0;
    double weight = 0.0;
    double brightness = 0.0;
    double noise = 0.0;
    double drive = 0.0;
    double integrity = 1.0;
    double imperfectionLikelihood = 0.0;
    double reactionLatencySeconds = 0.0;
    double recoveryStrength = 0.0;

    bool active() const noexcept {
        return articulation != BassArticulationIntent::Rest;
    }

    bool valid() const noexcept {
        return sourceIntent < BassStepIntent::Count
            && registerIntent < BassRegisterIntent::Count
            && kickRelation < BassKickRelation::Count
            && articulation < BassArticulationIntent::Count
            && imperfection < BassImperfectionIntent::Count
            && semitoneOffset >= -24 && semitoneOffset <= 24
            && finiteRange(timingOffsetSteps, -0.25, 0.25)
            && unit(gate) && unit(attackRenewal) && unit(slide)
            && finiteRange(pitchGestureCents, -240.0, 240.0)
            && unit(dynamicStart) && unit(dynamicPeak)
            && unit(dynamicPeakPosition) && unit(dynamicEnd)
            && unit(pressure) && unit(weight) && unit(brightness)
            && unit(noise) && unit(drive) && unit(integrity)
            && unit(imperfectionLikelihood)
            && finiteRange(reactionLatencySeconds, 0.0, 2.0)
            && unit(recoveryStrength)
            && (active() || neutralRest());
    }

private:
    bool neutralRest() const noexcept {
        return sourceIntent == BassStepIntent::Rest && gate == 0.0
            && attackRenewal == 0.0 && slide == 0.0
            && pitchGestureCents == 0.0 && dynamicStart == 0.0
            && dynamicPeak == 0.0 && dynamicEnd == 0.0
            && pressure == 0.0 && weight == 0.0 && brightness == 0.0
            && noise == 0.0 && drive == 0.0
            && imperfection == BassImperfectionIntent::None;
    }

    static bool finiteRange(double value, double minimum, double maximum) noexcept {
        return std::isfinite(value) && value >= minimum && value <= maximum;
    }
    static bool unit(double value) noexcept {
        return finiteRange(value, 0.0, 1.0);
    }
};

struct BassPerformancePlan {
    static constexpr std::size_t kMaxSteps = BassPhrasePlan::kMaxSteps;

    bool sourceValid = false;
    BassFunctionRole role = BassFunctionRole::Silence;
    std::array<BassPerformanceStep, kMaxSteps> steps{};
    std::size_t stepCount = 0;
    std::size_t activeStepCount = 0;
    int grooveApplicationCount = 0;
    int ghostCount = 0;
    int deadCount = 0;
    int slideCount = 0;
    int imperfectionCount = 0;
    double averagePressure = 0.0;
    double averageWeight = 0.0;
    PerformerState performerAtPlan{};
    bool realizationFeedbackRequired = false;

    bool silent() const noexcept { return activeStepCount == 0; }

    bool valid() const noexcept {
        if (!sourceValid || role >= BassFunctionRole::Count
            || stepCount == 0 || stepCount > kMaxSteps
            || activeStepCount > stepCount || !performerAtPlan.valid()
            || grooveApplicationCount < 0 || ghostCount < 0 || deadCount < 0
            || slideCount < 0 || imperfectionCount < 0
            || !unit(averagePressure) || !unit(averageWeight))
            return false;
        std::size_t counted = 0;
        for (std::size_t i = 0; i < stepCount; ++i) {
            if (!steps[i].valid()) return false;
            counted += steps[i].active() ? 1u : 0u;
        }
        if (counted != activeStepCount) return false;
        if (realizationFeedbackRequired != (activeStepCount > 0)) return false;
        return role == BassFunctionRole::Silence ? silent() : !silent();
    }

private:
    static bool unit(double value) noexcept {
        return std::isfinite(value) && value >= 0.0 && value <= 1.0;
    }
};

// Converts phrase intention into correlated performance requests. It never
// chooses a synth body and never treats a planned imperfection as a realized
// event; only explicit observed feedback may update PerformerState.
class BassPerformancePlanner {
public:
    static BassPerformancePlan plan(const BassPerformanceRequest &request) noexcept {
        BassPerformancePlan out;
        if (!request.phrase.valid()
            || !request.groove.validFor(request.phrase.stepCount))
            return out;

        const double stepDuration = std::clamp(
            finite(request.stepDurationSeconds, 0.12), 0.02, 2.0);
        const double imperfectionBudget = unit(request.controlledImperfection);
        PerformerState performer = sanitize(request.performer);
        if (request.hasObservedFeedback) {
            performer = PerformerStateModel::advance(
                performer, request.previousObservation);
        }

        out.sourceValid = true;
        out.role = request.phrase.role;
        out.stepCount = request.phrase.stepCount;
        out.performerAtPlan = performer;

        double previousOffset = std::clamp(
            finite(request.previousSemitoneOffset, 0.0), -24.0, 24.0);
        double pressureSum = 0.0;
        double weightSum = 0.0;
        for (std::size_t i = 0; i < out.stepCount; ++i) {
            const auto &source = request.phrase.steps[i];
            if (!source.active()) continue;
            auto &event = out.steps[i];
            event.sourceIntent = source.intent;
            event.semitoneOffset = source.semitoneOffset;
            event.registerIntent = source.registerIntent;
            event.kickRelation = source.kickRelation;

            const TemporalGrooveStep &groove = request.groove.steps[i];
            const double grooveStrength = request.groove.enabled
                ? request.groove.strength : 0.0;
            event.timingOffsetSteps = std::clamp(
                source.timingOffsetSteps
                    + groove.timingOffsetSteps * grooveStrength,
                -0.25, 0.25);
            event.gate = std::clamp(source.gate
                * blend(1.0, groove.gateScale, grooveStrength), 0.05, 1.0);
            const double accent = clamp01(source.weight
                + groove.accentDelta * grooveStrength * 0.24);

            const double interval = static_cast<double>(source.semitoneOffset)
                - previousOffset;
            const double difficulty = clamp01(std::fabs(interval) / 12.0 * 0.52
                + (source.registerIntent == BassRegisterIntent::LowMid ? 0.12 : 0.0)
                + request.phrase.risk * 0.18
                + (1.0 - performer.precision) * 0.18);
            const double phrasePosition = out.stepCount > 1
                ? static_cast<double>(i) / static_cast<double>(out.stepCount - 1u)
                : 0.0;
            const double phraseArc = 1.0 - std::fabs(2.0 * phrasePosition - 1.0);

            NotePrescription prescription;
            // Sixty is only a calculation anchor for the relative interval;
            // the plan does not prescribe a sounding register.
            prescription.targetMidi = 60.0 + source.semitoneOffset;
            prescription.durationSeconds = stepDuration * event.gate;
            prescription.dynamic = accent;
            prescription.phrasePosition = phrasePosition;
            prescription.phraseBoundary = i + 1u == out.stepCount
                && request.phrase.boundary != BassBoundaryGesture::None;
            NotePerformanceContext context;
            context.previousMidi = 60.0 + previousOffset;
            context.connection = connectionFor(source.intent, event.gate);
            context.phraseEnergy = clamp01(phraseArc * 0.44
                + request.phrase.tension * 0.32 + performer.energy * 0.24);
            context.difficulty = difficulty;
            const NoteIntention intention = NoteMorphology::intend(
                prescription, context, performer);
            const NoteMorphologyPlan morphology = NoteMorphology::plan(
                prescription, context, performer, intention);

            event.attackRenewal = clamp01(renewalFor(source.intent)
                + groove.attackDelta * grooveStrength * 0.22
                + (performer.confidence - 0.5) * 0.10);
            event.articulation = articulationFor(
                source.intent, event.attackRenewal, request.phrase.risk,
                request.phrase.realizedSignature, i);
            event.slide = clamp01(source.intent == BassStepIntent::Slide ? 0.82
                : event.articulation == BassArticulationIntent::Connected
                    ? context.connection * std::min(std::fabs(interval) / 12.0, 1.0)
                    : 0.0);
            event.pitchGestureCents = std::clamp(
                morphology.approachCents + morphology.overshootCents,
                -240.0, 240.0);

            const double ghostScale = event.articulation == BassArticulationIntent::Dead
                ? 0.16 : event.articulation == BassArticulationIntent::Ghost
                    ? 0.42 : 1.0;
            event.dynamicStart = clamp01(morphology.initialIntensity * ghostScale);
            event.dynamicPeak = clamp01(morphology.peakIntensity * ghostScale);
            event.dynamicPeakPosition = morphology.peakPosition;
            event.dynamicEnd = clamp01(morphology.finalIntensity * ghostScale);

            const double effort = clamp01(intention.effort * 0.54
                + accent * 0.24 + performer.effort * 0.22);
            event.pressure = clamp01(effort * 0.46
                + request.phrase.tension * 0.24
                + performer.tension * 0.18 + performer.risk * 0.12);
            event.weight = clamp01(request.phrase.gravity * 0.46
                + accent * 0.24 + performer.energy * 0.18
                + (source.registerIntent == BassRegisterIntent::Sub ? 0.12 : 0.0));
            event.brightness = clamp01(morphology.spectralBrightness * 0.58
                + event.pressure * 0.24 + effort * 0.18);
            event.noise = clamp01(morphology.spectralNoise * 0.54
                + (event.articulation == BassArticulationIntent::Ghost ? 0.22 : 0.0)
                + (event.articulation == BassArticulationIntent::Dead ? 0.46 : 0.0));
            event.drive = clamp01(event.pressure * 0.46
                + request.phrase.tension * 0.24 + performer.risk * 0.18
                + effort * 0.12);

            event.integrity = clamp01(morphology.integrity
                - (event.articulation == BassArticulationIntent::Dead ? 0.16 : 0.0));
            event.imperfectionLikelihood = clamp01(
                morphology.failureLikelihood * 0.72
                + performer.fatigue * 0.12 + request.phrase.risk * 0.16);
            if (unsignedUnit(request.phrase.realizedSignature, 900u + i)
                < event.imperfectionLikelihood * imperfectionBudget) {
                event.imperfection = imperfectionFor(
                    morphology, difficulty, performer, event.gate,
                    request.phrase.realizedSignature, i);
                ++out.imperfectionCount;
            }
            event.reactionLatencySeconds = morphology.reactionLatencySeconds;
            event.recoveryStrength = morphology.recoveryStrength;

            ++out.activeStepCount;
            if (request.groove.enabled && !groove.neutral())
                ++out.grooveApplicationCount;
            if (event.articulation == BassArticulationIntent::Ghost)
                ++out.ghostCount;
            if (event.articulation == BassArticulationIntent::Dead)
                ++out.deadCount;
            if (event.articulation == BassArticulationIntent::Slide
                || event.slide > 0.05)
                ++out.slideCount;
            pressureSum += event.pressure;
            weightSum += event.weight;
            previousOffset = static_cast<double>(source.semitoneOffset);
        }

        if (out.activeStepCount > 0) {
            out.averagePressure = pressureSum
                / static_cast<double>(out.activeStepCount);
            out.averageWeight = weightSum
                / static_cast<double>(out.activeStepCount);
            out.realizationFeedbackRequired = true;
        }
        return out;
    }

private:
    static double renewalFor(BassStepIntent intent) noexcept {
        switch (intent) {
            case BassStepIntent::Hold: return 0.10;
            case BassStepIntent::Slide: return 0.24;
            case BassStepIntent::Ghost: return 0.52;
            case BassStepIntent::Accent: return 0.94;
            case BassStepIntent::ApproachBelow:
            case BassStepIntent::ApproachAbove: return 0.66;
            case BassStepIntent::Fill:
            case BassStepIntent::Mutate: return 0.82;
            case BassStepIntent::Root:
            case BassStepIntent::ChordTone: return 0.74;
            case BassStepIntent::Rest:
            case BassStepIntent::Count: return 0.0;
        }
        return 0.0;
    }

    static double connectionFor(BassStepIntent intent, double gate) noexcept {
        if (intent == BassStepIntent::Hold) return 0.92;
        if (intent == BassStepIntent::Slide) return 0.86;
        if (intent == BassStepIntent::Ghost) return 0.18;
        if (intent == BassStepIntent::Accent) return 0.34;
        return clamp01(0.28 + gate * 0.52);
    }

    static BassArticulationIntent articulationFor(
        BassStepIntent intent, double renewal, double risk,
        std::uint64_t signature, std::size_t step) noexcept {
        if (intent == BassStepIntent::Ghost)
            return unsignedUnit(signature, 600u + step) < 0.30 + risk * 0.24
                ? BassArticulationIntent::Dead : BassArticulationIntent::Ghost;
        if (intent == BassStepIntent::Hold)
            return BassArticulationIntent::Connected;
        if (intent == BassStepIntent::Slide)
            return BassArticulationIntent::Slide;
        if (intent == BassStepIntent::Accent)
            return BassArticulationIntent::Accent;
        return renewal < 0.42 ? BassArticulationIntent::Connected
                              : BassArticulationIntent::Rearticulate;
    }

    static BassImperfectionIntent imperfectionFor(
        const NoteMorphologyPlan &morphology, double difficulty,
        const PerformerState &performer, double gate,
        std::uint64_t signature, std::size_t step) noexcept {
        if (morphology.onsetCertainty < 0.38)
            return BassImperfectionIntent::MissedAttack;
        if (gate < 0.24)
            return BassImperfectionIntent::Choked;
        if (difficulty > 0.62)
            return BassImperfectionIntent::BrokenPitch;
        const double detail = unsignedUnit(signature, 1100u + step);
        if (performer.risk > 0.68 && detail > 0.72)
            return BassImperfectionIntent::DoubleTrigger;
        return detail < 0.52 ? BassImperfectionIntent::Partial
                             : BassImperfectionIntent::Choked;
    }

    static double blend(double from, double to, double amount) noexcept {
        return from + (to - from) * unit(amount);
    }

    static double finite(double value, double fallback) noexcept {
        return std::isfinite(value) ? value : fallback;
    }

    static double unit(double value) noexcept {
        return std::clamp(finite(value, 0.0), 0.0, 1.0);
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

    static std::uint64_t mix64(std::uint64_t value) noexcept {
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30u)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27u)) * 0x94d049bb133111ebULL;
        return value ^ (value >> 31u);
    }

    static double unsignedUnit(std::uint64_t base, std::uint64_t salt) noexcept {
        const std::uint64_t bits = mix64(base ^ mix64(salt));
        return static_cast<double>(bits >> 11u) * (1.0 / 9007199254740992.0);
    }
};

} // namespace rasgo
