#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "CompositionIdentity.hpp"
#include "CompositionalTopology.hpp"
#include "GamePlan.hpp"

namespace rasgo {

enum class RasgarPhase { Ground, Pressure, Tear, Propagation, Scar };
enum class RasgarMotion { Surge, Drag, Shear, Stutter, Suspension, Recoil };

inline const char *rasgarPhaseName(RasgarPhase phase) {
    static const char *names[] = {"ground", "pressure", "tear", "propagation", "scar"};
    return names[static_cast<int>(phase)];
}

inline const char *rasgarMotionName(RasgarMotion motion) {
    static const char *names[] = {"surge", "drag", "shear", "stutter", "suspension", "recoil"};
    return names[static_cast<int>(motion)];
}

struct RasgarInput {
    double density = 0.0;
    double recentEnergy = 0.0;
    double repetition = 0.0;
    bool sectionBoundary = false;
};

struct RasgarFrame {
    RasgarPhase phase = RasgarPhase::Ground;
    RasgarMotion motion = RasgarMotion::Surge;
    double strain = 0.0;
    double instability = 0.0;
    double scar = 0.0;
    double drumPresenceScale = 1.0;
    bool tearTriggered = false;
    CismResult cism;
};

// A stateful, seed-deterministic gesture. It does not choose notes or synthesize
// a sound by itself: it reads energy/repetition, accumulates strain, tears the
// temporal region and exposes several outputs which different consumers may
// interpret according to their own character.
class RasgarGesture {
public:
    RasgarGesture(unsigned seed, const CompositionIdentity &identity,
                  const GamePlan &gamePlan,
                  CompositionalTopology &topology,
                  OpenModuleArchive &archive,
                  RegionId temporalRegion,
                  RegionId rhythmRegion)
        : seed_(seed), topology_(topology), archive_(archive),
          activeTemporalRegion_(temporalRegion), rhythmRegion_(rhythmRegion) {
        const std::uint64_t identityDisplacement =
            static_cast<std::uint64_t>(static_cast<int>(identity.focus)) * 17ULL
          + static_cast<std::uint64_t>(static_cast<int>(identity.rhythmModel)) * 7ULL;
        motion_ = static_cast<RasgarMotion>(
            (identityBits(seed, 180) + identityDisplacement) % 6ULL);
        resistance_ = std::clamp(0.28 + 0.46 * identityUnit(seed, 181)
                               + 0.16 * gamePlan.memory
                               - 0.12 * gamePlan.risk, 0.12, 0.88);
        threshold_ = 0.62 + 0.28 * identityUnit(seed, 182);
        pressureRate_ = 0.13 + 0.22 * identityUnit(seed, 183)
                      + 0.08 * gamePlan.risk;
        waitBars_ = 4 + static_cast<int>(identityBits(seed, 184) % 7ULL);
        propagationBars_ = 1 + static_cast<int>(identityBits(seed, 185) % 4ULL);
        scarBars_ = 2 + static_cast<int>(identityBits(seed, 186) % 6ULL);
        direction_ = (identityBits(seed, 187) & 1ULL) == 0ULL ? -1 : 1;
        freedom_ = gamePlan.freedom;
        risk_ = gamePlan.risk;
        frame_.motion = motion_;

        descartographerRegion_ = topology_.addRegion(
            RegionRole::Descartographer, 0.0, gamePlan.energy,
            resistance_, 0);
        gestureToRhythmPassage_ = topology_.connect(
            descartographerRegion_, rhythmRegion_,
            ModuleDomain::Gesture, ModuleDomain::Rhythm,
            PassageMode::Accumulate, 0.75 + 0.5 * freedom_,
            resistance_ * 0.55, false, 0);
    }

    RasgarFrame advanceBar(const RasgarInput &input, int bar) {
        ++barsSinceTear_;
        frame_.tearTriggered = false;
        frame_.cism = {};

        const double density = std::clamp(input.density, 0.0, 1.0);
        const double energy = std::clamp(input.recentEnergy, 0.0, 1.0);
        const double repetition = std::clamp(input.repetition, 0.0, 1.0);
        const double stimulus = 0.38 * density + 0.24 * energy
                              + 0.24 * repetition
                              + (input.sectionBoundary ? 0.14 : 0.0);

        const ArchivePageId energyPage = archive_.publish(
            rhythmRegion_, ModuleDomain::Energy, stimulus, repetition, bar);
        archive_.reinterpret(descartographerRegion_, energyPage,
                             ModuleDomain::Resistance, resistance_, strain_, bar);

        if (phase_ == RasgarPhase::Ground || phase_ == RasgarPhase::Pressure) {
            const double pressure = std::max(0.025,
                stimulus - resistance_ * (0.30 + 0.15 * (1.0 - freedom_)));
            strain_ = std::clamp(strain_ + pressureRate_ * pressure
                                + risk_ * 0.012, 0.0, 1.4);
            if (phase_ == RasgarPhase::Ground
                && (strain_ >= threshold_ * 0.42 || barsSinceTear_ >= waitBars_ - 2)) {
                phase_ = RasgarPhase::Pressure;
            }
            if (strain_ >= threshold_ || barsSinceTear_ >= waitBars_) {
                beginTear(bar);
            }
        } else if (phase_ == RasgarPhase::Tear) {
            phase_ = RasgarPhase::Propagation;
            phaseBarsRemaining_ = propagationBars_;
        } else if (phase_ == RasgarPhase::Propagation) {
            if (--phaseBarsRemaining_ <= 0) {
                phase_ = RasgarPhase::Scar;
                phaseBarsRemaining_ = scarBars_;
            }
        } else if (phase_ == RasgarPhase::Scar) {
            if (--phaseBarsRemaining_ <= 0) {
                phase_ = RasgarPhase::Ground;
                strain_ = scar_ * (0.12 + 0.16 * identityUnit(seed_, 188 + tearCount_));
                waitBars_ = 5 + static_cast<int>(identityBits(seed_, 200 + tearCount_) % 9ULL);
            }
        }

        updateFrame();

        if (gestureToRhythmPassage_ != 0) {
            const double gestureValue = direction_ * frame_.instability
                                      + frame_.strain * 0.35;
            topology_.traverse(gestureToRhythmPassage_,
                               descartographerRegion_, gestureValue, bar);
        }
        archive_.publish(descartographerRegion_, ModuleDomain::Gesture,
                         frame_.instability, frame_.strain, bar,
                         frame_.tearTriggered ? ArchiveAction::Split
                                              : ArchiveAction::Publish);
        if (phase_ == RasgarPhase::Scar) {
            archive_.publish(descartographerRegion_, ModuleDomain::Scar,
                             scar_, static_cast<double>(tearCount_), bar,
                             ArchiveAction::Scar);
        }
        ++barsObserved_;
        accumulatedPulseDeviation_ += meanStepDeviation(16);
        maxInstability_ = std::max(maxInstability_, frame_.instability);
        if (phase_ != RasgarPhase::Ground) ++unstableBars_;
        return frame_;
    }

    double stepDurationScale(int step, int barLength) const {
        const int safeLength = std::max(1, barLength);
        const int s = ((step % safeLength) + safeLength) % safeLength;
        const double x = frame_.instability;
        double scale = 1.0;
        switch (motion_) {
            case RasgarMotion::Surge:
                scale = 1.0 - 0.48 * x;
                break;
            case RasgarMotion::Drag:
                scale = 1.0 + 0.82 * x;
                break;
            case RasgarMotion::Shear: {
                const bool secondField = s >= safeLength / 2;
                scale = 1.0 + (secondField ? direction_ : -direction_) * 0.58 * x;
                break;
            }
            case RasgarMotion::Stutter: {
                static const double shape[] = {0.42, 0.48, 0.55, 2.05, 0.36, 1.55};
                const double target = shape[s % 6];
                scale = 1.0 + (target - 1.0) * x;
                break;
            }
            case RasgarMotion::Suspension: {
                const int suspensionStep = static_cast<int>(identityBits(seed_, 189) %
                                                            static_cast<std::uint64_t>(safeLength));
                const double target = s == suspensionStep ? 2.4 : 0.72;
                scale = 1.0 + (target - 1.0) * x;
                break;
            }
            case RasgarMotion::Recoil: {
                const double target = (s % 4 < 3) ? 0.58 : 2.15;
                scale = 1.0 + (target - 1.0) * x;
                break;
            }
        }
        // The scar keeps a small asymmetry after the spectacular phase has
        // passed. The field never returns to a mathematically identical 1.0.
        scale += direction_ * frame_.scar * ((s % 2 == 0) ? 0.035 : -0.018);
        return std::clamp(scale, 0.28, 2.6);
    }

    int patternReadStep(int step, int barLength, int voiceIndex) const {
        if (barLength <= 0 || frame_.instability < 0.18 || voiceIndex == 0)
            return ((step % std::max(1, barLength)) + std::max(1, barLength))
                 % std::max(1, barLength);
        int distance = std::max(1, static_cast<int>(std::round(
            frame_.instability * static_cast<double>(barLength) * 0.24)));
        int voiceDirection = (voiceIndex % 2 == 0) ? -direction_ : direction_;
        int displaced = step + voiceDirection * distance;
        return ((displaced % barLength) + barLength) % barLength;
    }

    int voiceDelaySamples(int voiceIndex, std::size_t samplesPerStep) const {
        if (frame_.instability < 0.08 || voiceIndex <= 0) return 0;
        const double field = (voiceIndex % 2 == 0) ? 0.16 : 0.29;
        return static_cast<int>(std::round(
            static_cast<double>(samplesPerStep) * field * frame_.instability));
    }

    double velocityScale(int step, int barLength) const {
        const double pulse = stepDurationScale(step, barLength);
        const double counterforce = pulse < 1.0 ? (1.0 - pulse) : -(pulse - 1.0) * 0.35;
        return std::clamp(1.0 + counterforce * frame_.instability
                         + (phase_ == RasgarPhase::Tear ? 0.35 : 0.0),
                          0.48, 1.75);
    }

    RasgarMotion motion() const { return motion_; }
    RasgarPhase phase() const { return phase_; }
    const RasgarFrame &frame() const { return frame_; }
    int tearCount() const { return tearCount_; }
    int unstableBars() const { return unstableBars_; }
    double maxInstability() const { return maxInstability_; }
    double meanPulseDeviation() const {
        return barsObserved_ > 0
            ? accumulatedPulseDeviation_ / static_cast<double>(barsObserved_) : 0.0;
    }
    RegionId activeTemporalRegion() const { return activeTemporalRegion_; }

private:
    void beginTear(int bar) {
        phase_ = RasgarPhase::Tear;
        frame_.tearTriggered = true;
        barsSinceTear_ = 0;
        ++tearCount_;
        const double force = std::clamp(strain_ / std::max(0.01, threshold_), 0.35, 1.0);
        frame_.cism = topology_.split(activeTemporalRegion_, force,
                                      ModuleDomain::Pulse, bar);
        if (frame_.cism.interior != 0) {
            activeTemporalRegion_ = direction_ > 0
                ? frame_.cism.exterior : frame_.cism.interior;
            if (frame_.cism.opening != 0) {
                topology_.feedback(frame_.cism.opening,
                                   frame_.cism.interior,
                                   direction_ * force,
                                   -0.35 - 0.45 * risk_, bar);
            }
        }
        scar_ = std::clamp(scar_ * 0.55 + force * 0.45, 0.0, 1.0);
        strain_ = std::clamp(strain_, threshold_, 1.2);
    }

    void updateFrame() {
        frame_.phase = phase_;
        frame_.motion = motion_;
        frame_.strain = strain_;
        frame_.scar = scar_;
        switch (phase_) {
            case RasgarPhase::Ground:
                frame_.instability = 0.04 + scar_ * 0.10;
                break;
            case RasgarPhase::Pressure:
                frame_.instability = std::clamp(0.18 + 0.55 * strain_ / threshold_, 0.18, 0.72);
                break;
            case RasgarPhase::Tear:
                frame_.instability = 1.0;
                break;
            case RasgarPhase::Propagation:
                frame_.instability = 0.48 + 0.34
                    * static_cast<double>(std::max(0, phaseBarsRemaining_))
                    / static_cast<double>(std::max(1, propagationBars_));
                break;
            case RasgarPhase::Scar:
                frame_.instability = 0.12 + 0.24 * scar_
                    * static_cast<double>(std::max(0, phaseBarsRemaining_))
                    / static_cast<double>(std::max(1, scarBars_));
                break;
        }
        if (motion_ == RasgarMotion::Suspension && phase_ == RasgarPhase::Tear)
            frame_.drumPresenceScale = 0.62;
        else
            frame_.drumPresenceScale = 1.0 + frame_.instability * 0.58;
    }

    double meanStepDeviation(int barLength) const {
        double sum = 0.0;
        for (int step = 0; step < barLength; ++step)
            sum += std::fabs(stepDurationScale(step, barLength) - 1.0);
        return sum / static_cast<double>(barLength);
    }

    unsigned seed_;
    CompositionalTopology &topology_;
    OpenModuleArchive &archive_;
    RegionId activeTemporalRegion_ = 0;
    RegionId rhythmRegion_ = 0;
    RegionId descartographerRegion_ = 0;
    std::uint32_t gestureToRhythmPassage_ = 0;
    RasgarMotion motion_ = RasgarMotion::Surge;
    RasgarPhase phase_ = RasgarPhase::Ground;
    RasgarFrame frame_;
    double resistance_ = 0.5;
    double threshold_ = 0.75;
    double pressureRate_ = 0.2;
    double freedom_ = 0.5;
    double risk_ = 0.5;
    double strain_ = 0.0;
    double scar_ = 0.0;
    int direction_ = 1;
    int waitBars_ = 6;
    int propagationBars_ = 2;
    int scarBars_ = 4;
    int phaseBarsRemaining_ = 0;
    int barsSinceTear_ = 0;
    int tearCount_ = 0;
    int unstableBars_ = 0;
    int barsObserved_ = 0;
    double maxInstability_ = 0.0;
    double accumulatedPulseDeviation_ = 0.0;
};

} // namespace rasgo
