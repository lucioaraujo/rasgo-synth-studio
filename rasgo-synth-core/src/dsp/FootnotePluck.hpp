#pragma once
#include "Oscillator.hpp"
#include <cmath>
#include <algorithm>

namespace rasgo {

// "Nota de rodape" (footnote): the timbre for a short, isolated 2-5 note
// gesture that plays once, at most, per piece. Two sine partials at a
// deliberately inharmonic ratio (never a clean interval like a fifth or
// an octave) so each pluck reads as its own small, slightly "off" bell
// rather than a clean harmonic tone -- distinct from every other pitched
// voice in this engine, all of which stay on some recognisable interval
// (unison, octave, fifth, or a tuned mode ratio).
inline double footnotePluckDecayCoeff(double seconds, double sampleRate) {
    double samples = std::max(1.0, seconds * sampleRate);
    return std::exp(std::log(0.001) / samples);
}

class FootnotePluck {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        osc1_.setSampleRate(sr);
        osc1_.setWaveform(Oscillator::Waveform::Sine);
        osc2_.setSampleRate(sr);
        osc2_.setWaveform(Oscillator::Waveform::Sine);
        decay_ = footnotePluckDecayCoeff(0.5, sr);
    }
    void setPartialRatio(double ratio) { partialRatio_ = ratio; }
    void setDecayTime(double seconds) { decay_ = footnotePluckDecayCoeff(seconds, sampleRate_); }

    // Retriggerable -- the same instance plays every note of the 2-5 note
    // gesture, one after another.
    void trigger(double freqHz) {
        osc1_.setFrequency(freqHz);
        osc2_.setFrequency(freqHz * partialRatio_);
        env_ = 1.0;
    }

    float tick() {
        float out = (osc1_.tick() * 0.7f + osc2_.tick() * 0.3f) * static_cast<float>(env_);
        env_ *= decay_;
        return out;
    }

    bool isActive() const { return env_ > 0.0005; }

private:
    double sampleRate_ = 44100.0;
    double partialRatio_ = 1.37;
    double env_ = 0.0;
    double decay_ = 0.0;
    Oscillator osc1_, osc2_;
};

} // namespace rasgo
