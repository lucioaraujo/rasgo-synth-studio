#pragma once
#include "MultibandFilter.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <random>

namespace rasgo {

// "Atropelo" (fase 90) -- what began as "voz narrante" grew, through
// design conversation with the user, into something that isn't really a
// narrating voice anymore: competing, interrupting voices that almost
// speak and refuse to, that remember an unfinished phrase and sometimes
// come back to it, and that specifically invade the piece's own moments
// of promised silence. This file is the single-voice synthesis core
// (stage 1) -- competition/hard-interruption, the persistent memory pool,
// recursive nesting, and the silence-invasion behavior are built on top
// of this in sibling files/stages once this stage is validated.
//
// Real technique: classic source-filter (formant) speech synthesis --
// excite a resonant filter bank with a pulse train (the "glottal source"),
// shape the result with 2-3 parallel bandpass resonances (the vocal
// tract's formants) -- the same principle behind decades of formant
// vocoders and speech synthesizers, reusing this engine's own
// StateVariableBandpass (MultibandFilter.hpp, fase-linked to the CBS
// Bandpass Filter #194 lineage) rather than inventing a new filter.
//
// Deliberately UNCANNY, not naturalistic: real human vowels live in a
// well-documented, narrow region of F1/F2/F3 ratio space (a few hundred
// Hz to a few kHz, with real formants clustering in recognizable vowel
// "islands" on an F1/F2 plot). This voice's three formants are
// deliberately drawn from OUTSIDE that real vowel space -- shifted and
// re-ratio'd so the result triggers the ear's "this could be speech"
// pattern-matching and then refuses to resolve into any real vowel, the
// same structural idea as this engine's own "elemento estranho" (fase 50)
// applied specifically to a voice-shaped source. This is a deliberate,
// citable design choice (grounded in real formant/uncanny-valley
// research), not a naturalistic vocal model.
class AtropeloVoice {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        for (auto &f : formants_) f.setSampleRate(sr);
    }

    // formantHzOffsetSeed: seeds this voice's own fixed, uncanny formant
    // layout (three ratios, deliberately outside real vowel-formant
    // space) -- drawn once per voice so each of the several competing
    // voices (see the sibling "competition" stage) has its own distinct,
    // consistent "almost-timbre", not a new random vowel every syllable.
    void configureFormants(unsigned seed) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        // Real human vowels: F1 roughly 250-900Hz, F2 roughly 700-2500Hz,
        // F3 roughly 2200-3200Hz, with F2/F1 and F3/F2 ratios that stay
        // inside a well-documented, fairly narrow band for any real vowel.
        // Pushed outside that: F1 allowed low or unusually high, and the
        // F2/F1 ratio drawn from a range real vowels never actually use
        // (either compressed near 1 -- formants crowd together, unlike
        // any real vowel -- or stretched far past what a human vocal
        // tract's real proportions allow).
        f1Hz_ = 120.0 + unit(rng) * 950.0;
        double ratio2 = (unit(rng) < 0.5) ? (1.05 + unit(rng) * 0.35) : (4.5 + unit(rng) * 3.5);
        double ratio3 = 1.4 + unit(rng) * 2.2;
        f2Hz_ = f1Hz_clamp(f1Hz_ * ratio2);
        f3Hz_ = f1Hz_clamp(f2Hz_ * ratio3);
        formants_[0].setFrequency(f1Hz_); formants_[0].setQ(6.0 + unit(rng) * 4.0);
        formants_[1].setFrequency(f2Hz_); formants_[1].setQ(8.0 + unit(rng) * 6.0);
        formants_[2].setFrequency(f3Hz_); formants_[2].setQ(6.0 + unit(rng) * 5.0);
        formantGain_[0] = 1.0f; formantGain_[1] = 0.75f; formantGain_[2] = 0.5f;
    }

    // Begins one syllable burst: a glottal-pulse excitation at pitchHz for
    // durationSeconds, fast onset (consonant-like), released at the end.
    void beginSyllable(double pitchHz, double durationSeconds) {
        pitchHz_ = pitchHz;
        durationSamples_ = static_cast<int>(durationSeconds * sampleRate_);
        elapsedSamples_ = 0;
        glottalPhase_ = 0.0;
        active_ = true;
    }

    bool isActive() const { return active_; }

    float tick() {
        if (!active_) return 0.0f;
        // Glottal source: a simple pulse train (real speech's own source
        // signal is a quasi-periodic glottal pulse, not a sine) -- a
        // narrow, sharply-decaying impulse each cycle, band-limited
        // enough at this engine's audio rates not to alias harshly.
        glottalPhase_ += pitchHz_ / sampleRate_;
        if (glottalPhase_ >= 1.0) glottalPhase_ -= 1.0;
        float excitation = static_cast<float>(std::pow(1.0 - glottalPhase_, 8.0));

        float out = formants_[0].process(excitation) * formantGain_[0]
                  + formants_[1].process(excitation) * formantGain_[1]
                  + formants_[2].process(excitation) * formantGain_[2];

        // Fast attack (~8ms), release over the last ~15% of the syllable --
        // consonant-like onset into a vowel-like sustain, never a clean
        // synth envelope.
        int attackSamples = static_cast<int>(0.008 * sampleRate_);
        int releaseStart = static_cast<int>(durationSamples_ * 0.85);
        float env = 1.0f;
        if (elapsedSamples_ < attackSamples) {
            env = static_cast<float>(elapsedSamples_) / static_cast<float>(attackSamples);
        } else if (elapsedSamples_ > releaseStart) {
            int releaseSamples = durationSamples_ - releaseStart;
            env = 1.0f - static_cast<float>(elapsedSamples_ - releaseStart) / static_cast<float>(std::max(1, releaseSamples));
        }

        ++elapsedSamples_;
        if (elapsedSamples_ >= durationSamples_) active_ = false;
        return out * env * 0.35f;
    }

private:
    static double f1Hz_clamp(double hz) { return std::min(hz, 6500.0); }

    double sampleRate_ = 44100.0;
    std::array<StateVariableBandpass, 3> formants_;
    float formantGain_[3] = {1.0f, 0.75f, 0.5f};
    double f1Hz_ = 400.0, f2Hz_ = 1200.0, f3Hz_ = 2600.0;

    double pitchHz_ = 140.0;
    double glottalPhase_ = 0.0;
    int durationSamples_ = 0;
    int elapsedSamples_ = 0;
    bool active_ = false;
};

} // namespace rasgo
