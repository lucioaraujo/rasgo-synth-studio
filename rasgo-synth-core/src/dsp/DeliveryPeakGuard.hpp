#pragma once

#include "TruePeakDetector.hpp"
#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

namespace rasgo {

// Original offline safeguard, NOT a replacement/certification of BS.1770.
// Normalized sinc/Hann, full input Nyquist, 8 phases, 64 source taps. No
// third-party coefficient bank/code. Keeping the normative meter independent
// makes its under-reading visible rather than relabelling this estimate.
class DeliveryPeakGuard {
public:
    static constexpr std::size_t kPhases = 8, kTaps = 64;
    static constexpr const char* methodId() { return "rasgo-sinc-hann8x64-v1"; }

    explicit DeliveryPeakGuard(bool boundedSkip = true) : boundedSkip_(boundedSkip) {
        constexpr double pi = 3.14159265358979323846;
        for (std::size_t phase = 1; phase < kPhases; ++phase) {
            auto& branch = coefficients_[phase - 1];
            double sum = 0;
            for (std::size_t tap = 0; tap < kTaps; ++tap) {
                const double d = static_cast<double>(tap) - (kTaps / 2 - 1)
                    - static_cast<double>(phase) / kPhases;
                const double window = .5 + .5 * std::cos(pi * d / (kTaps / 2));
                branch[tap] = std::sin(pi * d) / (pi * d) * window;
                sum += branch[tap];
            }
            double norm = 0;
            for (auto& coefficient : branch) {
                coefficient /= sum;
                norm += std::abs(coefficient);
            }
            maxL1_ = std::max(maxL1_, norm);
        }
    }

    void reset() { history_.fill(0); cursor_ = 0; peak_ = 0; }
    double peak() const { return peak_; }
    double process(float input) {
        if (!std::isfinite(input)) input = 0;
        history_[cursor_] = history_[cursor_ + kTaps] = input;
        cursor_ = (cursor_ + 1) % kTaps;
        const auto* samples = history_.data() + cursor_;
        peak_ = std::max(peak_, static_cast<double>(std::abs(input)));
        // Exact L1 upper bound for this finite bank, not a bound on every DAC.
        // Skip quiet windows only when no branch can exceed the existing peak.
        if (boundedSkip_) {
            float largest = 0;
            for (std::size_t i = 0; i < kTaps; ++i)
                largest = std::max(largest, std::abs(samples[i]));
            if (static_cast<double>(largest) * maxL1_
                    < peak_ * (1 - 8 * std::numeric_limits<double>::epsilon()))
                return peak_;
        }
        for (const auto& branch : coefficients_) {
            double value = 0;
            for (std::size_t i = 0; i < kTaps; ++i)
                value += branch[i] * samples[i];
            peak_ = std::max(peak_, std::abs(value));
        }
        return peak_;
    }

private:
    std::array<std::array<double, kTaps>, kPhases - 1> coefficients_{};
    std::array<float, kTaps * 2> history_{};
    std::size_t cursor_ = 0;
    double peak_ = 0, maxL1_ = 1;
    bool boundedSkip_;
};

struct DeliveryPeakReport {
    float preNormativePeak = 0, finalNormativePeak = 0, gain = 1;
    double preGuardPeak = 0, projectedGuardPeak = 0;
};
struct DeliveryGuardNoPoll { void operator()() const {} };

// Run on completed stereo PCM, after fades/edits, before recording. One new
// guard pass; the second pass remeasures the unchanged normative FIR. Constant
// gain preserves stereo ratios/dynamics, not loudness. Poll lets the existing
// worker callback throw its cancellation; no new thread/buffer/RT callback.
template<class Poll = DeliveryGuardNoPoll>
DeliveryPeakReport protectOfflineDelivery(std::vector<float>& samples,
                                         double sampleRate, Poll poll = {}) {
    if (samples.size() % 2 || !std::isfinite(sampleRate) || sampleRate <= 0)
        throw std::invalid_argument("Invalid stereo delivery input");
    TruePeakDetector left, right;
    left.prepare(sampleRate); right.prepare(sampleRate);
    DeliveryPeakGuard guardL, guardR;
    DeliveryPeakReport report;
    const std::size_t frames = samples.size() / 2;
    if (frames == 0) return report;
    // SRCs can reflect boundaries, whereas the normative FIR starts/ends at
    // zero. Inspect both finite boundary models; never put reflection into
    // the delivered PCM. Tail reflection repeats the final sample (the seam
    // lies between samples), unlike head reflection about the first sample.
    // Additional work is limited to 64 frames per edge.
    const auto reflected = [frames](std::size_t position) {
        if (frames == 1) return std::size_t{0};
        const auto period = 2 * (frames - 1);
        const auto index = position % period;
        return index < frames ? index : period - index;
    };
    DeliveryPeakGuard headL, headR;
    poll();
    for (std::size_t i = DeliveryPeakGuard::kTaps - 1; i > 0; --i) {
        const auto frame = reflected(i);
        headL.process(samples[frame * 2]); headR.process(samples[frame * 2 + 1]);
    }
    for (std::size_t i = 0; i < samples.size(); i += 2) {
        if (i % 8192 == 0) poll();
        if (!std::isfinite(samples[i]) || !std::isfinite(samples[i + 1]))
            throw std::invalid_argument("Non-finite stereo delivery input");
        report.preNormativePeak = std::max(report.preNormativePeak, left.process(samples[i]));
        report.preNormativePeak = std::max(report.preNormativePeak, right.process(samples[i + 1]));
        guardL.process(samples[i]); guardR.process(samples[i + 1]);
        if (i / 2 < DeliveryPeakGuard::kTaps) {
            headL.process(samples[i]); headR.process(samples[i + 1]);
        }
    }
    auto tailL = guardL, tailR = guardR;
    for (std::size_t i = 0; i < DeliveryPeakGuard::kTaps; ++i) {
        report.preNormativePeak = std::max(report.preNormativePeak, left.process(0));
        report.preNormativePeak = std::max(report.preNormativePeak, right.process(0));
        guardL.process(0); guardR.process(0);
        const auto frame = reflected(frames - 1 + i);
        tailL.process(samples[frame * 2]); tailR.process(samples[frame * 2 + 1]);
    }
    report.preGuardPeak = std::max({guardL.peak(), guardR.peak(), headL.peak(),
        headR.peak(), tailL.peak(), tailR.peak(),
        static_cast<double>(report.preNormativePeak)});
    if (!std::isfinite(report.preGuardPeak))
        throw std::invalid_argument("Stereo delivery peak overflow");
    constexpr float ceiling = .81283052f; // unchanged -1.8 dBTP estimate target
    if (report.preGuardPeak > ceiling) {
        report.gain = std::nextafter(static_cast<float>(ceiling / report.preGuardPeak), 0.f);
        for (std::size_t i = 0; i < samples.size(); ++i) {
            if (i % 8192 == 0) poll();
            samples[i] *= report.gain;
        }
    }
    // Linear projection, not a second guard measurement; float/PCM24 rounding
    // is covered by the delivery margin and independently tested at export.
    report.projectedGuardPeak = report.preGuardPeak * report.gain;
    left.reset(); right.reset();
    for (std::size_t i = 0; i < samples.size(); i += 2) {
        if (i % 8192 == 0) poll();
        report.finalNormativePeak = std::max(report.finalNormativePeak, left.process(samples[i]));
        report.finalNormativePeak = std::max(report.finalNormativePeak, right.process(samples[i + 1]));
    }
    for (std::size_t i = 0; i < TruePeakDetector::kTapsPerPhase; ++i) {
        report.finalNormativePeak = std::max(report.finalNormativePeak, left.process(0));
        report.finalNormativePeak = std::max(report.finalNormativePeak, right.process(0));
    }
    return report;
}
} // namespace rasgo
