#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace rasgo {

// Phase 168: deterministic temporal memory. It predicts a bounded push/pull
// residue for a voice, but owns no clock, RNG, scheduler or audio state.
// Integration into a renderer remains a separate, explicitly testable step.
enum class RasgarTimingVoice : unsigned char {
    Kick,
    Snare,
    Hihat,
    Cowbell,
    Count
};

class RasgarTimingMemory {
public:
    struct State {
        int section = -1;
        double biasSamples = 0.0;
        double momentumSamples = 0.0;
    };

    void reset() noexcept {
        states_ = {};
    }

    // Starts a section with a controlled morph toward its first observed
    // tendency. A new section never inherits the whole previous residue.
    void beginSection(RasgarTimingVoice voice, int section,
                      double seedBiasSamples = 0.0,
                      double morph = 0.35) noexcept {
        State &state = states_[index(voice)];
        const double safeMorph = std::clamp(morph, 0.0, 1.0);
        if (state.section != section) {
            state.section = section;
            state.biasSamples *= (1.0 - safeMorph);
            state.momentumSamples *= 0.5;
        }
        state.biasSamples += std::clamp(seedBiasSamples, -64.0, 64.0) * safeMorph;
    }

    // Learns a tendency with inertia. The result is always bounded by the
    // caller's residual budget, so memory cannot silently defeat the grid.
    double observe(RasgarTimingVoice voice, double requestedSamples,
                   double budgetSamples, double inertia = 0.25) noexcept {
        State &state = states_[index(voice)];
        const double budget = std::max(0.0, std::abs(budgetSamples));
        const double target = std::clamp(requestedSamples, -budget, budget);
        const double safeInertia = std::clamp(inertia, 0.0, 1.0);
        state.momentumSamples += (target - state.biasSamples) * safeInertia;
        state.momentumSamples = std::clamp(state.momentumSamples, -budget, budget);
        state.biasSamples = std::clamp(
            state.biasSamples + state.momentumSamples, -budget, budget);
        return state.biasSamples;
    }

    double predict(RasgarTimingVoice voice, double budgetSamples) const noexcept {
        const double budget = std::max(0.0, std::abs(budgetSamples));
        return std::clamp(states_[index(voice)].biasSamples, -budget, budget);
    }

    State state(RasgarTimingVoice voice) const noexcept {
        return states_[index(voice)];
    }

private:
    static constexpr std::size_t index(RasgarTimingVoice voice) noexcept {
        return static_cast<std::size_t>(voice);
    }

    std::array<State, static_cast<std::size_t>(RasgarTimingVoice::Count)> states_{};
};

} // namespace rasgo
