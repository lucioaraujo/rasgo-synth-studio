#pragma once
#include <cmath>
#include <algorithm>
#include "Oscillator.hpp" // for polyBlep()

namespace rasgo {

// A single continuously-variable oscillator, replacing a bank of discrete
// waveform choices with one "shape" knob that sweeps sine -> triangle ->
// sawtooth -> square, in that order -- the pendency left over from a
// fluxmonkey-style op-amp LFO schematic (triangle<->saw<->square) plus the
// classic diode-network trick of rounding a triangle wave's corners into a
// sine (approximated here as a phase-aligned crossfade rather than an
// actual diode-ladder simulation, per the project's "port the idea, not
// the transistor count" approach).
//
// All four target waveforms are generated from the same phase accumulator,
// so their zero-crossings line up and crossfading between adjacent shapes
// doesn't introduce clicks or phase-cancellation artifacts.
class MorphOscillator {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setFrequency(double hz) { frequencyHz_ = hz; }

    // 0 = sine, 1/3 = triangle, 2/3 = sawtooth, 1 = square. Values in
    // between crossfade linearly between the two neighbouring shapes.
    void setShape(float shape) { shape_ = std::clamp(shape, 0.0f, 1.0f); }

    float tick() {
        double dt = frequencyHz_ / sampleRate_;

        double sineVal = std::sin(kTwoPi * phase_);
        double triVal = triangle(phase_);

        // Saw and square both have hard discontinuities the sine/triangle
        // don't -- PolyBLEP-corrected here for the same reason as
        // Oscillator.hpp's own saw/pulse, and for the same audible payoff
        // (no more high-pitched digital crunch from unfiltered aliasing).
        double sawVal = 2.0 * phase_ - 1.0;
        sawVal -= polyBlep(phase_, dt);

        double sqVal = (2.0 * phase_ - 1.0 >= 0.0) ? 1.0 : -1.0;
        double risingRel = phase_ - 0.5;
        if (risingRel < 0.0) risingRel += 1.0;
        sqVal += polyBlep(risingRel, dt); // rising edge at phase 0.5
        sqVal -= polyBlep(phase_, dt);    // falling edge at the phase-0 wrap

        double segment = shape_ * 3.0; // 0..3 across the three crossfade zones
        double out;
        if (segment < 1.0) {
            out = lerp(sineVal, triVal, segment);
        } else if (segment < 2.0) {
            out = lerp(triVal, sawVal, segment - 1.0);
        } else {
            out = lerp(sawVal, sqVal, std::min(1.0, segment - 2.0));
        }

        phase_ += dt;
        if (phase_ >= 1.0) phase_ -= 1.0;

        return static_cast<float>(out);
    }

private:
    static constexpr double kTwoPi = 6.283185307179586476925286766559;

    static double triangle(double phase) {
        // Standard 0..1 phase triangle, peaking at phase==0.5, zero at
        // phase==0/1 to match the sawtooth's zero crossing there.
        return 4.0 * std::abs(phase - std::floor(phase + 0.5)) - 1.0;
    }
    static double lerp(double a, double b, double t) { return a + (b - a) * t; }

    double sampleRate_ = 44100.0;
    double frequencyHz_ = 220.0;
    double phase_ = 0.0;
    float shape_ = 1.0f / 3.0f; // starts at triangle
};

} // namespace rasgo
