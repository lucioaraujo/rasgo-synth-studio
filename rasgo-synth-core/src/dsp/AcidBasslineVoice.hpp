#pragma once
#include "Oscillator.hpp"
#include "PolivoksFilter.hpp"
#include <cmath>
#include <algorithm>

namespace rasgo {

// TB-303-style acid bassline: not a new circuit block, but the real
// IDENTITY that only shows up when three things a classic acid box always
// has together get combined into one voice -- a resonant, self-oscillation-
// capable lowpass (reusing PolivoksFilter's already-validated instability,
// fase 75) swept by its OWN fast envelope, independent of the amplitude
// envelope, plus the two per-step performance controls a real acid pattern
// is built from: accent (louder note, deeper filter sweep -- the classic
// "bark") and slide (the next note ties to this one: no envelope
// retrigger, pitch glides continuously instead of jumping). Building
// blocks (Polivoks filter, continuous pitch-following) already exist
// separately in this engine (fase 75, fase 88's theremin); this is their
// first combination into one recognisable acid voice rather than a
// generic bass patch.
inline double acidDecayCoeff(double seconds, double sampleRate) {
    double samples = std::max(1.0, seconds * sampleRate);
    return std::exp(std::log(0.001) / samples);
}

class AcidBasslineVoice {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        osc_.setSampleRate(sr);
        osc_.setWaveform(Oscillator::Waveform::Saw);
        filter_.setSampleRate(sr);
        filter_.setResonance(0.88);
        glideCoeff_ = std::exp(-1.0 / (0.05 * sr)); // ~50ms slide time, real 303-scale glide
        setDecayTimes(0.18, 0.12);
    }

    void setBaseCutoff(double hz) { baseCutoffHz_ = hz; }
    void setEnvDepth(double hz) { envDepthHz_ = hz; }
    void setResonance(double r) { filter_.setResonance(r); }

    void setDecayTimes(double ampSeconds, double filterSeconds) {
        ampDecay_ = acidDecayCoeff(ampSeconds, sampleRate_);
        filterDecay_ = acidDecayCoeff(filterSeconds, sampleRate_);
    }

    // Starts a fresh note. If slide is true, the amplitude/filter envelopes
    // are NOT retriggered -- only the target pitch changes and the
    // oscillator glides toward it, exactly the real 303 behaviour where two
    // adjacent slid steps read as one continuous note bending pitch rather
    // than two separate hits.
    void trigger(double targetFreqHz, bool accent, bool slide) {
        targetFreq_ = targetFreqHz;
        if (!slide) {
            currentFreq_ = targetFreqHz;
            ampEnv_ = 1.0;
            filterEnv_ = 1.0;
        }
        accentAmount_ = accent ? 1.0 : 0.55;
    }

    // Exposed for testability -- lets an isolated test confirm slide really
    // does skip the envelope retrigger instead of just eyeballing raw
    // oscillator output, which depends on waveform phase too.
    double ampEnvelope() const { return ampEnv_; }
    double currentFrequency() const { return currentFreq_; }

    float tick() {
        currentFreq_ += (targetFreq_ - currentFreq_) * (1.0 - glideCoeff_);
        osc_.setFrequency(currentFreq_);
        float raw = osc_.tick();

        double cutoff = baseCutoffHz_ + envDepthHz_ * filterEnv_ * accentAmount_;
        filter_.setCutoff(std::min(cutoff, sampleRate_ * 0.45));
        float filtered = filter_.process(raw);

        float out = filtered * static_cast<float>(ampEnv_ * (0.6 + 0.4 * accentAmount_));

        ampEnv_ *= ampDecay_;
        filterEnv_ *= filterDecay_;
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double targetFreq_ = 110.0, currentFreq_ = 110.0;
    double glideCoeff_ = 0.0;
    double baseCutoffHz_ = 300.0, envDepthHz_ = 2200.0;
    double ampEnv_ = 0.0, filterEnv_ = 0.0;
    double ampDecay_ = 0.0, filterDecay_ = 0.0;
    double accentAmount_ = 0.55;
    Oscillator osc_;
    PolivoksFilter filter_;
};

} // namespace rasgo
