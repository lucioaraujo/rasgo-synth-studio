#pragma once
#include <algorithm>

namespace rasgo {

// The last software-side pendency: an attack/release envelope generator
// run fast enough, and free-running (self-retriggering) enough, that it
// becomes an audio-rate oscillator rather than a one-shot contour --
// inspired by the CBS Musical Instruments "Dual Attack Generator #180",
// whose "Audio Attack" naming points at exactly this technique, well known
// in modular synthesis as "AD/AR as VCO" (Buchla/Serge-style patches).
//
// Unlike a fixed waveform, the shape parameter controls the balance
// between the attack (rising) and release (falling) portions of each
// cycle: near 0 gives a sharp attack + long decay (sawtooth-ish, bright
// falling edge), 0.5 gives a roughly symmetric triangle-like shape, and
// near 1 gives a slow rise + sharp release (reversed character). This is
// a different kind of "morph" than MorphOscillator's classic waveform
// sweep -- it comes from envelope-generator thinking, not oscillator
// thinking, which is the reason for this independent reinterpretation.
class EnvelopeOscillator {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setFrequency(double hz) { frequencyHz_ = std::max(1.0, hz); }

    // 0..1: fraction of each cycle spent rising (attack) vs falling
    // (release). Kept away from the exact extremes so neither segment
    // collapses to zero samples.
    void setShape(float shape) { shape_ = std::clamp(shape, 0.02f, 0.98f); }

    float tick() {
        double periodSamples = sampleRate_ / frequencyHz_;
        double attackSamples = std::max(1.0, periodSamples * shape_);
        double releaseSamples = std::max(1.0, periodSamples * (1.0 - shape_));

        if (stage_ == Stage::Attack) {
            value_ += 1.0 / attackSamples;
            if (value_ >= 1.0) { value_ = 1.0; stage_ = Stage::Release; }
        } else {
            value_ -= 1.0 / releaseSamples;
            if (value_ <= 0.0) { value_ = 0.0; stage_ = Stage::Attack; }
        }

        return static_cast<float>(value_ * 2.0 - 1.0); // bipolar for direct audio use
    }

private:
    enum class Stage { Attack, Release };

    double sampleRate_ = 44100.0;
    double frequencyHz_ = 110.0;
    float shape_ = 0.15f; // sharp attack, longer release -- sawtooth-ish default
    double value_ = 0.0;
    Stage stage_ = Stage::Attack;
};

} // namespace rasgo
