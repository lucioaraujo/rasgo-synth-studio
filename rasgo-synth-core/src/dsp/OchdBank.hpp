#pragma once
#include <array>
#include <cmath>
#include <random>

namespace rasgo {

// Instruo's ochd, designed in collaboration with Ben "DivKid" Wilson, prompted
// the idea of eight LFO/clock outputs derived from one shared
// base rate, each scaled by a different ratio (a "chord" of rates rather
// than eight unrelated free-running LFOs) so the outputs breathe in and out
// of phase relationships with each other instead of drifting independently
// -- an "organically coupled" character. This is an original digital design,
// not a circuit model and not a claim to reproduce ochd's internal ratios.
// Each channel here gets its own fixed ratio plus a small seed-derived detune, and on top of
// that a slow random-walk wobble on its instantaneous rate (a few % over
// tens of seconds) -- together they mean the whole ensemble very slowly
// slides in and out of any simple relationship rather than looping a fixed
// pattern, in keeping with this engine's chaos-over-repetition bias.
class OchdBank {
public:
    static constexpr int kNumChannels = 8;

    void setSampleRate(double sr) { sampleRate_ = sr; }

    // baseHz: the slowest channel's rate; the other seven scale up from it.
    void configure(double baseHz, unsigned seed) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> detune(-0.05, 0.05);
        std::uniform_real_distribution<double> phase01(0.0, 1.0);
        static constexpr double kRatios[kNumChannels] = {
            1.0, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0, 6.0
        };
        for (int i = 0; i < kNumChannels; ++i) {
            ratio_[i] = kRatios[i] * (1.0 + detune(rng));
            phase_[i] = phase01(rng);
            wobblePhase_[i] = phase01(rng) * 6.283185307179586;
        }
        baseHz_ = baseHz;
    }

    std::array<float, kNumChannels> tick() {
        std::array<float, kNumChannels> out;
        for (int i = 0; i < kNumChannels; ++i) {
            // Slow, smooth +/-4% rate wobble (its own slow sine, not audio-rate
            // noise) -- enough to detune the ensemble's rate relationships
            // over time without ever repeating a fixed loop.
            wobblePhase_[i] += (0.008 + 0.004 * ratio_[i]) / std::max(1.0, sampleRate_);
            double wobble = 1.0 + 0.04 * std::sin(wobblePhase_[i] * 2.0 * 3.141592653589793);

            double inc = (baseHz_ * ratio_[i] * wobble) / std::max(1.0, sampleRate_);
            phase_[i] += inc;
            phase_[i] -= std::floor(phase_[i]);
            out[i] = static_cast<float>(std::sin(2.0 * 3.141592653589793 * phase_[i]));
        }
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double baseHz_ = 0.05;
    double ratio_[kNumChannels] {};
    double phase_[kNumChannels] {};
    double wobblePhase_[kNumChannels] {};
};

} // namespace rasgo
