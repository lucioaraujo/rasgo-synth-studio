#pragma once
#include <random>
#include <algorithm>

namespace rasgo {

// KS Synthesis's "4006 Randomizer Core" (Scott Stites) -- the standout idea
// out of the batch of sequencer/randomizer schematics surveyed alongside
// the Lunetta modules (see PolyrhythmDivider.hpp/DrumVoices.hpp): a CD4006
// static shift register whose new bit isn't just feedback from earlier
// taps (the mechanism BuchlaRandomSource above already models, an LFSR) but
// that feedback mixed with a DC "Probability" bias voltage before being
// thresholded into a 1 or a 0. Turning the Probability pot doesn't reorder
// a fixed sequence -- it skews the long-run ratio of 1s to 0s from "almost
// always 0" through "fair coin" to "almost always 1". That's a genuinely
// different kind of randomness from the LFSR above: probability-skewed
// rather than deterministic-but-unpredictable-feeling.
//
// The real module also has a "Loop" switch that freezes whatever pattern
// is currently in the register and repeats it. That part is deliberately
// omitted here: this engine's whole premise is that nothing repeats
// verbatim within a piece (see the manual's Concept section), and a
// freeze/loop control is the one idea in that schematic that runs directly
// against it. The Probability bias, on its own, doesn't have that
// conflict -- it shapes a distribution, it doesn't stop the piece from
// moving -- so only that half made the cut.
class KSRandomizerCore {
public:
    void seed(unsigned s) { rng_.seed(s); }

    // bias in roughly [-1, 1]: -1 skews heavily toward 0/low outcomes, 0 is
    // a fair coin, +1 skews heavily toward 1/high outcomes -- the
    // Probability pot on the real module.
    void setProbability(double bias) {
        probability_ = std::clamp(0.5 + bias * 0.48, 0.02, 0.98);
    }

    // Draws one biased bit, returned as -1.0/+1.0 to match the bipolar CV
    // convention the other random sources in this engine use.
    float clock() {
        std::bernoulli_distribution dist(probability_);
        return dist(rng_) ? 1.0f : -1.0f;
    }

private:
    std::mt19937 rng_;
    double probability_ = 0.5;
};

} // namespace rasgo
