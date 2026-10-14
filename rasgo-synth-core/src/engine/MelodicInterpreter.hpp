#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "NoteMorphology.hpp"

namespace rasgo {

enum class MelodicGesture { Legato, Staccato, Marcato, Tenuto };

struct MelodicEvent {
    double midi = 60.0;
    double duration = 0.5;
    double density = 0.5;
    double accent = 0.0;
    double phrasePosition = 0.0;
    bool phraseBoundary = false;
};

struct MelodicInterpretation {
    double targetMidi = 60.0;
    double arrivalMidi = 60.0;
    // A bounded intermediate pitch: consumers may use it for a living glide
    // without replacing the target or making the interpreter a pitch source.
    double livingMidi = 60.0;
    double phraseEnergy = 0.0;
    double breath = 1.0;
    double articulation = 1.0;
    double connection = 0.0;
    double attack = 0.5;
    double sustain = 0.5;
    double release = 0.5;
    double gestureMemory = 0.5;
    double vibratoDepth = 0.0;
    double timbreBrightness = 0.0;
    // A bounded request for the voice's internal trajectory. It is metadata
    // until an instrument adapter proves the corresponding realization.
    NoteMorphologyPlan morphology;
    MelodicGesture gesture = MelodicGesture::Legato;
    bool restAfter = false;
};

// Instrument-neutral phrase layer. It interprets events; it does not generate
// audio, choose a scale, or impose one voice's identity on another instrument.
class MelodicInterpreter {
public:
    explicit MelodicInterpreter(std::uint32_t seed = 1u) : state_(seed) {}

    MelodicInterpretation interpret(const MelodicEvent &event) noexcept {
        const double interval = event.midi - previousMidi_;
        const double magnitude = std::clamp(std::fabs(interval), 0.0, 12.0);
        const double direction = interval < 0.0 ? -1.0 : 1.0;
        const double phrase = std::clamp(event.phrasePosition, 0.0, 1.0);
        const double arc = 1.0 - std::fabs(2.0 * phrase - 1.0);
        const double freedom = 1.0 - std::clamp(event.density, 0.0, 1.0);

        breath_ = std::clamp(breath_ + 0.18 * std::max(event.duration, 0.0) - 0.12, 0.0, 1.0);
        if (event.phraseBoundary) breath_ = std::min(1.0, breath_ + 0.35);

        const double monotony = previousDirection_ == direction ? 0.08 : 0.0;
        const double continuation = std::clamp(
            0.70 - std::min(magnitude, 6.0) * 0.07 + arc * 0.10
                - monotony + (nextUnit() - 0.5) * freedom * 0.18,
            0.18, 0.84);
        const bool continueDirection = nextUnit() < continuation;
        const double interpretedDirection = continueDirection ? previousDirection_ : -previousDirection_;
        const double gestureOffset = interpretedDirection * magnitude * (0.04 + arc * 0.08);

        MelodicInterpretation result;
        result.targetMidi = event.midi;
        result.arrivalMidi = event.midi + gestureOffset;
        result.livingMidi = event.midi + gestureOffset * (0.35 + breath_ * 0.35);
        result.phraseEnergy = arc;
        result.breath = breath_;
        result.articulation = std::clamp(
            1.0 - magnitude * 0.035 + event.accent * 0.12, 0.35, 1.15);
        result.vibratoDepth = std::clamp(
            0.002 + arc * 0.008 + event.accent * 0.002, 0.0, 0.018);
        result.timbreBrightness = std::clamp(
            (event.midi - 48.0) / 36.0 + arc * 0.18 + event.accent * 0.10,
            0.0, 1.5);
        result.restAfter = event.phraseBoundary || breath_ < 0.18;

        const double roll = nextUnit();
        const double staccato = 0.10 + freedom * 0.20;
        const double marcato = 0.08 + event.accent * 0.25 + magnitude * 0.01;
        const double tenuto = 0.08 + arc * 0.18;
        const double total = 0.64 + staccato + marcato + tenuto;
        const double choice = roll * total;
        result.gesture = choice < 0.64 ? MelodicGesture::Legato
                       : choice < 0.64 + staccato ? MelodicGesture::Staccato
                       : choice < 0.64 + staccato + marcato ? MelodicGesture::Marcato
                       : MelodicGesture::Tenuto;

        const bool continuesGesture = hasPreviousGesture_ && result.gesture == previousGesture_;
        gestureMemory_ = std::clamp(gestureMemory_ * 0.78 + (continuesGesture ? 0.22 : 0.0), 0.0, 1.0);
        const double gestureAttack = result.gesture == MelodicGesture::Marcato ? 0.90
                                    : result.gesture == MelodicGesture::Staccato ? 0.72
                                    : result.gesture == MelodicGesture::Tenuto ? 0.52 : 0.38;
        const double gestureSustain = result.gesture == MelodicGesture::Staccato ? 0.28
                                     : result.gesture == MelodicGesture::Marcato ? 0.62
                                     : result.gesture == MelodicGesture::Tenuto ? 0.92 : 0.78;
        result.connection = std::clamp(
            0.32 + (continuesGesture ? 0.28 : 0.0) + result.breath * 0.16
                + (1.0 - std::min(magnitude, 12.0) / 12.0) * 0.20,
            0.0, 1.0);
        result.attack = std::clamp(gestureAttack + event.accent * 0.08, 0.0, 1.0);
        result.sustain = std::clamp(gestureSustain * (0.72 + result.breath * 0.28), 0.0, 1.0);
        result.release = std::clamp(
            (result.restAfter ? 0.88 : 0.34) + (1.0 - result.breath) * 0.12,
            0.0, 1.0);
        result.gestureMemory = gestureMemory_;

        NotePrescription prescription;
        prescription.targetMidi = event.midi;
        prescription.durationSeconds = event.duration;
        prescription.dynamic = std::clamp(0.48 + event.accent * 0.28
                                           + result.phraseEnergy * 0.24,
                                           0.0, 1.0);
        prescription.phrasePosition = phrase;
        prescription.phraseBoundary = event.phraseBoundary;
        NotePerformanceContext context;
        context.previousMidi = previousMidi_;
        context.connection = result.connection;
        context.phraseEnergy = result.phraseEnergy;
        context.difficulty = std::clamp(magnitude / 12.0
                                        + event.density * 0.18, 0.0, 1.0);
        const PerformerObservation observation {
            std::max(0.0, event.duration),
            result.restAfter ? 0.12 : 0.0,
            std::clamp(event.density, 0.0, 1.0),
            result.attack,
            context.difficulty,
            1.0,
            event.phraseBoundary,
            false
        };
        performerState_ = PerformerStateModel::advance(performerState_, observation);
        const NoteIntention intention = NoteMorphology::intend(
            prescription, context, performerState_);
        result.morphology = NoteMorphology::plan(
            prescription, context, performerState_, intention);

        previousMidi_ = event.midi;
        previousDirection_ = interpretedDirection;
        previousGesture_ = result.gesture;
        hasPreviousGesture_ = true;
        return result;
    }

    double breath() const noexcept { return breath_; }
    const PerformerState &performerState() const noexcept { return performerState_; }

    // The DSP adapter may report what actually happened after rendering. The
    // interpreter never treats its previous intention as evidence; observed
    // integrity and recovery are the only inputs that close this feedback loop.
    void observeRealization(const PerformerObservation &observation) noexcept {
        performerState_ = PerformerStateModel::advance(performerState_, observation);
    }

private:
    double nextUnit() noexcept {
        state_ = state_ * 1664525u + 1013904223u;
        return static_cast<double>(state_ >> 8u) / static_cast<double>(1u << 24u);
    }

    std::uint32_t state_ = 1u;
    double previousMidi_ = 60.0;
    double previousDirection_ = 1.0;
    double breath_ = 1.0;
    MelodicGesture previousGesture_ = MelodicGesture::Legato;
    bool hasPreviousGesture_ = false;
    double gestureMemory_ = 0.5;
    PerformerState performerState_{};
};

} // namespace rasgo
