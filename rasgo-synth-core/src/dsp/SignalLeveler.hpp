#pragma once
#include <cmath>
#include <algorithm>

namespace rasgo {

// Buchla Associates sketched a "Dual Signal Leveler" (Model 172) circuit
// that, per the handwritten note right on the schematic itself -- "We don't
// make. Why?" -- apparently never shipped (see resultat_fusion.pdf, the
// scanned schematic archive). Functionally it's an automatic gain unit:
// instead of just clipping peaks, it tracks a signal's average loudness and
// corrects toward a target level. Rasgo's narrative arc deliberately swings
// between very different densities/registers per section (a quiet
// "abertura" next to a loud "colapso"), and until now the only thing
// keeping the overall level in check across that swing was a single
// tanh() at the very end. This ports the *idea* -- a slow envelope
// follower driving a smoothed gain correction -- rather than the
// transistor-level circuit, the same "technique, not schematic" approach
// used for the rest of this engine's circuit-inspired modules.
class SignalLeveler {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        followCoeff_ = coeffFor(followMs_);
        riseCoeff_ = coeffFor(riseMs_);
        fallCoeff_ = coeffFor(fallMs_);
    }

    // Times in milliseconds: how fast the level-follower tracks the input
    // (followMs), and how fast the gain itself moves up vs. down once it
    // decides a correction is needed (riseMs/fallMs -- slower fall than
    // rise keeps it from visibly "pumping" on transients).
    void setTimes(double followMs, double riseMs, double fallMs) {
        followMs_ = followMs; riseMs_ = riseMs; fallMs_ = fallMs;
        followCoeff_ = coeffFor(followMs_);
        riseCoeff_ = coeffFor(riseMs_);
        fallCoeff_ = coeffFor(fallMs_);
    }

    void setTargetLevel(float target) { target_ = target; }
    void setGainRange(float minGain, float maxGain) { minGain_ = minGain; maxGain_ = maxGain; }

    float process(float input) {
        float rectified = std::abs(input);
        envelope_ += (rectified - envelope_) * static_cast<float>(followCoeff_);
        float desiredGain = envelope_ > 1e-5f
                                 ? std::clamp(target_ / envelope_, minGain_, maxGain_)
                                 : maxGain_;
        double coeff = desiredGain > gain_ ? riseCoeff_ : fallCoeff_;
        gain_ += (desiredGain - gain_) * static_cast<float>(coeff);
        return input * gain_;
    }

    // For stereo output: feed process() a mono sum so the gain decision
    // reacts to overall loudness, then apply this same smoothed gain to
    // both channels' true (non-summed) samples -- reading it back after
    // process() rather than calling process() twice keeps the two
    // channels from getting two independently-drifting gains, which would
    // narrow/wobble the stereo image instead of just taming loud passages.
    float currentGain() const { return gain_; }

private:
    double coeffFor(double ms) const {
        double samples = std::max(1.0, (ms / 1000.0) * sampleRate_);
        return 1.0 - std::exp(-1.0 / samples);
    }

    double sampleRate_ = 44100.0;
    double followMs_ = 60.0, riseMs_ = 120.0, fallMs_ = 400.0;
    double followCoeff_ = 0.01, riseCoeff_ = 0.01, fallCoeff_ = 0.01;
    float target_ = 0.3f;
    float minGain_ = 0.4f, maxGain_ = 2.5f;
    float envelope_ = 0.0f;
    float gain_ = 1.0f;
};

} // namespace rasgo
