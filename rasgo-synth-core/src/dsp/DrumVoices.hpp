#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include "Oscillator.hpp"
#include "ColoredNoise.hpp"
#include "SallenKeyFilter.hpp"

namespace rasgo {

// A small percussive layer -- something this engine has never had at all.
// Everything else in the render loop is melodic or textural (voice,
// resonator, granular wash); these three voices give the generative arc a
// rhythmic backbone modeled on the simple analog drum circuits that show
// up again and again in Lunetta-adjacent builds (dlbelectronics.com's
// system documents its own "DS8 clone" kick/snare voices built the same
// way). Each is an independently written oscillator/noise source shaped by a
// fast exponential-decay envelope. The voices are original digital
// stylisations inspired by familiar analogue-drum behaviours: not samples,
// physical models or circuit replicas.

// Exponential decay helper shared by all three voices below: given a
// desired decay time, returns the per-sample multiplier that gets a unit
// level down to roughly -60dB (0.001) in that time.
inline double decayCoeff(double seconds, double sampleRate) {
    double samples = std::max(1.0, seconds * sampleRate);
    return std::exp(std::log(0.001) / samples);
}

// Sample count for a short linear attack ramp. No real circuit -- CMOS,
// discrete, or otherwise -- switches on in a single sample; even the
// fastest RC charge path takes some small but nonzero time. Skipping
// straight to full amplitude on the very sample a voice triggers meant
// these hits landed as a hard instantaneous jump on top of whatever the
// rest of the engine happened to be doing at that exact sample, which is
// what was actually causing the clipping/ducking regression this ramp
// fixes -- not the voices' sustained loudness, which measured perfectly
// reasonable on its own.
inline long attackSamplesFor(double seconds, double sampleRate) {
    return std::max<long>(1, static_cast<long>(seconds * sampleRate));
}

// Kick: a sine oscillator with its own pitch swept down fast (the familiar
// "click into thump" contour of an analogue kick) plus a short amplitude
// decay.
class KickVoice {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        osc_.setSampleRate(sr);
        osc_.setWaveform(Oscillator::Waveform::Sine);
        setDecayTimes(0.09, 0.28);
        attackSamples_ = attackSamplesFor(0.004, sr);
    }
    void setTuning(double baseHz, double sweepHz) { baseHz_ = baseHz; sweepHz_ = sweepHz; }

    // Exposed so the same digital voice structure (sine + fast pitch-down
    // sweep + amplitude decay) can also provide the tom/conga stylisation
    // below -- a code-reuse decision, not a claim that the hardware circuits
    // are identical.
    void setDecayTimes(double pitchDecaySeconds, double ampDecaySeconds) {
        pitchDecay_ = decayCoeff(pitchDecaySeconds, sampleRate_);
        ampDecay_ = decayCoeff(ampDecaySeconds, sampleRate_);
    }

    // Exponent 1 keeps the original linear onset. Values below 1 make a
    // more immediate, pointed attack; values above 1 let the body bloom
    // slightly later. The progress variable is separate from ampEnv_ so a
    // curved onset never compounds its own curve from sample to sample.
    void setAttackCurve(double exponent) {
        attackCurve_ = std::clamp(exponent, 0.5, 2.0);
    }

    void trigger() {
        pitchEnv_ = 1.0;
        ampEnv_ = 0.0;
        attackProgress_ = 0.0;
        attackRemaining_ = attackSamples_;
    }

    float process() {
        if (ampEnv_ <= 0.0001 && attackRemaining_ <= 0) return 0.0f;
        osc_.setFrequency(baseHz_ + sweepHz_ * pitchEnv_);
        float out = osc_.tick() * static_cast<float>(ampEnv_);
        pitchEnv_ *= pitchDecay_;
        if (attackRemaining_ > 0) {
            attackProgress_ = std::min(1.0, attackProgress_
                + 1.0 / static_cast<double>(attackSamples_));
            ampEnv_ = std::pow(attackProgress_, attackCurve_);
            --attackRemaining_;
        } else {
            ampEnv_ *= ampDecay_;
        }
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double baseHz_ = 52.0, sweepHz_ = 90.0;
    double pitchEnv_ = 0.0, ampEnv_ = 0.0;
    double attackProgress_ = 0.0, attackCurve_ = 1.0;
    double pitchDecay_ = 0.0, ampDecay_ = 0.0;
    long attackSamples_ = 1, attackRemaining_ = 0;
    Oscillator osc_;
};

// Snare: a low sine "body" plus a noise burst shaped through the legacy-named
// SallenKeyFilter's Chamberlin bandpass output -- a generic tone-plus-filtered-
// noise digital voice inspired by analogue snares, including the TR-808.
class SnareVoice {
public:
    struct Components {
        float body = 0.0f;
        float noise = 0.0f;
    };

    void setSampleRate(double sr) {
        sampleRate_ = sr;
        tone_.setSampleRate(sr);
        tone_.setWaveform(Oscillator::Waveform::Sine);
        tone_.setFrequency(toneHz_);
        noiseFilter_.setSampleRate(sr);
        noiseFilter_.setMode(SallenKeyFilter::Mode::Bandpass);
        noiseFilter_.setCutoff(1800.0);
        noiseFilter_.setResonance(0.35);
        toneDecay_ = decayCoeff(0.12, sr);
        noiseDecay_ = decayCoeff(0.16, sr);
        attackSamples_ = attackSamplesFor(0.003, sr);
    }

    void setTone(double hz) { toneHz_ = hz; tone_.setFrequency(hz); }
    void seedNoise(unsigned seed) { noise_.seed(seed); }
    void setNoiseColor(ColoredNoise::Color color) { noise_.setColor(color); }

    // Real per-piece timbral variation, not just the tone frequency above --
    // otherwise every snare is the same circuit just retuned, which reads
    // as "always the same sound" even though the pitch moves.
    void setDecayTimes(double toneDecaySeconds, double noiseDecaySeconds) {
        toneDecay_ = decayCoeff(toneDecaySeconds, sampleRate_);
        noiseDecay_ = decayCoeff(noiseDecaySeconds, sampleRate_);
    }
    void setNoiseFilter(double cutoffHz, double resonance) {
        noiseFilter_.setCutoff(cutoffHz);
        noiseFilter_.setResonance(resonance);
    }
    void setComponentGains(float bodyGain, float noiseGain) {
        bodyGain_ = std::clamp(bodyGain, 0.5f, 0.9f);
        // Keep the lower bound below unity: the renderer's local colour
        // compensation must be able to trim a noise-forward snare instead of
        // being silently cancelled by this generic voice safety floor.
        noiseGain_ = std::clamp(noiseGain, 0.85f, 1.5f);
    }
    // Exponent 1 is the historical linear attack. Values below 1 bring the
    // snare's body/noise forward quickly; values above 1 make the onset softer
    // without changing the decay contract.
    void setAttackCurve(double exponent) {
        attackCurve_ = std::clamp(exponent, 0.5, 2.0);
    }

    void trigger() { toneEnv_ = 0.0; noiseEnv_ = 0.0; attackProgress_ = 0.0; attackRemaining_ = attackSamples_; noiseFilter_.reset(); }

    Components processComponents() {
        if (toneEnv_ <= 0.0001 && noiseEnv_ <= 0.0001 && attackRemaining_ <= 0) return {};
        Components out;
        out.body = tone_.tick() * static_cast<float>(toneEnv_) * bodyGain_;
        out.noise = noiseFilter_.process(noise_.tick()) * static_cast<float>(noiseEnv_) * noiseGain_;
        if (attackRemaining_ > 0) {
            attackProgress_ = std::min(1.0, attackProgress_
                + 1.0 / static_cast<double>(attackSamples_));
            toneEnv_ = std::pow(attackProgress_, attackCurve_);
            noiseEnv_ = toneEnv_;
            --attackRemaining_;
        } else {
            toneEnv_ *= toneDecay_;
            noiseEnv_ *= noiseDecay_;
        }
        return out;
    }

    float process() {
        const Components components = processComponents();
        return components.body + components.noise;
    }

private:
    double sampleRate_ = 44100.0;
    double toneHz_ = 190.0;
    double toneEnv_ = 0.0, noiseEnv_ = 0.0, attackProgress_ = 0.0;
    double toneDecay_ = 0.0, noiseDecay_ = 0.0;
    double attackCurve_ = 1.0;
    float bodyGain_ = 0.6f, noiseGain_ = 1.4f;
    long attackSamples_ = 1, attackRemaining_ = 0;
    Oscillator tone_;
    ColoredNoise noise_;
    SallenKeyFilter noiseFilter_;
};

// Hi-hat: several inharmonic square-ish oscillators summed and pushed
// through the Chamberlin filter's highpass output -- the standard "six
// square waves through a highpass" trick used in countless discrete/CMOS
// drum circuits (and in plenty of Lunetta builds themselves, which already
// have banks of 40106 oscillators sitting around for exactly this purpose)
// to get a metallic, inharmonic transient without a physical cymbal model.
class HiHatVoice {
public:
    static constexpr int kNumOscs = 6;

    void setSampleRate(double sr) {
        sampleRate_ = sr;
        // Ratios derived from the real TR-808 hihat's six 4584 Schmitt-
        // trigger RC oscillators (R15/C8=300k/22n, R10/C5=270k/18n,
        // R5/C3=220k/18n, R4/C1=680k/10n, R6/C5=560k/10n, R13/C9=560k/18n
        // -- read off Eric Archer's clone schematic and a cleaner scan of
        // the original service-manual sheet). Frequency is proportional to
        // 1/RC; normalizing to the slowest oscillator gives this tight
        // cluster (1.0-2.55x), noticeably closer together than the wider
        // arbitrary spread this used before -- the real circuit's six
        // partials all sit within about one and a half octaves of each
        // other, which is part of why the 808 hihat reads as a tight
        // metallic cluster rather than a bright harmonic-ish sizzle.
        static const double ratios[kNumOscs] = {1.00, 1.48, 1.53, 1.80, 2.07, 2.55};
        for (int i = 0; i < kNumOscs; ++i) {
            oscs_[static_cast<size_t>(i)].setSampleRate(sr);
            oscs_[static_cast<size_t>(i)].setWaveform(Oscillator::Waveform::Pulse);
            ratios_[static_cast<size_t>(i)] = ratios[i];
        }
        filter_.setSampleRate(sr);
        filter_.setMode(SallenKeyFilter::Mode::Highpass);
        filter_.setCutoff(7500.0);
        filter_.setResonance(0.15);
        closedDecay_ = decayCoeff(0.045, sr);
        // The real TR-808 has entirely separate closed/open hihat circuits;
        // here the open variant just gets a much longer decay (~0.35s vs.
        // 0.045s) on the same six-oscillator cluster/filter -- close enough
        // to read as "open" against the closed hit's tight, clipped decay.
        openDecay_ = decayCoeff(0.35, sr);
        decay_ = closedDecay_;
        attackSamples_ = attackSamplesFor(0.0015, sr);
    }
    void setFundamental(double hz) { fundamental_ = hz; }

    // Same idea as SnareVoice::setDecayTimes/setNoiseFilter -- real per-
    // piece variation in the circuit's own character, not just pitch.
    void setDecayTimes(double closedSeconds, double openSeconds) {
        closedDecay_ = decayCoeff(closedSeconds, sampleRate_);
        openDecay_ = decayCoeff(openSeconds, sampleRate_);
        decay_ = closedDecay_;
    }
    void setFilter(double cutoffHz, double resonance) {
        filter_.setCutoff(cutoffHz);
        filter_.setResonance(resonance);
    }

    void trigger(bool open = false) {
        env_ = 0.0;
        attackRemaining_ = attackSamples_;
        filter_.reset();
        decay_ = open ? openDecay_ : closedDecay_;
    }

    float process() {
        if (env_ <= 0.0001 && attackRemaining_ <= 0) return 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < kNumOscs; ++i) {
            size_t idx = static_cast<size_t>(i);
            oscs_[idx].setFrequency(fundamental_ * ratios_[idx]);
            sum += oscs_[idx].tick();
        }
        sum /= static_cast<float>(kNumOscs);
        float out = filter_.process(sum) * static_cast<float>(env_);
        if (attackRemaining_ > 0) {
            env_ += 1.0 / static_cast<double>(attackSamples_);
            --attackRemaining_;
        } else {
            env_ *= decay_;
        }
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double fundamental_ = 320.0;
    double env_ = 0.0, decay_ = 0.0, closedDecay_ = 0.0, openDecay_ = 0.0;
    long attackSamples_ = 1, attackRemaining_ = 0;
    std::array<Oscillator, kNumOscs> oscs_;
    std::array<double, kNumOscs> ratios_{};
    SallenKeyFilter filter_;
};

// Tom/conga: an original stylisation using the same sine-plus-pitch-sweep-
// plus-decay software structure as KickVoice, retuned and reshaped. Reusing
// the class keeps the implementation small; it does not assert that the
// TR-808 kick, tom and conga hardware circuits are literally identical.
using TomVoice = KickVoice;

// Cowbell: two square-ish oscillators at a fixed ratio (real circuit: two
// dedicated 4584 RC oscillators tuned to 1.85ms/1.25ms periods -- 540Hz
// and 800Hz, a ratio of ~1.48 -- separate from the hihat/cymbal's six-
// oscillator bank, per the schematic's own "COWBELL FREQ.1/FREQ.2" labels)
// summed and pushed through a bandpass rather than a highpass, which is
// what gives the cowbell its pitched "clank" character instead of the
// hihat's unpitched hiss -- same building blocks as HiHatVoice, different
// filter shape and far fewer oscillators.
class CowbellVoice {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        osc1_.setSampleRate(sr); osc1_.setWaveform(Oscillator::Waveform::Pulse);
        osc2_.setSampleRate(sr); osc2_.setWaveform(Oscillator::Waveform::Pulse);
        filter_.setSampleRate(sr);
        filter_.setMode(SallenKeyFilter::Mode::Bandpass);
        filter_.setResonance(0.5);
        decay_ = decayCoeff(0.30, sr);
        attackSamples_ = attackSamplesFor(0.002, sr);
    }
    void setFundamental(double hz) { fundamental_ = hz; filter_.setCutoff(hz * ratio_); }

    // Real per-piece variation beyond just the fundamental: the two-
    // oscillator ratio itself (real cowbell circuits' two RC oscillators
    // aren't identical unit to unit), decay time, and filter resonance.
    void setRatio(double ratio) { ratio_ = ratio; filter_.setCutoff(fundamental_ * ratio_); }
    void setDecay(double seconds) { decay_ = decayCoeff(seconds, sampleRate_); }
    void setResonance(double resonance) { filter_.setResonance(resonance); }

    void trigger() { env_ = 0.0; attackRemaining_ = attackSamples_; filter_.reset(); }

    float process() {
        if (env_ <= 0.0001 && attackRemaining_ <= 0) return 0.0f;
        osc1_.setFrequency(fundamental_);
        osc2_.setFrequency(fundamental_ * ratio_);
        float sum = (osc1_.tick() + osc2_.tick()) * 0.5f;
        float out = filter_.process(sum) * static_cast<float>(env_);
        if (attackRemaining_ > 0) {
            env_ += 1.0 / static_cast<double>(attackSamples_);
            --attackRemaining_;
        } else {
            env_ *= decay_;
        }
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double fundamental_ = 540.0;
    double ratio_ = 1.48;
    double env_ = 0.0, decay_ = 0.0;
    long attackSamples_ = 1, attackRemaining_ = 0;
    Oscillator osc1_, osc2_;
    SallenKeyFilter filter_;
};

// Clap: an original behavioural approximation of a familiar handclap contour:
// several quick noise bursts followed by one longer tail. It is inspired by
// the audible organisation of analogue drum voices such as the TR-808 clap,
// but does not reproduce its comparator/OTA circuit.
class ClapVoice {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        filter_.setSampleRate(sr);
        filter_.setMode(SallenKeyFilter::Mode::Bandpass);
        filter_.setCutoff(1600.0);
        filter_.setResonance(0.3);
        tailDecay_ = decayCoeff(0.20, sr);
        burstSamples_ = static_cast<long>(0.006 * sr);
        gapSamples_ = static_cast<long>(0.009 * sr);
    }

    // Real per-piece variation beyond just triggering the same fixed burst
    // pattern every time.
    void setDecay(double tailSeconds) { tailDecay_ = decayCoeff(tailSeconds, sampleRate_); }
    void setFilter(double cutoffHz, double resonance) {
        filter_.setCutoff(cutoffHz);
        filter_.setResonance(resonance);
    }
    void setBurstTiming(double burstSeconds, double gapSeconds) {
        // Keep the gesture recognisable while allowing a piece's individual
        // claps to be tighter or more spread. Bounds also prevent a zero-
        // sample phase from creating a runaway burst state.
        burstSamples_ = std::max<long>(1, static_cast<long>(
            std::clamp(burstSeconds, 0.002, 0.018) * sampleRate_));
        gapSamples_ = std::max<long>(1, static_cast<long>(
            std::clamp(gapSeconds, 0.002, 0.024) * sampleRate_));
    }
    void seedNoise(unsigned seed) { noise_.seed(seed); }
    void setNoiseColor(ColoredNoise::Color color) { noise_.setColor(color); }

    void trigger() {
        burstsRemaining_ = kNumBursts;
        phaseSamplesLeft_ = burstSamples_;
        inBurst_ = true;
        tailEnv_ = 0.0;
        filter_.reset();
    }

    float process() {
        float gate = 0.0f;
        if (burstsRemaining_ > 0) {
            gate = inBurst_ ? 1.0f : 0.0f;
            if (--phaseSamplesLeft_ <= 0) {
                if (inBurst_) {
                    inBurst_ = false;
                    phaseSamplesLeft_ = gapSamples_;
                } else {
                    --burstsRemaining_;
                    inBurst_ = true;
                    phaseSamplesLeft_ = burstSamples_;
                    if (burstsRemaining_ == 0) tailEnv_ = 1.0; // hand off to the longer tail
                }
            }
        } else if (tailEnv_ > 0.0001) {
            gate = static_cast<float>(tailEnv_);
            tailEnv_ *= tailDecay_;
        } else {
            return 0.0f;
        }
        return filter_.process(noise_.tick()) * gate;
    }

private:
    static constexpr int kNumBursts = 3;
    double sampleRate_ = 44100.0;
    long burstSamples_ = 1, gapSamples_ = 1;
    int burstsRemaining_ = 0;
    long phaseSamplesLeft_ = 0;
    bool inBurst_ = false;
    double tailEnv_ = 0.0, tailDecay_ = 0.0;
    ColoredNoise noise_;
    SallenKeyFilter filter_;
};

// Cymbal: an original eight-partial metallic voice, broader and longer than
// the hi-hat. The dense inharmonic oscillator cluster is inspired by classic
// analogue drum-machine practice, including the TR-808, but the topology and
// ratios here are a Rasgo Synth design rather than a circuit reconstruction.
class CymbalVoice {
public:
    static constexpr int kNumOscs = 8;

    void setSampleRate(double sr) {
        sampleRate_ = sr;
        static const double ratios[kNumOscs] = {1.00, 1.32, 1.48, 1.53, 1.80, 2.07, 2.55, 2.98};
        for (int i = 0; i < kNumOscs; ++i) {
            oscs_[static_cast<size_t>(i)].setSampleRate(sr);
            oscs_[static_cast<size_t>(i)].setWaveform(Oscillator::Waveform::Pulse);
            ratios_[static_cast<size_t>(i)] = ratios[i];
        }
        filter_.setSampleRate(sr);
        filter_.setMode(SallenKeyFilter::Mode::Bandpass);
        filter_.setCutoff(4200.0);
        filter_.setResonance(0.1);
        decay_ = decayCoeff(1.4, sr);
        attackSamples_ = attackSamplesFor(0.002, sr);
    }
    void setFundamental(double hz) { fundamental_ = hz; }

    // Real per-piece variation: decay length (a crash's ring time varies
    // with the physical cymbal/circuit) and filter shape, not just pitch.
    void setDecay(double seconds) { decay_ = decayCoeff(seconds, sampleRate_); }
    void setFilter(double cutoffHz, double resonance) {
        filter_.setCutoff(cutoffHz);
        filter_.setResonance(resonance);
    }

    void trigger() { env_ = 0.0; attackRemaining_ = attackSamples_; filter_.reset(); }

    float process() {
        if (env_ <= 0.0001 && attackRemaining_ <= 0) return 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < kNumOscs; ++i) {
            size_t idx = static_cast<size_t>(i);
            oscs_[idx].setFrequency(fundamental_ * ratios_[idx]);
            sum += oscs_[idx].tick();
        }
        sum /= static_cast<float>(kNumOscs);
        float out = filter_.process(sum) * static_cast<float>(env_);
        if (attackRemaining_ > 0) {
            env_ += 1.0 / static_cast<double>(attackSamples_);
            --attackRemaining_;
        } else {
            env_ *= decay_;
        }
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double fundamental_ = 320.0;
    double env_ = 0.0, decay_ = 0.0;
    long attackSamples_ = 1, attackRemaining_ = 0;
    std::array<Oscillator, kNumOscs> oscs_;
    std::array<double, kNumOscs> ratios_{};
    SallenKeyFilter filter_;
};

// Maracas: a generic short, bright filtered-noise burst (no multi-hit pattern
// like the clap), placed higher and airier than the clap. It is an original
// shaker-like voice, not a reconstruction of the TR-808 maracas circuit.
class MaracasVoice {
public:
    void setSampleRate(double sr) {
        sampleRate_ = sr;
        filter_.setSampleRate(sr);
        filter_.setMode(SallenKeyFilter::Mode::Highpass);
        filter_.setCutoff(4000.0);
        filter_.setResonance(0.1);
        decay_ = decayCoeff(0.06, sr);
        attackSamples_ = attackSamplesFor(0.001, sr);
    }

    // Real per-piece variation: decay/filter, not a fixed shaker sound
    // every time.
    void setDecay(double seconds) { decay_ = decayCoeff(seconds, sampleRate_); }
    void setFilter(double cutoffHz, double resonance) {
        filter_.setCutoff(cutoffHz);
        filter_.setResonance(resonance);
    }
    void seedNoise(unsigned seed) { noise_.seed(seed); }
    void setNoiseColor(ColoredNoise::Color color) { noise_.setColor(color); }

    void trigger() { env_ = 0.0; attackRemaining_ = attackSamples_; filter_.reset(); }

    float process() {
        if (env_ <= 0.0001 && attackRemaining_ <= 0) return 0.0f;
        float out = filter_.process(noise_.tick()) * static_cast<float>(env_);
        if (attackRemaining_ > 0) {
            env_ += 1.0 / static_cast<double>(attackSamples_);
            --attackRemaining_;
        } else {
            env_ *= decay_;
        }
        return out;
    }

private:
    double sampleRate_ = 44100.0;
    double env_ = 0.0, decay_ = 0.0;
    long attackSamples_ = 1, attackRemaining_ = 0;
    ColoredNoise noise_;
    SallenKeyFilter filter_;
};

} // namespace rasgo
