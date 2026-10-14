#pragma once
#include <cmath>
#include <algorithm>

namespace rasgo {

// A third filter colour, alongside MoogFilter's 4-pole ladder and
// SallenKeyFilter's clean 2-pole state-variable design -- both of those
// self-oscillate (if at all) into a clean, textbook sine at high
// resonance, a linear resonator ringing at its own pole frequency. The
// real Polivoks (Soviet analogue synth, built around unstable, non-
// compensated op-amps rather than precision parts) instead self-oscillates
// harshly -- the instability itself distorts the resonant energy rather
// than letting it settle into a pure tone. Modeled here (per this
// project's "port the idea, not the transistor count" approach) as the
// same Chamberlin-style state-variable core SallenKeyFilter already uses,
// but with two real differences: damping goes slightly NEGATIVE as
// resonance approaches 1 (genuine linear instability -- loop gain > 1,
// so the oscillation actually grows instead of just ringing longer), and
// the resonant feedback path runs through a tanh saturation that catches
// that growth and bounds it into a real, sustained, nonlinearly-clipped
// limit cycle rather than letting it either decay to silence (like a
// normal damped resonance) or run away to infinity.
//
// Verified by an isolated test before wiring in: stable (no NaN/Inf/
// blowup) across a cutoff x resonance sweep, and genuinely self-
// sustaining at high resonance -- fed silence after a brief kick, the
// output settles into a real non-decaying, non-exploding limit cycle
// rather than ringing out to zero the way a normal damped resonator
// would.
class PolivoksFilter {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }
    void setCutoff(double hz) { cutoffHz_ = hz; }
    // Roughly 0..1, same shape as MoogFilter/SallenKeyFilter's own knob --
    // unlike those, resonance here pushes damping slightly negative near
    // the top instead of just narrowing a Q peak, so self-oscillation is
    // a real, deliberate outcome of this filter's design, not a marginal
    // edge case.
    void setResonance(double r) { resonance_ = std::clamp(r, 0.0, 1.0); }

    // Same rationale as SallenKeyFilter::reset() -- a drum voice that
    // goes fully silent between hits should start each hit from a clean
    // state, not from however the unstable resonance happened to leave
    // things ringing after the last one.
    void reset() { low_ = 0.0; band_ = 0.0; }

    float process(float input) {
        double nyquist = sampleRate_ * 0.5;
        // Same conservative stability clamp as SallenKeyFilter, for the
        // same reason -- this simple topology is only unconditionally
        // stable for f well below 2.
        double fc = std::clamp(cutoffHz_ / nyquist, 0.001, 0.24);
        double f = 2.0 * std::sin(3.141592653589793 * fc);

        double damping = 0.5 - resonance_ * 0.52;

        low_ += f * band_;
        double bandSat = std::tanh(band_ * (1.0 + resonance_ * 2.5));
        double high = static_cast<double>(input) - low_ - damping * bandSat;
        band_ += f * high;

        // Belt-and-braces, same defensive reset SallenKeyFilter already
        // uses -- deliberately courting instability above makes this
        // guard more load-bearing here than there, not less necessary.
        if (!std::isfinite(low_) || !std::isfinite(band_) ||
            std::fabs(low_) > 50.0 || std::fabs(band_) > 50.0) {
            low_ = 0.0; band_ = 0.0;
        }

        // Output-stage soft limit: keeps the self-oscillation's settled
        // amplitude (which depends on cutoff/resonance and would
        // otherwise vary a lot) in the same rough -1..1 range every other
        // waveshaper/filter in this engine already targets.
        return static_cast<float>(std::tanh(low_ * 0.35));
    }

private:
    double sampleRate_ = 44100.0;
    double cutoffHz_ = 1200.0;
    double resonance_ = 0.3;
    double low_ = 0.0, band_ = 0.0;
};

} // namespace rasgo
