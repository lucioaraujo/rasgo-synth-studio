#pragma once
#include <algorithm>
#include <cstdint>
#include <random>

namespace rasgo {

// Ports the "Random Gates" section of Ray Wilson's Noise Cornucopia: a
// comparator watches a (rate-limited, not full-bandwidth) noise source and
// produces an irregularly-timed pulse train wherever the noise crosses
// zero, which then clocks a CD4024 7-stage binary ripple counter -- tapping
// one of its later stages divides that already-irregular clock down
// further. The result is a trigger source whose timing has a genuinely
// different statistical character from this engine's other two trigger
// sources: the PolyrhythmDivider's XOR clock (deterministic, tightly
// gridded) and the chaos maps' zero-crossings (smooth, continuous chaos).
// This one is noise-derived -- irregular at the source, not just complex.
class NoiseGateGenerator {
public:
    void seed(unsigned s) { rng_.seed(s); }

    // How often the underlying noise value itself updates -- the real
    // circuit's noise is filtered, not full audio bandwidth, so this
    // stands in for that filtering (also keeps the divided-down output at
    // a musically useful rate rather than many kHz).
    void setNoiseRate(double hz) { noiseRateHz_ = hz; }

    // Which CD4024 stage to tap: divides the noise-crossing clock by 2^stage.
    void setDivider(int stage) { divStage_ = std::clamp(stage, 1, 7); }

    void setSampleRate(double sr) { sampleRate_ = sr; }

    // Call once per sample. Returns true exactly on the sample where the
    // selected divided-down output has a rising edge.
    bool tick() {
        bool risingEdgeOut = false;
        notePhase_ += noiseRateHz_ / sampleRate_;
        if (notePhase_ >= 1.0) {
            notePhase_ -= 1.0;
            float noise = unit_(rng_) * 2.0f - 1.0f;
            bool aboveZero = noise > 0.0f;
            if (aboveZero && !lastAboveZero_) {
                ++counter_;
                uint32_t mask = 1u << (divStage_ - 1);
                bool newBit = (counter_ & mask) != 0;
                if (newBit && !lastDividedBit_) risingEdgeOut = true;
                lastDividedBit_ = newBit;
            }
            lastAboveZero_ = aboveZero;
        }
        return risingEdgeOut;
    }

private:
    double sampleRate_ = 44100.0;
    double noiseRateHz_ = 60.0;
    double notePhase_ = 0.0;
    int divStage_ = 4;
    uint32_t counter_ = 0;
    bool lastAboveZero_ = false;
    bool lastDividedBit_ = false;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> unit_{0.0f, 1.0f};
};

} // namespace rasgo
