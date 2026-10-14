#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "NarrativeAttentionEngine.hpp"

namespace rasgo {

// Phase 165: the first authority granted to narrative attention. A directive
// is deliberately smaller than a mixer: one evidenced organism may receive a
// bounded invitation at a section boundary. It contains no routing, DSP,
// allocation or audio-thread state and can therefore be tested independently
// before it reaches OrganismAudibility's existing smoothing path.
struct NarrativeMixDirective {
    bool valid = false;
    OrganismStem focus = OrganismStem::Other;
    NarrativeAttentionReason reason = NarrativeAttentionReason::None;
    double gainDb = 0.0;
    bool heldByHysteresis = false;

    bool finite() const noexcept {
        return std::isfinite(gainDb) && gainDb >= 0.0 && gainDb <= 1.25;
    }
};

class NarrativeMixDirector {
public:
    explicit NarrativeMixDirector(bool enabled = true) noexcept
        : enabled_(enabled) {}

    NarrativeMixDirective decide(const NarrativeAttentionDecision &attention,
                                 const OrganismSectionSnapshot &section,
                                 const NarrativeAttentionEngine::ProfileSet &profiles) noexcept {
        NarrativeMixDirective directive;
        if (!enabled_ || !attention.valid || !attention.finite()
            || attention.focus == OrganismStem::Count
            || !manifested(section[index(attention.focus)])) {
            return directive;
        }

        OrganismStem chosen = attention.focus;
        bool held = false;
        // A newly chosen focus may replace the current one only when its
        // evidence is materially stronger. The incumbent must itself still be
        // manifest: continuity is never permission to boost an absent body.
        if (hasFocus_ && chosen != previousFocus_
            && manifested(section[index(previousFocus_)])
            && attention.reason != NarrativeAttentionReason::Emergence
            && attention.reason != NarrativeAttentionReason::Gesture
            && attention.reason != NarrativeAttentionReason::Consequence
            && attention.value < previousValue_ + kSwitchMargin) {
            chosen = previousFocus_;
            held = true;
        }

        const auto &profile = profiles[index(chosen)];
        const double evidence = std::clamp((attention.value - 0.22) / 0.78, 0.0, 1.0);
        if (evidence < 0.10) return directive;

        double reasonWeight = 0.72;
        switch (held ? NarrativeAttentionReason::Continuity : attention.reason) {
            case NarrativeAttentionReason::Emergence: reasonWeight = 1.00; break;
            case NarrativeAttentionReason::Gesture: reasonWeight = 1.00; break;
            case NarrativeAttentionReason::Consequence: reasonWeight = 0.92; break;
            case NarrativeAttentionReason::Contrast: reasonWeight = 0.82; break;
            case NarrativeAttentionReason::Continuity: reasonWeight = 0.58; break;
            case NarrativeAttentionReason::Presence: reasonWeight = 0.72; break;
            case NarrativeAttentionReason::None: break;
        }
        if (profile.role == OrchestralRole::Event) reasonWeight *= 1.05;
        if (profile.role == OrchestralRole::Shadow) reasonWeight *= 0.78;

        directive.valid = true;
        directive.focus = chosen;
        directive.reason = held ? NarrativeAttentionReason::Continuity : attention.reason;
        directive.gainDb = std::clamp((0.22 + 1.03 * evidence) * reasonWeight,
                                      0.0, 1.25);
        directive.heldByHysteresis = held;

        previousFocus_ = chosen;
        previousValue_ = attention.value;
        hasFocus_ = true;
        ++decisionCount_;
        if (held) ++hysteresisHoldCount_;
        if (profile.role == OrchestralRole::Event) ++eventDirectiveCount_;
        maximumGainDb_ = std::max(maximumGainDb_, directive.gainDb);
        lastDirective_ = directive;
        return directive;
    }

    bool enabled() const noexcept { return enabled_; }
    int decisionCount() const noexcept { return decisionCount_; }
    int hysteresisHoldCount() const noexcept { return hysteresisHoldCount_; }
    int eventDirectiveCount() const noexcept { return eventDirectiveCount_; }
    double maximumGainDb() const noexcept { return maximumGainDb_; }
    const NarrativeMixDirective &lastDirective() const noexcept {
        return lastDirective_;
    }

private:
    static constexpr std::size_t index(OrganismStem stem) noexcept {
        return static_cast<std::size_t>(stem);
    }

    static bool manifested(const OrganismSectionMetrics &metrics) noexcept {
        return metrics.frames > 0 && std::isfinite(metrics.rms)
            && metrics.rms > 1.0e-9 && std::isfinite(metrics.audibleRatio)
            && metrics.audibleRatio > 0.004;
    }

    static constexpr double kSwitchMargin = 0.10;
    bool enabled_ = true;
    bool hasFocus_ = false;
    OrganismStem previousFocus_ = OrganismStem::Other;
    double previousValue_ = 0.0;
    int decisionCount_ = 0;
    int hysteresisHoldCount_ = 0;
    int eventDirectiveCount_ = 0;
    double maximumGainDb_ = 0.0;
    NarrativeMixDirective lastDirective_{};
};

} // namespace rasgo
