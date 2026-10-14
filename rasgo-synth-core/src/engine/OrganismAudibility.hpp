#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "CompositionIdentity.hpp"
#include "GamePlan.hpp"
#include "MaterialAgency.hpp"
#include "NarrativeMixDirective.hpp"
#include "OrchestralRole.hpp"
#include "OrchestralSectionMemory.hpp"
#include "OrganismStemAuditor.hpp"

namespace rasgo {

using OrganismStemActiveSet = std::array<bool, OrganismStemAuditor::kCount>;

// Phase 122: a moving desk ecology, not an automatic normalizer.  The
// calibration ranges repair large, measured output-scale differences between
// modules (most notably Sieve versus theremin/voices/drums).  Section movement
// then lets every active desk independently emerge, remain neutral or yield.
// Nothing is triggered, scheduled or guaranteed to become foreground here:
// gain applied to silence remains silence.
class OrganismAudibility {
public:
    OrganismAudibility(unsigned seed, bool enabled,
                       const OrganismStemActiveSet &active,
                       const CompositionIdentity &identity,
                       const GamePlan &game, double sampleRate)
        : seed_(seed), enabled_(enabled), active_(active),
          identity_(identity), game_(game) {
        smoothing_ = 1.0 - std::exp(
            -1.0 / (std::max(1.0, sampleRate) * 0.42));
        for (std::size_t i = 0; i < kCount; ++i) {
            const auto stem = static_cast<OrganismStem>(i);
            base_[i] = enabled_ && active_[i] ? calibratedBase(stem) : 1.0;
            target_[i] = current_[i] = base_[i];
            minimum_[i] = maximum_[i] = current_[i];
        }
    }

    void enterSection(double density, AgencyFamily familyForeground,
                      AgencyFamily familyInterlocutor) {
        moveSection(density, familyForeground, familyInterlocutor, nullptr);
    }

    void enterSection(double density, const MaterialAgency &agency) {
        moveSection(density, agency.foreground(), agency.interlocutor(), &agency);
    }

    // The previous section is now allowed to answer the next one.  This is a
    // bounded role response, not a normalizer: a repeatedly strongest stem
    // yields a little so another material can become legible, while silence
    // and inactive stems remain untouched.
    void enterSection(double density, const MaterialAgency &agency,
                      const OrchestralSectionReport &previous) {
        moveSection(density, agency.foreground(), agency.interlocutor(),
                    &agency, &previous);
    }

private:
    void moveSection(double density, AgencyFamily familyForeground,
                     AgencyFamily familyInterlocutor,
                     const MaterialAgency *agency,
                     const OrchestralSectionReport *previous = nullptr) {
        if (!enabled_) return;
        ++sectionIndex_;
        for (std::size_t i = 0; i < kCount; ++i) {
            if (!active_[i]) continue;
            const auto stem = static_cast<OrganismStem>(i);
            const double roll = identityUnit(
                seed_, 520 + static_cast<std::size_t>(sectionIndex_) * 37U + i);
            const double detail = identityUnit(
                seed_, 860 + static_cast<std::size_t>(sectionIndex_) * 41U + i);

            double motion = 0.82 + detail * 0.36;
            const double openingChance = 0.10 + game_.freedom * 0.13
                                       + game_.risk * 0.05;
            if (roll < openingChance) {
                motion = 1.30 + detail * (0.48 + game_.energy * 0.42);
                ++emergenceCount_;
            } else if (roll > 0.84 - game_.hiddenInformation * 0.05) {
                motion = 0.50 + detail * 0.30;
                ++yieldCount_;
            }

            // Family agency is context, never a command. Phase 123 admits no
            // claimant and several simultaneous claimants; each organism still
            // owns its independent draw and may remain silent under any gain.
            const AgencyFamily family = familyFor(stem);
            if (agency != nullptr) {
                switch (agency->disposition(family)) {
                    case AgencyDisposition::Contest: motion *= 1.09; break;
                    case AgencyDisposition::Claim: motion *= 1.06; break;
                    case AgencyDisposition::Yield: motion *= 0.95; break;
                    case AgencyDisposition::Withhold: motion *= 0.93; break;
                    case AgencyDisposition::Listen: motion *= 1.01; break;
                    case AgencyDisposition::Event:
                    case AgencyDisposition::Dormant: break;
                }
            } else if (family == familyForeground) {
                motion *= 1.08;
            } else if (family == familyInterlocutor) {
                motion *= 1.03;
            }

            // Memory acts only after an actual observed repetition.  Keep
            // the response local and reversible; the winning material is
            // not muted, merely invited to make room for a different voice.
            if (previous != nullptr && previous->repeatedLeader
                && stem == previous->strongest) {
                motion *= 0.84;
                ++orchestralRoleResponseCount_;
            }

            // A measured high-register crowd is a compositional condition,
            // not merely a post-mix problem. Let the next section answer it
            // locally: non-protagonist air voices yield a little, while a
            // low/foundation desk gets a small invitation to occupy the
            // missing body. Protagonists and transient events remain nearly
            // untouched, so the rule cannot flatten the identity into an EQ
            // preset or silence a deliberate flash of brightness.
            if (previous != nullptr && previous->highConflictCount >= 2
                && previous->highBandShare >= 0.42) {
                const auto profile = baseOrchestralProfile(stem, identity_);
                if (profile.registerBand == RegisterBand::High) {
                    if (profile.role == OrchestralRole::Protagonist) {
                        motion *= 0.97;
                    } else if (profile.role != OrchestralRole::Event) {
                        motion *= 0.84;
                    }
                } else if (profile.registerBand == RegisterBand::Low
                           || profile.registerBand == RegisterBand::Sub) {
                    motion *= 1.04;
                }
                ++spectralRoleResponseCount_;
            }

            // Dense sections need less additive gain, but event-shaped desks
            // retain their capacity to cut through an already crowded field.
            if (!eventShaped(stem))
                motion *= 1.0 - std::clamp(density, 0.0, 1.0) * 0.10;

            target_[i] = std::clamp(base_[i] * motion,
                                    lowerBound(stem), upperBound(stem));
            ++movementCount_;
        }
    }

public:
    // Applies the narrative invitation after normal section ecology chose its
    // own target. The directive is gain-only in this first actuation phase;
    // existing audio-rate smoothing makes the section transition reversible.
    // No other desk is ducked, so attention cannot manufacture a solo.
    void applyNarrativeDirective(const NarrativeMixDirective &directive) {
        if (!enabled_ || !directive.valid || !directive.finite()) return;
        const auto i = index(directive.focus);
        if (i >= kCount || !active_[i]) return;
        const double linear = std::pow(10.0,
            std::clamp(directive.gainDb, 0.0, 1.25) / 20.0);
        const auto stem = static_cast<OrganismStem>(i);
        target_[i] = std::clamp(target_[i] * linear,
                                lowerBound(stem), upperBound(stem));
        ++narrativeDirectiveCount_;
        if (directive.heldByHysteresis) ++narrativeHysteresisHoldCount_;
        narrativeMaximumGainDb_ = std::max(narrativeMaximumGainDb_, directive.gainDb);
    }

    void tick() noexcept {
        if (!enabled_) return;
        for (std::size_t i = 0; i < kCount; ++i) {
            current_[i] += (target_[i] - current_[i]) * smoothing_;
            minimum_[i] = std::min(minimum_[i], current_[i]);
            maximum_[i] = std::max(maximum_[i], current_[i]);
        }
    }

    double gain(OrganismStem stem) const noexcept {
        return enabled_ ? current_[index(stem)] : 1.0;
    }
    double targetGain(OrganismStem stem) const noexcept {
        return enabled_ ? target_[index(stem)] : 1.0;
    }
    double minimumGain(OrganismStem stem) const noexcept {
        return enabled_ ? minimum_[index(stem)] : 1.0;
    }
    double maximumGain(OrganismStem stem) const noexcept {
        return enabled_ ? maximum_[index(stem)] : 1.0;
    }
    bool enabled() const noexcept { return enabled_; }
    int movementCount() const noexcept { return movementCount_; }
    int emergenceCount() const noexcept { return emergenceCount_; }
    int yieldCount() const noexcept { return yieldCount_; }
    int orchestralRoleResponseCount() const noexcept {
        return orchestralRoleResponseCount_;
    }
    int spectralRoleResponseCount() const noexcept {
        return spectralRoleResponseCount_;
    }
    int narrativeDirectiveCount() const noexcept { return narrativeDirectiveCount_; }
    int narrativeHysteresisHoldCount() const noexcept {
        return narrativeHysteresisHoldCount_;
    }
    double narrativeMaximumGainDb() const noexcept {
        return narrativeMaximumGainDb_;
    }

    static constexpr std::size_t kCount = OrganismStemAuditor::kCount;

    static constexpr std::size_t index(OrganismStem stem) noexcept {
        return static_cast<std::size_t>(stem);
    }

    double calibratedBase(OrganismStem stem) const {
        const double u = identityUnit(seed_, 470 + index(stem));
        switch (stem) {
            case OrganismStem::Core:
                // The invariant synthesizer is allowed to lead when the
                // identity actually chose it. Under a flute/theremin/tuba/
                // ensemble identity it must not contradict that decision by
                // remaining the loudest desk almost automatically.
                return identity_.leadVoice == LeadVoiceModel::Core
                    ? 0.78 + 0.20 * u
                    : 0.50 + 0.22 * u;
            case OrganismStem::Texture:       return 0.74 + 0.30 * u;
            case OrganismStem::Theremin:      return 3.20 + 3.20 * u;
            case OrganismStem::FmFlute:       return 1.25 + 0.75 * u;
            case OrganismStem::Tuba:          return 0.78 + 0.30 * u;
            case OrganismStem::Acid:          return 1.20 + 0.65 * u;
            case OrganismStem::BassFallback:  return 1.10 + 0.45 * u;
            case OrganismStem::Galo:          return 2.00 + 1.40 * u;
            case OrganismStem::Scat:          return 1.80 + 1.10 * u;
            case OrganismStem::Choir:         return 2.00 + 1.40 * u;
            case OrganismStem::DejaVu:        return 2.40 + 1.50 * u;
            case OrganismStem::Drums:         return 1.35 + 0.80 * u;
            case OrganismStem::Harmonaig:     return 0.88 + 0.38 * u;
            case OrganismStem::Sieve:
                // The audit measured 76-89% energy in two Sieve pieces.
                // Most seeds recalibrate it to an ensemble desk; a rare seed
                // can still choose a genuinely dominant swarm.
                return u > 0.92 ? 0.48 : 0.15 + 0.15 * u;
            case OrganismStem::Bloom:         return 1.25 + 0.90 * u;
            case OrganismStem::Hocket:        return 1.05 + 0.65 * u;
            case OrganismStem::Spectral:      return 0.88 + 0.48 * u;
            case OrganismStem::Polymeter:     return 0.52 + 0.38 * u;
            case OrganismStem::Bluff:         return 1.15 + 0.80 * u;
            case OrganismStem::Footnote:      return 1.80 + 1.20 * u;
            case OrganismStem::Rupture:       return 0.90 + 0.45 * u;
            case OrganismStem::Other:         return 1.0;
            case OrganismStem::ThermalStatic: return 1.0;
            case OrganismStem::Count:         break;
        }
        return 1.0;
    }

    static constexpr double lowerBound(OrganismStem stem) noexcept {
        switch (stem) {
            case OrganismStem::Sieve: return 0.08;
            case OrganismStem::Polymeter: return 0.28;
            case OrganismStem::Core: return 0.42;
            default: return 0.35;
        }
    }

    static constexpr double upperBound(OrganismStem stem) noexcept {
        switch (stem) {
            case OrganismStem::Theremin: return 11.0;
            case OrganismStem::DejaVu: return 7.0;
            case OrganismStem::Galo:
            case OrganismStem::Choir: return 6.0;
            case OrganismStem::Scat:
            case OrganismStem::Drums: return 5.0;
            case OrganismStem::Sieve: return 0.82;
            case OrganismStem::Polymeter: return 1.35;
            default: return 3.5;
        }
    }

    static constexpr bool eventShaped(OrganismStem stem) noexcept {
        return stem == OrganismStem::Galo || stem == OrganismStem::Scat
            || stem == OrganismStem::Choir || stem == OrganismStem::DejaVu
            || stem == OrganismStem::Drums || stem == OrganismStem::Bloom
            || stem == OrganismStem::Bluff || stem == OrganismStem::Footnote
            || stem == OrganismStem::Rupture;
    }

    static constexpr AgencyFamily familyFor(OrganismStem stem) noexcept {
        switch (stem) {
            case OrganismStem::Core: return AgencyFamily::Core;
            case OrganismStem::Texture:
            case OrganismStem::Spectral: return AgencyFamily::Texture;
            case OrganismStem::Theremin:
            case OrganismStem::FmFlute:
            case OrganismStem::Tuba:
            case OrganismStem::Acid:
            case OrganismStem::BassFallback:
            case OrganismStem::Galo:
            case OrganismStem::Footnote: return AgencyFamily::Instrument;
            case OrganismStem::Scat:
            case OrganismStem::Choir:
            case OrganismStem::DejaVu:
            case OrganismStem::Bluff: return AgencyFamily::Voice;
            case OrganismStem::Harmonaig:
            case OrganismStem::Sieve:
            case OrganismStem::Bloom:
            case OrganismStem::Hocket: return AgencyFamily::Polyphony;
            case OrganismStem::Drums:
            case OrganismStem::Polymeter: return AgencyFamily::Rhythm;
            case OrganismStem::Rupture: return AgencyFamily::Rupture;
            case OrganismStem::Other:
            case OrganismStem::ThermalStatic:
            case OrganismStem::Count: return AgencyFamily::Core;
        }
        return AgencyFamily::Core;
    }

    unsigned seed_ = 0;
    bool enabled_ = false;
    OrganismStemActiveSet active_{};
    CompositionIdentity identity_;
    GamePlan game_;
    std::array<double, kCount> base_{};
    std::array<double, kCount> target_{};
    std::array<double, kCount> current_{};
    std::array<double, kCount> minimum_{};
    std::array<double, kCount> maximum_{};
    double smoothing_ = 0.001;
    int sectionIndex_ = 0;
    int movementCount_ = 0;
    int emergenceCount_ = 0;
    int yieldCount_ = 0;
    int orchestralRoleResponseCount_ = 0;
    int spectralRoleResponseCount_ = 0;
    int narrativeDirectiveCount_ = 0;
    int narrativeHysteresisHoldCount_ = 0;
    double narrativeMaximumGainDb_ = 0.0;
};

} // namespace rasgo
