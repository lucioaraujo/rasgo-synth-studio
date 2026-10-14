#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "CompositionIdentity.hpp"
#include "GamePlan.hpp"

namespace rasgo {

enum class RelationVerb {
    Follow,
    Transform,
    Counter,
    Aggress,
    Conceal,
    Negotiate
};

enum class HarmonyDisposition {
    None,
    Accept,
    Tension,
    Resist,
    Reconfigure
};

inline const char *relationVerbName(RelationVerb verb) {
    static const char *names[] = {
        "follow", "transform", "counter", "aggress", "conceal", "negotiate"
    };
    return names[static_cast<std::size_t>(verb)];
}

inline const char *harmonyDispositionName(HarmonyDisposition disposition) {
    static const char *names[] = {
        "none", "accept", "tension", "resist", "reconfigure"
    };
    return names[static_cast<std::size_t>(disposition)];
}

struct RhythmObservation {
    bool manifested = false;
    double rms = 0.0;
    double peak = 0.0;
    double audibleRatio = 0.0;
    double attackDensity = 0.0;
    double pulseDeviation = 0.0;
    double pressure = 0.0;
};

struct HarmonyObservation {
    int rootMidiNote = 57;
    std::array<int, 4> chordOffsets {0, 3, 7, 10};
    double tension = 0.0;
};

struct OrchestraDecision {
    RhythmObservation rhythm;
    bool thereminResponded = false;
    bool harmonyResponded = false;
    bool consequence = false;
    double receivedPressure = 0.0;
    double thereminTargetMidi = 69.0;
    double thereminGainScale = 1.0;
    double thereminPresence = 1.0;
    HarmonyDisposition harmonyDisposition = HarmonyDisposition::None;
    int harmonyShiftSemitones = 0;
    double rhythmVelocityScale = 1.0;
    int rhythmDelaySamples = 0;
};

// First small implementation of the Orquestra Relacional RASGO. It does not
// offer a universal "anything modulates anything" matrix. It represents one
// explicit triangular relation: the audible drum bus is observed; theremin
// interprets that gesture according to its own sensitivity and memory; harmony
// may accept, tension, resist or reconfigure; the consequence returns to the
// next bar of rhythm. Participants keep different clocks and pitches.
//
// The class owns no audio generator and performs no allocation in the sample
// path. observeRhythmSample() only measures what the drum bus actually emitted.
// finishRhythmBar()/interpretBar() run at bar rate and expose every causal step
// so PieceRenderer can archive and route it without hiding the relation here.
class IntermodularOrchestra {
public:
    IntermodularOrchestra(unsigned seed, const CompositionIdentity &identity,
                          const GamePlan &game, double sampleRate, bool enabled)
        : seed_(seed), identity_(identity), game_(game), enabled_(enabled) {
        sampleRate_ = std::max(8000.0, sampleRate);
        verb_ = verbForStrategy(game.strategy);
        sensitivity_ = std::clamp(
            0.18 + 0.34 * identityUnit(seed_, 501) + 0.20 * game.hiddenInformation,
            0.12, 0.82);
        resistance_ = std::clamp(
            0.22 + 0.46 * game.memory - 0.24 * game.risk
            + 0.16 * identityUnit(seed_, 502), 0.08, 0.88);
        permeability_ = std::clamp(
            0.30 + 0.42 * game.freedom + 0.20 * identityUnit(seed_, 503),
            0.18, 0.96);
        // Presence is a degree of orchestral availability, not a mandatory
        // entrance/exit script. Some identities begin sustained, others latent,
        // and the lived relation may or may not move them afterwards.
        presenceAffinity_ = std::clamp(
            0.04 + 0.84 * identityUnit(seed_, 504)
            + (identity.leadVoice == LeadVoiceModel::Theremin ? 0.18 : 0.0),
            0.0, 1.0);
        presencePersistence_ = std::clamp(
            0.12 + 0.78 * identityUnit(seed_, 505)
            + 0.08 * game.memory, 0.08, 0.98);
        const double initialDisposition = identityUnit(seed_, 506);
        currentThereminPresence_ = initialDisposition < 0.14
            ? 0.0
            : std::clamp(0.22 * initialDisposition
                       + 0.78 * presenceAffinity_, 0.0, 1.0);
        if (!enabled_) currentThereminPresence_ = 1.0;
        presenceWasAudible_ = currentThereminPresence_ >= presenceAudibilityThreshold_;
        attackCooldownSamples_ = static_cast<int>(sampleRate_ * 0.018);
    }

    bool enabled() const { return enabled_; }
    RelationVerb verb() const { return verb_; }
    double sensitivity() const { return sensitivity_; }
    double resistance() const { return resistance_; }
    double permeability() const { return permeability_; }
    double memory() const { return memory_; }
    double fatigue() const { return fatigue_; }
    double currentThereminTargetMidi() const { return currentThereminTargetMidi_; }
    double currentThereminGainScale() const { return currentThereminGainScale_; }
    double currentThereminPresence() const { return currentThereminPresence_; }
    double presenceAffinity() const { return presenceAffinity_; }
    double presencePersistence() const { return presencePersistence_; }
    double currentRhythmVelocityScale() const { return currentRhythmVelocityScale_; }
    int currentRhythmDelaySamples() const { return currentRhythmDelaySamples_; }
    int rhythmObservationCount() const { return rhythmObservationCount_; }
    int thereminResponseCount() const { return thereminResponseCount_; }
    int harmonyResponseCount() const { return harmonyResponseCount_; }
    int harmonyReconfigurationCount() const { return harmonyReconfigurationCount_; }
    int rhythmFeedbackCount() const { return rhythmFeedbackCount_; }
    int resistanceCount() const { return resistanceCount_; }
    int thereminPresentBarCount() const { return thereminPresentBarCount_; }
    int thereminSilentBarCount() const { return thereminSilentBarCount_; }
    int thereminEntryCount() const { return thereminEntryCount_; }
    int thereminWithdrawalCount() const { return thereminWithdrawalCount_; }

    void observeRhythmSample(float sample) noexcept {
        if (!enabled_) return;
        const double finite = std::isfinite(sample) ? static_cast<double>(sample) : 0.0;
        const double magnitude = std::min(4.0, std::fabs(finite));
        sumSquares_ += magnitude * magnitude;
        peak_ = std::max(peak_, magnitude);
        ++sampleCount_;
        if (magnitude > 0.003) ++audibleSampleCount_;

        if (attackCooldown_ > 0) --attackCooldown_;
        const bool onset = magnitude > 0.012
            && magnitude > envelope_ * 1.55 + 0.004
            && attackCooldown_ == 0;
        if (onset) {
            ++attackCount_;
            attackCooldown_ = attackCooldownSamples_;
        }
        envelope_ = magnitude > envelope_
            ? magnitude : envelope_ * 0.9965;
    }

    RhythmObservation finishRhythmBar(double pulseDeviation) noexcept {
        RhythmObservation observation;
        observation.pulseDeviation = std::clamp(pulseDeviation, -1.0, 1.0);
        if (!enabled_ || sampleCount_ == 0) {
            resetRhythmAccumulator();
            return observation;
        }

        observation.rms = std::sqrt(sumSquares_ / static_cast<double>(sampleCount_));
        observation.peak = peak_;
        observation.audibleRatio = static_cast<double>(audibleSampleCount_)
                                  / static_cast<double>(sampleCount_);
        observation.attackDensity = std::clamp(
            static_cast<double>(attackCount_) / 18.0, 0.0, 1.0);
        observation.manifested = observation.rms > 0.002
                              && observation.audibleRatio > 0.002;
        const double energy = std::clamp(observation.rms / 0.18, 0.0, 1.0);
        observation.pressure = observation.manifested
            ? std::clamp(0.46 * energy
                       + 0.30 * observation.attackDensity
                       + 0.14 * observation.audibleRatio
                       + 0.10 * std::fabs(observation.pulseDeviation), 0.0, 1.0)
            : 0.0;
        if (observation.manifested) ++rhythmObservationCount_;
        resetRhythmAccumulator();
        return observation;
    }

    OrchestraDecision interpretBar(const RhythmObservation &rhythm,
                                    const HarmonyObservation &harmony,
                                    double transportedPressure,
                                    int barIndex) noexcept {
        OrchestraDecision decision;
        decision.rhythm = rhythm;
        decision.thereminTargetMidi = currentThereminTargetMidi_;
        decision.thereminGainScale = currentThereminGainScale_;
        decision.thereminPresence = currentThereminPresence_;
        decision.rhythmVelocityScale = currentRhythmVelocityScale_;
        decision.rhythmDelaySamples = currentRhythmDelaySamples_;
        if (!enabled_) return decision;

        if (!initializedTarget_) {
            currentThereminTargetMidi_ = static_cast<double>(harmony.rootMidiNote + 12);
            initializedTarget_ = true;
        }

        if (cooldownBars_ > 0) --cooldownBars_;
        fatigue_ *= 0.78;
        memory_ *= 0.94;
        currentRhythmVelocityScale_ += (1.0 - currentRhythmVelocityScale_) * 0.32;
        currentRhythmDelaySamples_ = static_cast<int>(
            std::round(static_cast<double>(currentRhythmDelaySamples_) * 0.55));

        const double pressure = std::clamp(transportedPressure, 0.0, 1.5);
        decision.receivedPressure = pressure;
        const double effectiveSensitivity = std::clamp(
            sensitivity_ + fatigue_ * 0.24 - memory_ * 0.16, 0.08, 0.94);
        if (!rhythm.manifested || cooldownBars_ > 0 || pressure < effectiveSensitivity) {
            updatePresence(rhythm, HarmonyDisposition::None, pressure, false, barIndex);
            decision.thereminTargetMidi = currentThereminTargetMidi_;
            decision.thereminGainScale = currentThereminGainScale_;
            decision.thereminPresence = currentThereminPresence_;
            decision.rhythmVelocityScale = currentRhythmVelocityScale_;
            decision.rhythmDelaySamples = currentRhythmDelaySamples_;
            return decision;
        }

        const std::uint64_t eventKey = static_cast<std::uint64_t>(thereminResponseCount_)
                                     + static_cast<std::uint64_t>(std::max(0, barIndex)) * 17ULL;
        const std::size_t chordIndex = static_cast<std::size_t>(
            identityBits(seed_, 510 + eventKey) % harmony.chordOffsets.size());
        const int direction = (identityBits(seed_, 520 + eventKey) & 1ULL) == 0ULL ? -1 : 1;
        int relationalInterval = intervalForVerb(verb_, rhythm, direction, eventKey);
        int registerOffset = registerForVerb(verb_, eventKey);
        double proposedMidi = static_cast<double>(harmony.rootMidiNote
            + harmony.chordOffsets[chordIndex] + registerOffset + relationalInterval);
        proposedMidi = std::clamp(proposedMidi, 40.0, 96.0);

        decision.thereminResponded = true;
        decision.thereminTargetMidi = proposedMidi;
        ++thereminResponseCount_;
        memory_ = std::clamp(0.72 * memory_ + 0.28 * pressure, 0.0, 1.0);
        fatigue_ = std::clamp(fatigue_ + 0.18 + 0.22 * pressure, 0.0, 1.0);
        cooldownBars_ = 1 + static_cast<int>(identityBits(seed_, 530 + eventKey) % 3ULL);

        const double penetration = pressure * permeability_
                                 + memory_ * 0.22
                                 - resistance_
                                 - harmony.tension * 0.16;
        decision.harmonyResponded = true;
        ++harmonyResponseCount_;

        switch (verb_) {
            case RelationVerb::Follow:
                decision.harmonyDisposition = penetration > 0.18
                    ? HarmonyDisposition::Accept : HarmonyDisposition::Tension;
                break;
            case RelationVerb::Transform:
                decision.harmonyDisposition = penetration > 0.02
                    ? HarmonyDisposition::Reconfigure : HarmonyDisposition::Tension;
                break;
            case RelationVerb::Counter:
                decision.harmonyDisposition = penetration > 0.28
                    ? HarmonyDisposition::Tension : HarmonyDisposition::Resist;
                break;
            case RelationVerb::Aggress:
                decision.harmonyDisposition = penetration > -0.05
                    ? HarmonyDisposition::Reconfigure : HarmonyDisposition::Resist;
                break;
            case RelationVerb::Conceal:
                decision.harmonyDisposition = penetration > 0.32
                    ? HarmonyDisposition::Accept : HarmonyDisposition::Resist;
                break;
            case RelationVerb::Negotiate:
                decision.harmonyDisposition = penetration > 0.08
                    ? HarmonyDisposition::Accept : HarmonyDisposition::Tension;
                break;
        }

        const int signedDistance = shortestPitchClassDistance(
            static_cast<int>(std::round(proposedMidi)), harmony.rootMidiNote);
        if (decision.harmonyDisposition == HarmonyDisposition::Reconfigure) {
            int maximumShift = verb_ == RelationVerb::Aggress ? 4 : 3;
            decision.harmonyShiftSemitones = std::clamp(signedDistance, -maximumShift, maximumShift);
            if (decision.harmonyShiftSemitones == 0)
                decision.harmonyShiftSemitones = direction;
            ++harmonyReconfigurationCount_;
            resistance_ = std::clamp(resistance_ - 0.06 * pressure, 0.04, 0.96);
        } else if (decision.harmonyDisposition == HarmonyDisposition::Resist) {
            ++resistanceCount_;
            resistance_ = std::clamp(resistance_ + 0.04, 0.04, 0.96);
            memory_ = std::clamp(memory_ + 0.08, 0.0, 1.0);
        } else if (decision.harmonyDisposition == HarmonyDisposition::Accept) {
            resistance_ = std::clamp(resistance_ - 0.025, 0.04, 0.96);
        }

        applyReturnToRhythm(decision, pressure);
        currentThereminTargetMidi_ = proposedMidi;
        currentThereminGainScale_ = gainScaleForDisposition(decision.harmonyDisposition);
        updatePresence(rhythm, decision.harmonyDisposition, pressure, true, barIndex);
        currentRhythmVelocityScale_ = decision.rhythmVelocityScale;
        currentRhythmDelaySamples_ = decision.rhythmDelaySamples;
        decision.thereminGainScale = currentThereminGainScale_;
        decision.thereminPresence = currentThereminPresence_;
        decision.consequence = decision.harmonyDisposition != HarmonyDisposition::None;
        return decision;
    }

private:
    static RelationVerb verbForStrategy(GameStrategy strategy) {
        switch (strategy) {
            case GameStrategy::Follow: return RelationVerb::Follow;
            case GameStrategy::Transform: return RelationVerb::Transform;
            case GameStrategy::Counter: return RelationVerb::Counter;
            case GameStrategy::Interrupt: return RelationVerb::Aggress;
            case GameStrategy::Conceal: return RelationVerb::Conceal;
            case GameStrategy::Negotiate: return RelationVerb::Negotiate;
        }
        return RelationVerb::Follow;
    }

    int intervalForVerb(RelationVerb verb, const RhythmObservation &rhythm,
                        int direction, std::uint64_t key) const {
        const int agitation = rhythm.attackDensity > 0.55 ? 2 : 1;
        switch (verb) {
            case RelationVerb::Follow:
                return direction * (rhythm.pulseDeviation > 0.22 ? 2 : 0);
            case RelationVerb::Transform:
                return direction * (1 + agitation);
            case RelationVerb::Counter:
                return direction * ((identityBits(seed_, 540 + key) & 1ULL) == 0ULL ? 5 : 6);
            case RelationVerb::Aggress:
                return direction * (rhythm.peak > 0.35 ? 7 : 1 + agitation);
            case RelationVerb::Conceal:
                return -12 + direction * (rhythm.attackDensity > 0.35 ? 1 : 0);
            case RelationVerb::Negotiate:
                return direction * ((identityBits(seed_, 550 + key) & 1ULL) == 0ULL ? 2 : 5);
        }
        return 0;
    }

    int registerForVerb(RelationVerb verb, std::uint64_t key) const {
        if (verb == RelationVerb::Conceal) return 12;
        if (verb == RelationVerb::Aggress)
            return (identityBits(seed_, 560 + key) & 1ULL) == 0ULL ? 12 : 24;
        return (identityBits(seed_, 570 + key) % 3ULL == 0ULL) ? 24 : 12;
    }

    static int shortestPitchClassDistance(int note, int root) {
        int distance = (note - root) % 12;
        if (distance < -6) distance += 12;
        if (distance > 6) distance -= 12;
        return distance;
    }

    double gainScaleForDisposition(HarmonyDisposition disposition) const {
        switch (disposition) {
            case HarmonyDisposition::None: return 1.0;
            case HarmonyDisposition::Accept: return 1.04 + 0.18 * memory_;
            case HarmonyDisposition::Tension: return 0.92 + 0.22 * game_.risk;
            case HarmonyDisposition::Resist: return 0.58 + 0.22 * game_.freedom;
            case HarmonyDisposition::Reconfigure: return 1.10 + 0.20 * game_.energy;
        }
        return 1.0;
    }

    void updatePresence(const RhythmObservation &rhythm,
                        HarmonyDisposition disposition,
                        double pressure, bool interpreted,
                        int barIndex) noexcept {
        // No fixed window and no guaranteed withdrawal: affinity supplies a
        // personal resting tendency, persistence determines how reluctantly
        // the body changes, while events only exert pressure on that state.
        double desired = presenceAffinity_;
        if (rhythm.manifested) {
            desired += (rhythm.attackDensity - 0.42) * 0.18;
            desired += (pressure - 0.50) * 0.12;
        }
        if (interpreted) {
            desired += 0.10 + 0.18 * memory_;
            switch (disposition) {
                case HarmonyDisposition::Accept: desired += 0.10; break;
                case HarmonyDisposition::Tension: desired += 0.03; break;
                case HarmonyDisposition::Resist: desired -= 0.22; break;
                case HarmonyDisposition::Reconfigure: desired += 0.16; break;
                case HarmonyDisposition::None: break;
            }
        } else if (!rhythm.manifested) {
            // Silence permits rest but does not demand disappearance.
            desired -= 0.05 * (1.0 - game_.memory);
        }
        if (verb_ == RelationVerb::Conceal) desired -= 0.12;
        if (verb_ == RelationVerb::Aggress && interpreted) desired += 0.08;

        // A small deterministic local fluctuation prevents presence from being
        // merely another static gain. It cannot by itself dictate a formal
        // entrance or departure.
        const std::uint64_t temporalKey = 580ULL
            + static_cast<std::uint64_t>(std::max(0, barIndex)) * 19ULL;
        desired += (identityUnit(seed_, temporalKey) - 0.5)
                 * (0.06 + 0.08 * game_.freedom);
        desired = std::clamp(desired, 0.0, 1.0);

        const double changeFreedom = 1.0 - presencePersistence_;
        const double rate = desired > currentThereminPresence_
            ? 0.10 + 0.54 * changeFreedom
            : 0.05 + 0.38 * changeFreedom;
        currentThereminPresence_ += (desired - currentThereminPresence_) * rate;
        if (currentThereminPresence_ < 0.035) currentThereminPresence_ = 0.0;
        if (currentThereminPresence_ > 0.985) currentThereminPresence_ = 1.0;

        const bool audible = currentThereminPresence_ >= presenceAudibilityThreshold_;
        if (audible) ++thereminPresentBarCount_;
        else ++thereminSilentBarCount_;
        if (audible && !presenceWasAudible_) ++thereminEntryCount_;
        if (!audible && presenceWasAudible_) ++thereminWithdrawalCount_;
        presenceWasAudible_ = audible;
    }

    void applyReturnToRhythm(OrchestraDecision &decision, double pressure) {
        switch (decision.harmonyDisposition) {
            case HarmonyDisposition::Accept:
                decision.rhythmVelocityScale = std::clamp(0.90 + 0.28 * memory_, 0.75, 1.25);
                decision.rhythmDelaySamples = 0;
                break;
            case HarmonyDisposition::Tension:
                decision.rhythmVelocityScale = std::clamp(1.0 + 0.25 * pressure, 0.8, 1.35);
                decision.rhythmDelaySamples = static_cast<int>(sampleRate_ * (0.002 + 0.007 * pressure));
                break;
            case HarmonyDisposition::Resist:
                decision.rhythmVelocityScale = std::clamp(0.72 + 0.18 * game_.risk, 0.65, 1.0);
                decision.rhythmDelaySamples = static_cast<int>(sampleRate_ * (0.006 + 0.012 * resistance_));
                break;
            case HarmonyDisposition::Reconfigure:
                decision.rhythmVelocityScale = std::clamp(1.08 + 0.24 * game_.energy, 1.0, 1.38);
                decision.rhythmDelaySamples = static_cast<int>(sampleRate_ * 0.003 * fatigue_);
                break;
            case HarmonyDisposition::None:
                break;
        }
        if (decision.harmonyDisposition != HarmonyDisposition::None)
            ++rhythmFeedbackCount_;
    }

    void resetRhythmAccumulator() noexcept {
        sumSquares_ = 0.0;
        peak_ = 0.0;
        sampleCount_ = 0;
        audibleSampleCount_ = 0;
        attackCount_ = 0;
    }

    unsigned seed_ = 0;
    CompositionIdentity identity_{};
    GamePlan game_{};
    bool enabled_ = false;
    RelationVerb verb_ = RelationVerb::Follow;
    double sampleRate_ = 48000.0;
    double sensitivity_ = 0.5;
    double resistance_ = 0.5;
    double permeability_ = 0.5;
    double memory_ = 0.0;
    double fatigue_ = 0.0;
    int cooldownBars_ = 0;
    double presenceAffinity_ = 1.0;
    double presencePersistence_ = 1.0;
    double currentThereminPresence_ = 1.0;
    static constexpr double presenceAudibilityThreshold_ = 0.075;
    bool presenceWasAudible_ = true;

    double sumSquares_ = 0.0;
    double peak_ = 0.0;
    std::size_t sampleCount_ = 0;
    std::size_t audibleSampleCount_ = 0;
    int attackCount_ = 0;
    int attackCooldown_ = 0;
    int attackCooldownSamples_ = 864;
    double envelope_ = 0.0;

    bool initializedTarget_ = false;
    double currentThereminTargetMidi_ = 69.0;
    double currentThereminGainScale_ = 1.0;
    double currentRhythmVelocityScale_ = 1.0;
    int currentRhythmDelaySamples_ = 0;

    int rhythmObservationCount_ = 0;
    int thereminResponseCount_ = 0;
    int harmonyResponseCount_ = 0;
    int harmonyReconfigurationCount_ = 0;
    int rhythmFeedbackCount_ = 0;
    int resistanceCount_ = 0;
    int thereminPresentBarCount_ = 0;
    int thereminSilentBarCount_ = 0;
    int thereminEntryCount_ = 0;
    int thereminWithdrawalCount_ = 0;
};

} // namespace rasgo
