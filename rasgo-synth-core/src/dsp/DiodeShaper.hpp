#pragma once
#include <cmath>

namespace rasgo {

// Approximates the "diode mixing" character described across the CMOS/
// Lunetta references gathered for this project (Beavis Audio's CMOS
// Synthesizers page, fluxmonkey's Looney Board docs): instead of linearly
// summing oscillator outputs (a plain resistive mix, which just attenuates),
// each signal is passed through an asymmetric, diode-like transfer curve
// before combining -- a real diode conducts easily in one direction and
// barely at all in the other. The result is a half-wave-rectified,
// harmonically richer character, distinctly different from a resistive mix,
// matching what those sources describe as "a very interesting variation."
inline float diodeShape(float x, float drive = 3.0f) {
    float d = x * drive;
    if (d >= 0.0f) {
        // forward-biased: conducts, saturates smoothly
        return (1.0f - std::exp(-d)) * 0.9f;
    } else {
        // reverse-biased: mostly blocked, small leakage
        return (1.0f - std::exp(d * 0.15f)) * 0.15f;
    }
}

inline float diodeMix(float a, float b, float drive = 3.0f) {
    return diodeShape(a, drive) + diodeShape(b, drive);
}

// A lightly-oversampled version of diodeMix for the primary voice. Any
// nonlinear waveshaper -- and diodeShape() very much is one -- generates
// harmonic energy the input signal didn't have, some of which lands above
// Nyquist if computed at the plain sample rate, folding back as aliasing
// the same way an unfiltered saw/pulse oscillator would (see Oscillator.hpp
// and MorphOscillator.hpp's PolyBLEP correction for the same problem at
// the oscillator stage). Self-contained (keeps its own one-sample history)
// so the caller's per-sample loop doesn't change shape at all -- swap the
// free function for this stateful one. Approximates 2x oversampling via
// linear interpolation for the upsample and averaging for the decimate
// step -- not a full polyphase design, but a real, cheap reduction in
// shaper-introduced aliasing on the voice that carries this engine's
// primary timbre.
class DiodeMixer {
public:
    float process(float a, float b, float drive = 3.0f) {
        float midA = (prevA_ + a) * 0.5f;
        float midB = (prevB_ + b) * 0.5f;
        float shapedMid = diodeShape(midA, drive) + diodeShape(midB, drive);
        float shapedNow = diodeShape(a, drive) + diodeShape(b, drive);
        prevA_ = a;
        prevB_ = b;
        return (shapedMid + shapedNow) * 0.5f;
    }

private:
    float prevA_ = 0.0f, prevB_ = 0.0f;
};

} // namespace rasgo
