#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>

#include "CompositionIdentity.hpp"
#include "GamePlan.hpp"

namespace rasgo {

enum class AgencyFamily {
    Core,
    Texture,
    Instrument,
    Voice,
    Polyphony,
    Rhythm,
    Rupture,
    Count
};

inline const char *agencyFamilyName(AgencyFamily family) {
    static const char *names[] = {
        "core", "texture", "instrument", "voice",
        "polyphony", "rhythm", "rupture"
    };
    return names[static_cast<std::size_t>(family)];
}

using AgencyActiveSet = std::array<bool, static_cast<std::size_t>(AgencyFamily::Count)>;
using AgencyGainSet = std::array<double, static_cast<std::size_t>(AgencyFamily::Count)>;

enum class AgencyDisposition {
    Dormant,
    Event,
    Withhold,
    Yield,
    Listen,
    Claim,
    Contest
};

inline const char *agencyDispositionName(AgencyDisposition disposition) {
    static const char *names[] = {
        "dormant", "event", "withhold", "yield", "listen", "claim", "contest"
    };
    return names[static_cast<std::size_t>(disposition)];
}

struct AgencyObservation {
    AgencyGainSet rms{};
    AgencyGainSet audibleRatio{};
    std::uint64_t frames = 0;
};

// Phase 123: agency is an ecology of claims, refusals and remembered encounters,
// not a turn-taking schedule. Each eligible family has its own seeded
// temperament and reacts to sound measured in the previous section. Sparse,
// asymmetric links let one family's actual emission invite or suppress another.
// A section may consequently have no claimant, one claimant or several. Event-
// shaped families keep their event gain without pretending to occupy a section.
class MaterialAgency {
public:
    MaterialAgency(unsigned seed, const CompositionIdentity &identity,
                   const GamePlan &game, AgencyActiveSet active,
                   double sampleRate, bool enabled = true)
        : MaterialAgency(seed, identity, game, active, active, sampleRate,
                         enabled) {}

    MaterialAgency(unsigned seed, const CompositionIdentity &identity,
                   const GamePlan &game, AgencyActiveSet active,
                   AgencyActiveSet foregroundEligible, double sampleRate,
                   bool enabled = true)
        : seed_(seed), enabled_(enabled), identity_(identity), game_(game),
          active_(active) {
        active_[index(AgencyFamily::Core)] = true;
        foregroundEligible_ = foregroundEligible;
        foregroundEligible_[index(AgencyFamily::Core)] = true;
        smoothing_ = 1.0 - std::exp(-1.0 / (std::max(1.0, sampleRate) * 0.32));
        initialiseTemperaments();
        if (enabled_) decideSection(false);
        else makeTransparent();
        current_ = target_;
        rememberSection();
    }

    void enterSection() {
        enterSection(AgencyObservation{});
    }

    void enterSection(const AgencyObservation &observation) {
        ++sectionIndex_;
        if (!enabled_) {
            rememberSection();
            return;
        }
        const std::string previous = foregroundLabel();
        const auto previousDispositions = dispositions_;
        updateMemory(observation);
        decideSection(observation.frames > 0);
        rememberSection();
        if (previous != foregroundLabel()) ++foregroundChangeCount_;
        for (std::size_t i = 0; i < dispositions_.size(); ++i) {
            if (previousDispositions[i] != dispositions_[i])
                ++dispositionChangeCount_;
        }
    }

    void tick() {
        for (std::size_t i = 0; i < current_.size(); ++i)
            current_[i] += (target_[i] - current_[i]) * smoothing_;
    }

    double gain(AgencyFamily family) const { return current_[index(family)]; }
    double targetGain(AgencyFamily family) const { return target_[index(family)]; }
    AgencyFamily foreground() const { return foreground_; }
    AgencyFamily interlocutor() const { return interlocutor_; }
    bool hasForeground() const { return hasForeground_; }
    bool hasInterlocutor() const { return hasInterlocutor_; }
    int claimantCount() const { return claimantCount_; }
    AgencyDisposition disposition(AgencyFamily family) const {
        return dispositions_[index(family)];
    }
    const char *foregroundLabel() const {
        return hasForeground_ ? agencyFamilyName(foreground_) : "none";
    }
    int sectionIndex() const { return sectionIndex_; }
    int foregroundChangeCount() const { return foregroundChangeCount_; }
    int dispositionChangeCount() const { return dispositionChangeCount_; }
    int leaderlessSectionCount() const { return leaderlessSectionCount_; }
    int pluralSectionCount() const { return pluralSectionCount_; }
    int observedSectionCount() const { return observedSectionCount_; }
    int relationalReactionCount() const { return relationalReactionCount_; }
    bool enabled() const { return enabled_; }
    const std::vector<AgencyFamily> &history() const { return history_; }
    const std::vector<std::string> &foregroundHistory() const {
        return foregroundHistory_;
    }
    const std::vector<std::vector<AgencyFamily>> &claimantHistory() const {
        return claimantHistory_;
    }

private:
    static constexpr std::size_t index(AgencyFamily family) {
        return static_cast<std::size_t>(family);
    }

    AgencyFamily identityPrimary() const {
        switch (identity_.leadVoice) {
            case LeadVoiceModel::FmFlute:
            case LeadVoiceModel::Theremin:
            case LeadVoiceModel::Tuba:
                if (foregroundEligible_[index(AgencyFamily::Instrument)])
                    return AgencyFamily::Instrument;
                break;
            case LeadVoiceModel::Ensemble:
                if (foregroundEligible_[index(AgencyFamily::Polyphony)])
                    return AgencyFamily::Polyphony;
                break;
            case LeadVoiceModel::Core:
                break;
        }
        switch (identity_.focus) {
            case MaterialFocus::Harmonic:
                if (foregroundEligible_[index(AgencyFamily::Polyphony)])
                    return AgencyFamily::Polyphony;
                break;
            case MaterialFocus::Rhythmic:
                if (foregroundEligible_[index(AgencyFamily::Rhythm)])
                    return AgencyFamily::Rhythm;
                break;
            case MaterialFocus::Spectral:
            case MaterialFocus::Textural:
            case MaterialFocus::Circuit:
                if (foregroundEligible_[index(AgencyFamily::Texture)])
                    return AgencyFamily::Texture;
                break;
            case MaterialFocus::Fragmented:
                // Rupture is event-shaped rather than section-shaped. It
                // interrupts at the instant it manifests; reserving a whole
                // section for a rare event produced a silent foreground.
                break;
        }
        return AgencyFamily::Core;
    }

    void initialiseTemperaments() {
        for (std::size_t i = 0; i < active_.size(); ++i) {
            boldness_[i] = identityUnit(seed_, 330 + i);
            persistence_[i] = identityUnit(seed_, 350 + i);
            silenceResponse_[i] = identityUnit(seed_, 370 + i);
            resistance_[i] = identityUnit(seed_, 390 + i);
            sociability_[i] = identityUnit(seed_, 410 + i);
        }
    }

    double identityAffinity(AgencyFamily family) const {
        double affinity = family == identityPrimary() ? 1.0 : 0.25;
        switch (identity_.focus) {
            case MaterialFocus::Harmonic:
                if (family == AgencyFamily::Polyphony) affinity += 0.45;
                break;
            case MaterialFocus::Rhythmic:
                if (family == AgencyFamily::Rhythm) affinity += 0.45;
                break;
            case MaterialFocus::Spectral:
            case MaterialFocus::Textural:
                if (family == AgencyFamily::Texture) affinity += 0.45;
                break;
            case MaterialFocus::Circuit:
                if (family == AgencyFamily::Texture
                    || family == AgencyFamily::Instrument) affinity += 0.24;
                break;
            case MaterialFocus::Fragmented:
                if (family == AgencyFamily::Voice
                    || family == AgencyFamily::Rhythm) affinity += 0.18;
                break;
        }
        return std::clamp(affinity, 0.0, 1.0);
    }

    void updateMemory(const AgencyObservation &observation) {
        if (observation.frames == 0) return;
        ++observedSectionCount_;
        double maximumRms = 1.0e-9;
        for (std::size_t i = 0; i < active_.size(); ++i)
            maximumRms = std::max(maximumRms, observation.rms[i]);

        for (std::size_t i = 0; i < active_.size(); ++i) {
            if (!active_[i]) continue;
            const double audible = std::clamp(observation.audibleRatio[i], 0.0, 1.0);
            const double relativeEnergy = std::clamp(
                observation.rms[i] / maximumRms, 0.0, 1.0);
            const double memoryRate = 0.18 + 0.42 * game_.memory;
            presenceMemory_[i] += (audible - presenceMemory_[i]) * memoryRate;
            energyMemory_[i] += (relativeEnergy - energyMemory_[i]) * memoryRate;
            if (audible < 0.004) ++silentSections_[i];
            else silentSections_[i] = 0;
            const bool wasClaiming = dispositions_[i] == AgencyDisposition::Claim
                                  || dispositions_[i] == AgencyDisposition::Contest;
            const double fatigueTarget = wasClaiming && audible > 0.18 ? audible : 0.0;
            fatigue_[i] += (fatigueTarget - fatigue_[i])
                         * (0.24 + 0.20 * (1.0 - persistence_[i]));

            double encounter = 0.0;
            double encounterWeight = 0.0;
            for (std::size_t j = 0; j < active_.size(); ++j) {
                if (i == j || !active_[j] || observation.audibleRatio[j] < 0.004)
                    continue;
                const double gate = identityUnit(seed_, 440 + i * active_.size() + j);
                if (gate < 0.68) continue;
                const double polarity = identityUnit(
                    seed_, 500 + i * active_.size() + j) < 0.5 ? -1.0 : 1.0;
                const double strength = (gate - 0.68) / 0.32;
                encounter += polarity * strength
                           * std::clamp(observation.rms[j] / maximumRms, 0.0, 1.0);
                encounterWeight += strength;
            }
            if (encounterWeight > 0.0) encounter /= encounterWeight;
            const double previousTrace = relationTrace_[i];
            relationTrace_[i] += (encounter - relationTrace_[i])
                               * (0.16 + 0.46 * sociability_[i]);
            if (std::fabs(relationTrace_[i] - previousTrace) > 0.035)
                ++relationalReactionCount_;
        }
    }

    double gameResponse(std::size_t i) const {
        const double presence = presenceMemory_[i];
        const double energy = energyMemory_[i];
        double response = 0.0;
        switch (game_.strategy) {
            case GameStrategy::Follow:
                response = energy * (0.12 + 0.16 * game_.memory);
                break;
            case GameStrategy::Transform:
                response = relationTrace_[i] * (0.15 + 0.22 * game_.freedom)
                         + std::fabs(0.5 - energy) * 0.08;
                break;
            case GameStrategy::Counter:
                response = (0.5 - energy) * (0.16 + 0.22 * game_.energy);
                break;
            case GameStrategy::Interrupt:
                response = (identityUnit(seed_, 690 + sectionIndex_ * 13U + i) - 0.5)
                         * (0.18 + 0.28 * game_.risk);
                break;
            case GameStrategy::Conceal:
                response = (0.4 - presence) * (0.12 + 0.24 * game_.hiddenInformation);
                break;
            case GameStrategy::Negotiate:
                response = relationTrace_[i] * (0.12 + 0.22 * sociability_[i])
                         + (0.45 - std::fabs(0.45 - presence)) * 0.12;
                break;
        }
        switch (game_.rule) {
            case GameRule::Preserve: response += energy * persistence_[i] * 0.10; break;
            case GameRule::Mutate: response += relationTrace_[i] * 0.12; break;
            case GameRule::Oppose: response -= relationTrace_[i] * 0.13; break;
            case GameRule::Withhold: response -= presence * resistance_[i] * 0.12; break;
            case GameRule::Exchange: response += sociability_[i] * (0.5 - presence) * 0.15; break;
            case GameRule::Break: response += (game_.risk - fatigue_[i]) * 0.11; break;
        }
        return response;
    }

    void decideSection(bool hasObservation) {
        static constexpr AgencyGainSet neutral = {
            0.92, 1.00, 1.00, 1.00, 1.00, 1.00, 1.00
        };
        static constexpr AgencyGainSet claimGain = {
            1.08, 1.42, 1.38, 1.52, 1.45, 1.45, 1.00
        };
        static constexpr AgencyGainSet contestGain = {
            1.24, 1.72, 1.68, 1.82, 1.75, 1.78, 1.00
        };

        claimantCount_ = 0;
        hasForeground_ = false;
        hasInterlocutor_ = false;
        currentClaimants_.clear();
        double strongestScore = -10.0;
        double interlocutorScore = -10.0;

        for (std::size_t i = 0; i < target_.size(); ++i) {
            const auto family = static_cast<AgencyFamily>(i);
            if (!active_[i]) {
                dispositions_[i] = AgencyDisposition::Dormant;
                target_[i] = 0.0;
                continue;
            }
            if (!foregroundEligible_[i]) {
                dispositions_[i] = AgencyDisposition::Event;
                target_[i] = neutral[i];
                continue;
            }

            const double change = identityUnit(
                seed_, 720 + static_cast<std::size_t>(sectionIndex_) * 31U + i * 3U);
            const double thresholdNoise = identityUnit(
                seed_, 980 + static_cast<std::size_t>(sectionIndex_) * 43U + i * 5U);
            const bool wasClaiming = dispositions_[i] == AgencyDisposition::Claim
                                  || dispositions_[i] == AgencyDisposition::Contest;
            const double silenceAge = std::min(1.0, silentSections_[i] / 4.0);
            const double silenceEffect = silenceAge
                * (silenceResponse_[i] - 0.52) * 0.32;
            const double inertia = wasClaiming
                ? persistence_[i] * (0.10 + 0.16 * game_.memory) : 0.0;
            double score = 0.18 + boldness_[i] * 0.30
                + identityAffinity(family) * 0.14
                + (change - 0.5) * (0.32 + game_.freedom * 0.14)
                + gameResponse(i) + relationTrace_[i] * 0.14
                + silenceEffect + inertia - fatigue_[i] * 0.24;
            if (family == AgencyFamily::Core
                && identity_.leadVoice != LeadVoiceModel::Core) {
                // A non-core lead identity must have perceptual consequence.
                // Core remains alive and may still contradict it when its
                // own boldness/encounters are strong enough, but it no longer
                // claims merely because its continuous signal is ubiquitous.
                score -= 0.16 + resistance_[i] * 0.10;
            }
            claimScores_[i] = score;

            // No family can claim manifestation before the orchestra has
            // emitted anything, nor on the strength of a completely silent
            // previous section. Silence can still alter listening/withholding.
            const bool manifested = hasObservation
                && (presenceMemory_[i] > 0.002 || energyMemory_[i] > 0.002);
            const double claimThreshold = 0.48 + resistance_[i] * 0.20
                                        + (thresholdNoise - 0.5) * 0.10;
            const double contestThreshold = claimThreshold + 0.18
                                          + (1.0 - game_.risk) * 0.07;

            if (manifested && score >= contestThreshold) {
                dispositions_[i] = AgencyDisposition::Contest;
                target_[i] = contestGain[i] * (0.90 + game_.energy * 0.18);
                currentClaimants_.push_back(family);
            } else if (manifested && score >= claimThreshold) {
                dispositions_[i] = AgencyDisposition::Claim;
                target_[i] = claimGain[i] * (0.90 + game_.energy * 0.16);
                currentClaimants_.push_back(family);
            } else if (presenceMemory_[i] > 0.42
                       && (fatigue_[i] + resistance_[i] * 0.35) > 0.42) {
                dispositions_[i] = AgencyDisposition::Yield;
                target_[i] = neutral[i] * (0.52 + persistence_[i] * 0.20);
            } else if (change < 0.13 + game_.hiddenInformation * 0.12) {
                dispositions_[i] = AgencyDisposition::Withhold;
                target_[i] = neutral[i] * (0.34 + resistance_[i] * 0.20);
            } else {
                dispositions_[i] = AgencyDisposition::Listen;
                target_[i] = neutral[i] * (0.88 + sociability_[i] * 0.20);
            }

            if (family == AgencyFamily::Core
                && identity_.leadVoice != LeadVoiceModel::Core) {
                target_[i] *= 0.58 + persistence_[i] * 0.18;
            }

            if ((dispositions_[i] == AgencyDisposition::Claim
                 || dispositions_[i] == AgencyDisposition::Contest)
                && score > strongestScore) {
                strongestScore = score;
                foreground_ = family;
                hasForeground_ = true;
            }
        }

        claimantCount_ = static_cast<int>(currentClaimants_.size());
        if (claimantCount_ == 0) ++leaderlessSectionCount_;
        if (claimantCount_ > 1) ++pluralSectionCount_;

        // An interlocutor is heard as a separate relation, not selected as
        // the automatic next item in an ordered list.
        if (hasForeground_) {
            for (std::size_t i = 0; i < target_.size(); ++i) {
                const auto family = static_cast<AgencyFamily>(i);
                if (!active_[i] || family == foreground_) continue;
                const double score = sociability_[i] * 0.40
                                   + presenceMemory_[i] * 0.26
                                   + relationTrace_[i] * 0.24
                                   + claimScores_[i] * 0.10;
                if (score > interlocutorScore && score > 0.24) {
                    interlocutorScore = score;
                    interlocutor_ = family;
                    hasInterlocutor_ = true;
                }
            }
        }
        if (!hasInterlocutor_) interlocutor_ = foreground_;
    }

    void makeTransparent() {
        currentClaimants_.clear();
        claimantCount_ = 0;
        hasForeground_ = false;
        hasInterlocutor_ = false;
        foreground_ = interlocutor_ = AgencyFamily::Core;
        for (std::size_t i = 0; i < target_.size(); ++i) {
            target_[i] = active_[i] ? 1.0 : 0.0;
            if (!active_[i]) dispositions_[i] = AgencyDisposition::Dormant;
            else if (!foregroundEligible_[i]) dispositions_[i] = AgencyDisposition::Event;
            else dispositions_[i] = AgencyDisposition::Listen;
        }
    }

    void rememberSection() {
        history_.push_back(foreground_);
        foregroundHistory_.emplace_back(foregroundLabel());
        claimantHistory_.push_back(currentClaimants_);
    }

    unsigned seed_;
    bool enabled_ = true;
    CompositionIdentity identity_;
    GamePlan game_;
    AgencyActiveSet active_{};
    AgencyActiveSet foregroundEligible_{};
    AgencyGainSet current_{};
    AgencyGainSet target_{};
    AgencyGainSet boldness_{};
    AgencyGainSet persistence_{};
    AgencyGainSet silenceResponse_{};
    AgencyGainSet resistance_{};
    AgencyGainSet sociability_{};
    AgencyGainSet presenceMemory_{};
    AgencyGainSet energyMemory_{};
    AgencyGainSet fatigue_{};
    AgencyGainSet relationTrace_{};
    AgencyGainSet claimScores_{};
    std::array<int, static_cast<std::size_t>(AgencyFamily::Count)> silentSections_{};
    std::array<AgencyDisposition, static_cast<std::size_t>(AgencyFamily::Count)>
        dispositions_{};
    std::vector<AgencyFamily> history_;
    std::vector<std::string> foregroundHistory_;
    std::vector<AgencyFamily> currentClaimants_;
    std::vector<std::vector<AgencyFamily>> claimantHistory_;
    AgencyFamily foreground_ = AgencyFamily::Core;
    AgencyFamily interlocutor_ = AgencyFamily::Core;
    bool hasForeground_ = false;
    bool hasInterlocutor_ = false;
    int claimantCount_ = 0;
    double smoothing_ = 0.001;
    int sectionIndex_ = 0;
    int foregroundChangeCount_ = 0;
    int dispositionChangeCount_ = 0;
    int leaderlessSectionCount_ = 0;
    int pluralSectionCount_ = 0;
    int observedSectionCount_ = 0;
    int relationalReactionCount_ = 0;
};

// Pre-space contribution meter. Families can add several module signals during
// one sample; finishFrame observes their sum, so a family is measured as heard
// before the shared delay/reverb rather than as a misleading count of flags.
class AgencyContributionMeter {
public:
    void setBarObservationEnabled(bool enabled) { measureBar_ = enabled; }
    void add(AgencyFamily family, double left, double right) {
        frameLeft_[index(family)] += left;
        frameRight_[index(family)] += right;
    }

    // A later desk can make the already-assembled orchestra yield (for
    // example a drum transient opening real room). Keep the diagnostic bus
    // faithful to that audible interaction instead of reporting the
    // pre-duck values as if they survived unchanged.
    void scaleFrame(double scale) {
        if (!std::isfinite(scale)) return;
        for (std::size_t i = 0; i < frameLeft_.size(); ++i) {
            frameLeft_[i] *= scale;
            frameRight_[i] *= scale;
        }
    }

    void finishFrame() {
        for (std::size_t i = 0; i < frameLeft_.size(); ++i) {
            const double left = frameLeft_[i];
            const double right = frameRight_[i];
            const double frameEnergy = 0.5 * (left * left + right * right);
            energy_[i] += frameEnergy;
            sectionEnergy_[i] += frameEnergy;
            if (measureBar_) barEnergy_[i] += frameEnergy;
            peak_[i] = std::max(peak_[i], std::max(std::fabs(left), std::fabs(right)));
            if (std::max(std::fabs(left), std::fabs(right)) > 0.003) {
                ++audibleFrames_[i];
                ++sectionAudibleFrames_[i];
            }
            frameLeft_[i] = 0.0;
            frameRight_[i] = 0.0;
        }
        ++frames_;
        ++sectionFrames_;
        if (measureBar_) ++barFrames_;
    }

    AgencyObservation takeBarObservation() {
        AgencyObservation out;
        out.frames = barFrames_;
        if (barFrames_ > 0)
            for (std::size_t i = 0; i < barEnergy_.size(); ++i)
                out.rms[i] = std::sqrt(barEnergy_[i] / static_cast<double>(barFrames_));
        barEnergy_.fill(0.0); barFrames_ = 0;
        return out;
    }

    double rms(AgencyFamily family) const {
        return frames_ == 0 ? 0.0
            : std::sqrt(energy_[index(family)] / static_cast<double>(frames_));
    }
    double peak(AgencyFamily family) const { return peak_[index(family)]; }
    double audibleRatio(AgencyFamily family) const {
        return frames_ == 0 ? 0.0
            : static_cast<double>(audibleFrames_[index(family)])
              / static_cast<double>(frames_);
    }
    std::uint64_t frames() const { return frames_; }

    AgencyObservation takeSectionObservation() {
        AgencyObservation out;
        out.frames = sectionFrames_;
        if (sectionFrames_ > 0) {
            for (std::size_t i = 0; i < sectionEnergy_.size(); ++i) {
                out.rms[i] = std::sqrt(
                    sectionEnergy_[i] / static_cast<double>(sectionFrames_));
                out.audibleRatio[i] = static_cast<double>(sectionAudibleFrames_[i])
                    / static_cast<double>(sectionFrames_);
            }
        }
        sectionEnergy_.fill(0.0);
        sectionAudibleFrames_.fill(0);
        sectionFrames_ = 0;
        return out;
    }

private:
    static constexpr std::size_t index(AgencyFamily family) {
        return static_cast<std::size_t>(family);
    }
    AgencyGainSet frameLeft_{};
    AgencyGainSet frameRight_{};
    AgencyGainSet energy_{};
    AgencyGainSet peak_{};
    AgencyGainSet sectionEnergy_{};
    AgencyGainSet barEnergy_{};
    bool measureBar_ = false;
    std::uint64_t barFrames_ = 0;
    std::array<std::uint64_t, static_cast<std::size_t>(AgencyFamily::Count)>
        audibleFrames_{};
    std::array<std::uint64_t, static_cast<std::size_t>(AgencyFamily::Count)>
        sectionAudibleFrames_{};
    std::uint64_t frames_ = 0;
    std::uint64_t sectionFrames_ = 0;
};

} // namespace rasgo
