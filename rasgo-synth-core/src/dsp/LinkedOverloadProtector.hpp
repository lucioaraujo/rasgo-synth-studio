#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace rasgo {

// A matched stereo overload knee for the summed creative bus. Below the
// threshold this is mathematically an identity operation. Above it, both
// channels use the same continuous transfer curve. This avoids the abrupt
// gain changes a peak-derived linked multiplier can create when the louder
// channel alternates, while preventing dozens of individually-safe sources
// from forcing the final true-peak limiter into tens of decibels of reduction.
// It is overload protection, not loudness normalization or an always-on tone.
class LinkedOverloadProtector {
public:
    void setThresholdAndLimit(float threshold, float limit) {
        threshold_ = std::clamp(threshold, 0.1f, 1.0f);
        limit_ = std::clamp(limit, threshold_ + 0.01f, 2.0f);
    }

    void process(float inL, float inR, float &outL, float &outR) {
        if (!std::isfinite(inL)) inL = 0.0f;
        if (!std::isfinite(inR)) inR = 0.0f;
        const float peak = std::max(std::fabs(inL), std::fabs(inR));
        if (peak > threshold_) {
            ++overloadFrames_;
            const float protectedPeak = protectMagnitude(peak);
            const float reduction = 20.0f * std::log10(peak / std::max(protectedPeak, 1.0e-9f));
            maxReductionDb_ = std::max(maxReductionDb_, reduction);
        }
        outL = protectSample(inL);
        outR = protectSample(inR);
    }

    std::size_t overloadFrames() const { return overloadFrames_; }
    float maxReductionDb() const { return maxReductionDb_; }

private:
    float protectMagnitude(float magnitude) const {
        if (magnitude <= threshold_) return magnitude;
        const float width = limit_ - threshold_;
        return threshold_ + width * std::tanh((magnitude - threshold_) / width);
    }

    float protectSample(float sample) const {
        return std::copysign(protectMagnitude(std::fabs(sample)), sample);
    }

    float threshold_ = 0.78f;
    float limit_ = 1.18f;
    std::size_t overloadFrames_ = 0;
    float maxReductionDb_ = 0.0f;
};

} // namespace rasgo
