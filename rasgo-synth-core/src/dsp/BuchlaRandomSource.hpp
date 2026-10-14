#pragma once
#include <array>
#include <cstdint>

namespace rasgo {

// Modeled on Don Buchla's Random Voltage Source from the Music Easel /
// Model 208 "Stored Program Sound Source" (the schematic redrawn by Scott
// Stites in 2004 that this project used as reference): an 8-bit shift
// register clocked by an incoming pulse, with the input bit fed not from
// outside but from an XOR of two tapped register outputs -- a linear
// feedback shift register (LFSR), the same structure often built from a
// CD4015 + logic gates in the Lunetta/Buchla world, done here in a plain
// array of bits. On its own, an LFSR just produces a long, deterministic-
// but-unpredictable-feeling bit sequence (period up to 2^8-1 = 255 clocks
// for an 8-bit register with a maximal-length tap set).
//
// The elegant part of the original circuit, and the reason this is worth
// keeping distinct from a single random bit source, is that it doesn't stop
// at one output: several *different weighted combinations* of the same 8
// bits are summed into several independent-sounding control voltages. They
// are not statistically independent -- they are different "readings" of
// the same underlying random state -- which gives a family of modulation
// sources that drift together in a way that feels related, rather than
// four uncorrelated noise generators. That's a genuinely different flavor
// of "randomico" from the continuous chaotic maps (Henon/Lorenz/
// Latoocarfian) already in this engine: this one is stepped (updates once
// per clock pulse, holds between updates) rather than continuously
// drifting.
class BuchlaRandomSource {
public:
    static constexpr int kNumBits = 8;
    static constexpr int kNumOutputs = 4;

    explicit BuchlaRandomSource(uint8_t seed = 0b10110101) {
        for (int i = 0; i < kNumBits; ++i) {
            bits_[static_cast<size_t>(i)] = (seed >> i) & 1;
        }
    }

    // Advances the register by one clock pulse (call this at whatever rate
    // you want the "sample and hold" to update -- audio rate for a buzzy/
    // grainy texture, a few Hz for classic slow-wandering CV behavior).
    void clock() {
        // Maximal-length 8-bit LFSR taps (bits 8,6,5,4 in 1-indexed
        // convention -> indices 7,5,4,3 here), giving the full 255-step
        // period before the sequence repeats -- the same kind of tap
        // selection the Buchla/Lunetta designs rely on for a sequence that
        // doesn't feel like it's obviously looping on a human timescale.
        bool feedback = bits_[7] ^ bits_[5] ^ bits_[4] ^ bits_[3];
        for (int i = kNumBits - 1; i > 0; --i) {
            bits_[static_cast<size_t>(i)] = bits_[static_cast<size_t>(i - 1)];
        }
        bits_[0] = feedback;
    }

    // Returns kNumOutputs values in roughly [-1, 1], each a different
    // weighted sum of the 8 register bits -- analogous to the UCV1-4
    // outputs of the original circuit's resistor-weighted summing op-amps.
    std::array<float, kNumOutputs> outputs() const {
        // Weight sets loosely mirroring the original's mix of "heavy" and
        // "light" resistor taps (bigger weight = that bit dominates more),
        // each output emphasizing a different subset/ordering of the bits
        // so the four outputs correlate without being identical.
        static const float weights[kNumOutputs][kNumBits] = {
            {0.50f, 0.25f, 0.13f, 0.06f, 0.03f, 0.015f, 0.008f, 0.004f},
            {0.004f, 0.008f, 0.015f, 0.03f, 0.06f, 0.13f, 0.25f, 0.50f},
            {0.13f, 0.50f, 0.06f, 0.25f, 0.008f, 0.03f, 0.004f, 0.015f},
            {0.03f, 0.004f, 0.25f, 0.008f, 0.50f, 0.015f, 0.13f, 0.06f},
        };
        std::array<float, kNumOutputs> result{};
        for (int o = 0; o < kNumOutputs; ++o) {
            float sum = 0.0f;
            for (int b = 0; b < kNumBits; ++b) {
                sum += (bits_[static_cast<size_t>(b)] ? 1.0f : -1.0f) * weights[o][b];
            }
            result[static_cast<size_t>(o)] = sum; // already roughly in [-1, 1] since weights sum to ~1
        }
        return result;
    }

private:
    std::array<bool, kNumBits> bits_{};
};

// Wraps BuchlaRandomSource with its own clock, so it can be driven by a
// frequency in Hz rather than manual clock() calls -- convenient for using
// it as a standard modulation source alongside the oscillators and chaos
// maps.
class ClockedRandomSource {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setClockFrequency(double hz) { clockHz_ = hz; }

    std::array<float, BuchlaRandomSource::kNumOutputs> tick() {
        phase_ += clockHz_ / sampleRate_;
        if (phase_ >= 1.0) {
            phase_ -= 1.0;
            source_.clock();
        }
        return source_.outputs();
    }

private:
    double sampleRate_ = 44100.0;
    double clockHz_ = 8.0;
    double phase_ = 0.0;
    BuchlaRandomSource source_;
};

} // namespace rasgo
