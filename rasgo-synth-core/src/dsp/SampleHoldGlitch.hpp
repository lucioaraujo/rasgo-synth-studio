#pragma once
#include <random>

namespace rasgo {

// Real sample-rate corruption -- decimation, the honest DSP term for it.
// Instead of updating every sample, hold the last accepted value for an
// irregular, chaos-jittered number of samples before accepting a new one.
// The output is always literally one of the input's own past values --
// never synthesized, never amplified -- so it's bounded by construction
// to whatever range the input itself already was in. The glitch is
// entirely in WHEN it updates, not in the value itself, which is what
// keeps this safe to drop into a signal chain that assumes nothing
// upstream introduces its own amplitude risk.
class SampleHoldGlitch {
public:
    void seed(unsigned s) { rng_.seed(s); }
    void setHoldRange(int minSamples, int maxSamples) { minHold_ = minSamples; maxHold_ = maxSamples; }

    float process(float x) {
        if (counter_ <= 0) {
            held_ = x;
            std::uniform_int_distribution<int> dist(minHold_, maxHold_);
            counter_ = dist(rng_);
        }
        --counter_;
        return held_;
    }

private:
    float held_ = 0.0f;
    int counter_ = 0;
    int minHold_ = 2;
    int maxHold_ = 40;
    std::mt19937 rng_;
};

} // namespace rasgo
