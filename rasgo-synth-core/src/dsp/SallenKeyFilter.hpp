#pragma once
#include <cmath>
#include <algorithm>

namespace rasgo {

// A second filter colour alongside MoogFilter's 4-pole transistor-ladder
// lowpass: a 2-pole state-variable filter with simultaneous lowpass,
// highpass and bandpass outputs. Despite the legacy filename/class name, this
// is Hal Chamberlin's state-variable filter algorithm (see "Musical
// Applications of Microprocessors"), not an implementation of the electronic
// Sallen-Key topology. The name survived from an early research note about
// vactrol-controlled filters in Lunetta-style CMOS builds and should not be
// read as a circuit-provenance claim.
//
// Two poles read as more open and "airier" than the 4-pole ladder's steeper
// rolloff -- a genuinely different timbral character, not a redundant
// second lowpass.
class SallenKeyFilter {
public:
    enum class Mode { Lowpass, Highpass, Bandpass };

    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setCutoff(double hz) { cutoffHz_ = hz; }
    // Roughly 0..1; higher values narrow the resonant peak and can self-
    // oscillate near the top, same shape as MoogFilter's own resonance knob.
    void setResonance(double r) { resonance_ = std::clamp(r, 0.0, 1.0); }
    void setMode(Mode m) { mode_ = m; }

    // Clears the filter's own internal state. A state-variable filter's
    // two integrators keep whatever energy was in them from the last time
    // they were fed -- fine for a filter that's continuously in the
    // signal path, but a drum voice that goes fully silent between hits
    // and then resumes feeding it should start each hit from a clean
    // state, not from however things happened to be left ringing (however
    // faintly) after the previous one.
    void reset() { low_ = 0.0; band_ = 0.0; }

    float process(float input) {
        double nyquist = sampleRate_ * 0.5;
        // This simple (non-oversampled, non-trapezoidal) state-variable
        // topology is only unconditionally stable for f well below 2 --
        // clamped more conservatively here than a first pass did, after
        // testing turned up genuine numerical divergence (output running
        // away to infinity, not just "loud") at higher cutoff/resonance
        // combinations. 0.24 keeps f comfortably under 1.5 across the
        // whole resonance range this class exposes.
        double fc = std::clamp(cutoffHz_ / nyquist, 0.001, 0.24);
        double f = 2.0 * std::sin(3.141592653589793 * fc);

        double qFactor = 0.5 + resonance_ * 15.0;
        double damping = 1.0 / qFactor;

        low_ += f * band_;
        double high = static_cast<double>(input) - low_ - damping * band_;
        band_ += f * high;

        // Belt-and-braces: if some combination of parameters this class
        // hasn't been tested against still manages to diverge, don't feed
        // an infinity/NaN into the rest of the signal chain -- reset and
        // carry on, the same defensive move HenonMap/QuadMap already make
        // elsewhere in this codebase when their own recurrences blow up.
        if (!std::isfinite(low_) || !std::isfinite(band_) ||
            std::fabs(low_) > 50.0 || std::fabs(band_) > 50.0) {
            low_ = 0.0; band_ = 0.0;
        }

        switch (mode_) {
            case Mode::Lowpass:  return static_cast<float>(low_);
            case Mode::Highpass: return static_cast<float>(high);
            case Mode::Bandpass: return static_cast<float>(band_);
        }
        return static_cast<float>(low_);
    }

private:
    double sampleRate_ = 44100.0;
    double cutoffHz_ = 1200.0;
    double resonance_ = 0.3;
    Mode mode_ = Mode::Lowpass;
    double low_ = 0.0, band_ = 0.0;
};

} // namespace rasgo
