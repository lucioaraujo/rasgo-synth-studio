#pragma once
#include <random>

namespace rasgo {

// Mutable Instruments' Branches: a probabilistic router for a trigger/gate
// stream. Each incoming pulse goes to one of two outputs -- which one is a
// coin flip biased by a probability knob, in "Bernoulli" mode; in "Toggle"
// mode (the module's other switch position) it alternates deterministically
// instead, same idea as a flip-flop. Used here to route a single clock edge
// between two different outcomes for a percussion voice instead of that
// edge always doing the exact same thing.
class BernoulliGate {
public:
    enum class Mode { Bernoulli, Toggle };

    void seed(unsigned s) { rng_.seed(s); }
    void setMode(Mode m) { mode_ = m; }
    void setProbability(double p) { probability_ = p; }
    Mode mode() const { return mode_; }
    double probability() const { return probability_; }

    // Call once per incoming trigger. Returns true for "branch B", false
    // for "branch A".
    bool next() {
        if (mode_ == Mode::Toggle) {
            toggleState_ = !toggleState_;
            return toggleState_;
        }
        return unit_(rng_) < probability_;
    }

private:
    Mode mode_ = Mode::Bernoulli;
    double probability_ = 0.5;
    bool toggleState_ = false;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> unit_{0.0, 1.0};
};

} // namespace rasgo
