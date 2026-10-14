#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include "TruePeakDetector.hpp"

namespace rasgo {

// Replaces a blanket tanh() applied to every single sample -- which, even
// though tanh is close to linear well below its knee, still shapes the
// signal's harmonic content a little all the time, not just when it
// actually needs to catch a peak. A lookahead limiter stays transparent
// (gain 1.0, no processing at all) whenever the signal is comfortably
// under the ceiling, which given SignalLeveler already keeps things near
// its target is the overwhelming majority of the time, and only reduces
// gain -- smoothly, starting a few milliseconds before the peak actually
// arrives -- when something genuinely needs it. Deliberately NOT a
// loudness-maximizing brickwall: the ceiling has real margin (0.95, about
// -0.45dBFS) and release is slow enough not to pump or audibly compress
// the chaotic dynamic range that's the whole character of this engine's
// output -- this is a safety net, not a mastering-loudness tool.
//
// Phase 118 replaces the old sample-peak envelope approximation with a
// four-times reconstructed-peak detector and an explicit gain ramp across
// the lookahead window. This is still a transparent safety stage, not an
// automatic loudness maximizer.
class LookaheadLimiter {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        lookaheadSamples_ = std::max<size_t>(1, static_cast<size_t>(std::llround(0.005 * sr)));
        delaySamples_ = lookaheadSamples_ + 1;
        delayL_.assign(delaySamples_, 0.0f);
        delayR_.assign(delaySamples_, 0.0f);
        writePos_ = 0;
        releaseCoeff_ = static_cast<float>(std::exp(-1.0 / (0.080 * sr)));
        inputDetectorL_.prepare(sr); inputDetectorR_.prepare(sr);
        outputDetectorL_.prepare(sr); outputDetectorR_.prepare(sr);
        resetState();
    }

    void setCeiling(float ceilingLinear) {
        ceiling_ = std::clamp(ceilingLinear, 0.01f, 1.0f);
        detectorCeiling_ = ceiling_ * std::pow(10.0f, -0.2f / 20.0f);
    }

    void process(float inL, float inR, float &outL, float &outR) {
        if (!std::isfinite(inL)) { inL = 0.0f; ++nonFiniteCount_; }
        if (!std::isfinite(inR)) { inR = 0.0f; ++nonFiniteCount_; }
        float detected = std::max(inputDetectorL_.process(inL), inputDetectorR_.process(inR));
        inputTruePeak_ = std::max(inputTruePeak_, detected);
        float requiredGain = detected > detectorCeiling_ && detected > 0.0f
            ? detectorCeiling_ / detected : 1.0f;
        if (requiredGain < targetGain_) {
            targetGain_ = requiredGain;
            if (attackRemaining_ == 0)
                attackRemaining_ = std::max<std::size_t>(1, lookaheadSamples_ - inputDetectorL_.latencySamples() - 2);
            attackIncrement_ = (targetGain_ - currentGain_) / static_cast<float>(attackRemaining_);
            holdRemaining_ = lookaheadSamples_;
        }
        if (attackRemaining_ > 0) {
            currentGain_ += attackIncrement_;
            if (--attackRemaining_ == 0) currentGain_ = targetGain_;
        } else if (holdRemaining_ > 0) {
            --holdRemaining_;
        } else {
            currentGain_ = 1.0f - (1.0f - currentGain_) * releaseCoeff_;
            targetGain_ = currentGain_;
        }
        currentGain_ = std::clamp(currentGain_, 0.0f, 1.0f);

        delayL_[writePos_] = inL;
        delayR_[writePos_] = inR;
        writePos_ = (writePos_ + 1) % delaySamples_;
        outL = std::clamp(delayL_[writePos_] * currentGain_, -ceiling_, ceiling_);
        outR = std::clamp(delayR_[writePos_] * currentGain_, -ceiling_, ceiling_);
        outputTruePeak_ = std::max(outputTruePeak_,
            std::max(outputDetectorL_.process(outL), outputDetectorR_.process(outR)));
        float reduction = currentGain_ > 0.0f ? -20.0f * std::log10(currentGain_) : 120.0f;
        maxGainReductionDb_ = std::max(maxGainReductionDb_, reduction);
    }

    std::size_t latencySamples() const { return lookaheadSamples_; }
    float inputTruePeak() const { return inputTruePeak_; }
    float outputTruePeak() const { return outputTruePeak_; }
    float maxGainReductionDb() const { return maxGainReductionDb_; }
    std::size_t nonFiniteCount() const { return nonFiniteCount_; }

private:
    void resetState() {
        currentGain_ = 1.0f; targetGain_ = 1.0f; attackIncrement_ = 0.0f;
        attackRemaining_ = 0; holdRemaining_ = 0;
        inputTruePeak_ = 0.0f; outputTruePeak_ = 0.0f; maxGainReductionDb_ = 0.0f;
        nonFiniteCount_ = 0;
    }

    double sampleRate_ = 44100.0;
    size_t delaySamples_ = 1;
    size_t lookaheadSamples_ = 1;
    std::vector<float> delayL_, delayR_;
    size_t writePos_ = 0;
    size_t attackRemaining_ = 0, holdRemaining_ = 0;
    float releaseCoeff_ = 0.999f;
    float currentGain_ = 1.0f, targetGain_ = 1.0f, attackIncrement_ = 0.0f;
    float ceiling_ = 0.89125094f;
    float detectorCeiling_ = 0.87096359f;
    float inputTruePeak_ = 0.0f, outputTruePeak_ = 0.0f, maxGainReductionDb_ = 0.0f;
    std::size_t nonFiniteCount_ = 0;
    TruePeakDetector inputDetectorL_, inputDetectorR_, outputDetectorL_, outputDetectorR_;
};

} // namespace rasgo
