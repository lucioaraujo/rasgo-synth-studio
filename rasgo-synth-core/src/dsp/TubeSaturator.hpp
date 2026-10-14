#pragma once
#include <cmath>
#include <algorithm>

namespace rasgo {

// Ports the idea behind a valve/tube saturator (Pantala Labs' "Tube
// Control" among many others) -- a different harmonic signature from
// TapeSaturator.hpp's rounded transients or DiodeShaper's asymmetric
// rectifier curve: real tube stages bias toward even-order harmonics via
// an asymmetric transfer curve (the two halves of the waveform see
// slightly different gain), which reads as "warmer"/"fuller" rather than
// "gritty" the way odd-harmonic-heavy clipping does.
class TubeSaturator {
public:
    void setDrive(float drive) { drive_ = std::clamp(drive, 0.0f, 1.0f); }
    void setBias(float bias) { bias_ = bias; } // small DC-like asymmetry before shaping

    float process(float input) {
        float driven = (input + bias_ * 0.1f) * (1.0f + drive_ * 2.5f);
        // Asymmetric soft clip: positive and negative halves use
        // different curves, the even-harmonic-biased signature real
        // valve stages have that a symmetric tanh doesn't.
        float shaped;
        if (driven >= 0.0f) {
            shaped = 1.0f - std::exp(-driven);
        } else {
            shaped = -1.0f + std::exp(driven * 1.3f);
        }
        return shaped * 0.85f;
    }

private:
    float drive_ = 0.35f;
    float bias_ = 0.0f;
};

} // namespace rasgo
