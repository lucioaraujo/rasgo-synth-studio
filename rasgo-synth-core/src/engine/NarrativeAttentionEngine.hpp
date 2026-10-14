#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "OrchestralRole.hpp"

namespace rasgo {

// The first observational contract for narrative mixing. It interprets a
// completed section, but owns no gain, EQ, pan or effect parameter. Keeping
// interpretation separate from actuation lets same-seed tests ask whether an
// event was valued before any mixer policy is allowed to change the audio.
enum class NarrativeAttentionReason {
    None,
    Presence,
    Emergence,
    Gesture,
    Contrast,
    Continuity,
    Consequence
};

inline const char *narrativeAttentionReasonName(
    NarrativeAttentionReason reason) noexcept {
    switch (reason) {
        case NarrativeAttentionReason::None: return "none";
        case NarrativeAttentionReason::Presence: return "presence";
        case NarrativeAttentionReason::Emergence: return "emergence";
        case NarrativeAttentionReason::Gesture: return "gesture";
        case NarrativeAttentionReason::Contrast: return "contrast";
        case NarrativeAttentionReason::Continuity: return "continuity";
        case NarrativeAttentionReason::Consequence: return "consequence";
    }
    return "none";
}

struct NarrativeAttentionContext {
    double density = 0.0;
    double tension = 0.0;
    double memory = 0.0;
    bool causalConsequence = false;
    OrganismStem consequenceTarget = OrganismStem::Other;
};

struct NarrativeAttentionDecision {
    bool valid = false;
    OrganismStem focus = OrganismStem::Other;
    NarrativeAttentionReason reason = NarrativeAttentionReason::None;
    double value = 0.0;
    double presence = 0.0;
    double emergence = 0.0;
    double gesture = 0.0;
    double contrast = 0.0;
    double continuity = 0.0;
    double consequence = 0.0;
    int manifestedBodies = 0;
    bool focusChanged = false;

    bool finite() const noexcept {
        return std::isfinite(value) && std::isfinite(presence)
            && std::isfinite(emergence) && std::isfinite(gesture)
            && std::isfinite(contrast) && std::isfinite(continuity)
            && std::isfinite(consequence);
    }
};

class NarrativeAttentionEngine {
public:
    static constexpr std::size_t kCount = OrganismStemAuditor::kCount;
    using ProfileSet = std::array<OrchestralProfile, kCount>;

    explicit NarrativeAttentionEngine(unsigned seed, bool enabled = true) noexcept
        : seed_(seed), enabled_(enabled) {}

    NarrativeAttentionDecision observe(
        const OrganismSectionSnapshot &section,
        const ProfileSet &profiles,
        const NarrativeAttentionContext &rawContext) noexcept {
        NarrativeAttentionDecision decision;
        if (!enabled_) return decision;

        const NarrativeAttentionContext context = sanitize(rawContext);
        double maximumRms = 1.0e-12;
        double weightedSpectralPosition = 0.0;
        double spectralWeight = 0.0;
        for (std::size_t i = 0; i < kCount; ++i) {
            if (!manifested(section[i])) continue;
            maximumRms = std::max(maximumRms, finitePositive(section[i].rms));
            const double weight = finitePositive(section[i].rms);
            weightedSpectralPosition += spectralPosition(section[i]) * weight;
            spectralWeight += weight;
        }
        const double sectionSpectralPosition = spectralWeight > 0.0
            ? weightedSpectralPosition / spectralWeight : 0.5;

        std::array<ComponentScores, kCount> components{};
        double bestValue = -1.0;
        std::size_t bestIndex = 0;
        int manifestedBodies = 0;
        for (std::size_t i = 0; i < kCount; ++i) {
            const auto &metrics = section[i];
            if (!manifested(metrics)) continue;
            ++manifestedBodies;

            ComponentScores score;
            const double rms = finitePositive(metrics.rms);
            const double audible = clamp01(metrics.audibleRatio);
            const double relativeRms = clamp01(rms / maximumRms);
            score.presence = 0.52 * audible + 0.48 * relativeRms;

            const bool wasManifested = previousManifested_[i];
            const double levelChange = std::fabs(relativeRms - previousRelativeRms_[i]);
            const double occupancyChange = std::fabs(audible - previousAudible_[i]);
            score.emergence = wasManifested
                ? clamp01(0.55 * levelChange + 0.45 * occupancyChange)
                : (observedSections_ == 0 ? 0.20 : 1.0);

            const double peak = std::max(rms, finitePositive(metrics.peak));
            const double peakShape = peak > 0.0
                ? clamp01((peak - rms) / peak) : 0.0;
            const bool eventShaped = profiles[i].presenceMode == PresenceMode::Event
                || profiles[i].presenceMode == PresenceMode::Episodic
                || profiles[i].spectralOccupation == SpectralOccupation::Transient;
            score.gesture = clamp01(peakShape * (eventShaped ? 1.15 : 0.85));
            score.contrast = clamp01(std::fabs(
                spectralPosition(metrics) - sectionSpectralPosition) * 1.6);
            score.continuity = wasManifested
                ? clamp01(std::min(audible, previousAudible_[i]) * 1.15) : 0.0;
            score.consequence = context.causalConsequence
                && static_cast<std::size_t>(context.consequenceTarget) == i
                ? clamp01(0.70 + 0.30 * context.memory) : 0.0;

            const double eventPressure = 0.78 + 0.44 * context.tension;
            const double flowPressure = 1.12 - 0.24 * context.tension;
            const double roleValue = roleAffinity(profiles[i].role);
            const double fatigue = i == previousFocus_
                ? std::min(0.12, static_cast<double>(focusStreak_) * 0.025)
                : 0.0;
            score.total = clamp01(
                0.18 * score.presence
                + 0.22 * score.emergence * eventPressure
                + 0.18 * score.gesture * eventPressure
                + 0.11 * score.contrast
                + 0.11 * score.continuity * flowPressure
                + 0.16 * score.consequence
                + roleValue
                + tieBreak(i)
                - fatigue);
            components[i] = score;
            if (score.total > bestValue) {
                bestValue = score.total;
                bestIndex = i;
            }
        }

        if (manifestedBodies > 0) {
            decision.valid = true;
            decision.focus = static_cast<OrganismStem>(bestIndex);
            decision.value = components[bestIndex].total;
            decision.presence = components[bestIndex].presence;
            decision.emergence = components[bestIndex].emergence;
            decision.gesture = components[bestIndex].gesture;
            decision.contrast = components[bestIndex].contrast;
            decision.continuity = components[bestIndex].continuity;
            decision.consequence = components[bestIndex].consequence;
            decision.reason = strongestReason(components[bestIndex]);
            decision.manifestedBodies = manifestedBodies;
            decision.focusChanged = hasFocus_ && bestIndex != previousFocus_;
            if (bestIndex == previousFocus_ && hasFocus_) ++focusStreak_;
            else focusStreak_ = 1;
            previousFocus_ = bestIndex;
            hasFocus_ = true;
            if (decision.focusChanged) ++focusChangeCount_;
            if (profiles[bestIndex].role == OrchestralRole::Event)
                ++eventFocusCount_;
            maximumValue_ = std::max(maximumValue_, decision.value);
            lastDecision_ = decision;
        }

        for (std::size_t i = 0; i < kCount; ++i) {
            previousManifested_[i] = manifested(section[i]);
            previousAudible_[i] = clamp01(section[i].audibleRatio);
            previousRelativeRms_[i] = manifested(section[i])
                ? clamp01(finitePositive(section[i].rms) / maximumRms) : 0.0;
        }
        ++observedSections_;
        return decision;
    }

    bool enabled() const noexcept { return enabled_; }
    int observedSections() const noexcept { return observedSections_; }
    int focusChangeCount() const noexcept { return focusChangeCount_; }
    int eventFocusCount() const noexcept { return eventFocusCount_; }
    double maximumValue() const noexcept { return maximumValue_; }
    const NarrativeAttentionDecision &lastDecision() const noexcept {
        return lastDecision_;
    }

private:
    struct ComponentScores {
        double total = 0.0;
        double presence = 0.0;
        double emergence = 0.0;
        double gesture = 0.0;
        double contrast = 0.0;
        double continuity = 0.0;
        double consequence = 0.0;
    };

    static double clamp01(double value) noexcept {
        return std::isfinite(value) ? std::clamp(value, 0.0, 1.0) : 0.0;
    }

    static double finitePositive(double value) noexcept {
        return std::isfinite(value) && value > 0.0 ? value : 0.0;
    }

    static bool manifested(const OrganismSectionMetrics &metrics) noexcept {
        return metrics.frames > 0 && finitePositive(metrics.rms) > 1.0e-9
            && clamp01(metrics.audibleRatio) > 0.004;
    }

    static double spectralPosition(const OrganismSectionMetrics &metrics) noexcept {
        const double low = finitePositive(metrics.lowBandRms);
        const double mid = finitePositive(metrics.midBandRms);
        const double high = finitePositive(metrics.highBandRms);
        const double sum = low + mid + high;
        return sum > 1.0e-12 ? (0.5 * mid + high) / sum : 0.5;
    }

    static double roleAffinity(OrchestralRole role) noexcept {
        switch (role) {
            case OrchestralRole::Protagonist: return 0.035;
            case OrchestralRole::Interlocutor: return 0.055;
            case OrchestralRole::Support: return 0.020;
            case OrchestralRole::Shadow: return 0.0;
            case OrchestralRole::Event: return 0.075;
        }
        return 0.0;
    }

    static NarrativeAttentionContext sanitize(
        NarrativeAttentionContext context) noexcept {
        context.density = clamp01(context.density);
        context.tension = clamp01(context.tension);
        context.memory = clamp01(context.memory);
        if (context.consequenceTarget == OrganismStem::Count)
            context.consequenceTarget = OrganismStem::Other;
        return context;
    }

    static NarrativeAttentionReason strongestReason(
        const ComponentScores &scores) noexcept {
        NarrativeAttentionReason reason = NarrativeAttentionReason::Presence;
        double strongest = scores.presence * 0.18;
        const auto consider = [&](NarrativeAttentionReason candidate,
                                  double value,
                                  NarrativeAttentionReason &currentReason,
                                  double &currentStrongest) {
            if (value > currentStrongest) {
                currentStrongest = value;
                currentReason = candidate;
            }
        };
        consider(NarrativeAttentionReason::Emergence, scores.emergence * 0.22,
                 reason, strongest);
        consider(NarrativeAttentionReason::Gesture, scores.gesture * 0.18,
                 reason, strongest);
        consider(NarrativeAttentionReason::Contrast, scores.contrast * 0.11,
                 reason, strongest);
        consider(NarrativeAttentionReason::Continuity, scores.continuity * 0.11,
                 reason, strongest);
        consider(NarrativeAttentionReason::Consequence, scores.consequence * 0.16,
                 reason, strongest);
        return reason;
    }

    double tieBreak(std::size_t index) const noexcept {
        std::uint64_t value = static_cast<std::uint64_t>(seed_)
            ^ (static_cast<std::uint64_t>(observedSections_ + 1) << 32u)
            ^ static_cast<std::uint64_t>(index * 0x9e3779b9u);
        value ^= value >> 30u;
        value *= 0xbf58476d1ce4e5b9ULL;
        value ^= value >> 27u;
        value *= 0x94d049bb133111ebULL;
        value ^= value >> 31u;
        return static_cast<double>(value & 0xffffu) / 65535.0 * 1.0e-6;
    }

    unsigned seed_ = 0;
    bool enabled_ = true;
    int observedSections_ = 0;
    int focusChangeCount_ = 0;
    int eventFocusCount_ = 0;
    std::array<bool, kCount> previousManifested_{};
    std::array<double, kCount> previousAudible_{};
    std::array<double, kCount> previousRelativeRms_{};
    std::size_t previousFocus_ = 0;
    int focusStreak_ = 0;
    bool hasFocus_ = false;
    double maximumValue_ = 0.0;
    NarrativeAttentionDecision lastDecision_{};
};

} // namespace rasgo
