#pragma once
#include <cmath>
#include <algorithm>

namespace rasgo {

// A compact 4-pole transistor-ladder-style lowpass filter (Stilson/Smith
// topology, as widely reproduced in the DSP community): the incoming signal
// is scaled once by a term derived from the normalized cutoff, then passed
// through four cascaded leaky-integrator stages with a small feedforward
// term between them, with resonance implemented as feedback from the last
// stage back into the input. Generic, independently written implementation
// of a well-documented public technique -- not a port of SuperCollider's
// MoogFF (that source wasn't inspected), so there's no licensing entanglement.
class MoogFilter {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; }

    // cutoff in Hz, resonance roughly 0..4 (self-oscillates near the top end)
    void setCutoff(double cutoffHz) { cutoffHz_ = cutoffHz; }
    void setResonance(double resonance) { resonance_ = resonance; }

    float process(float input) {
        double nyquist = sampleRate_ * 0.5;
        double fc = std::clamp(cutoffHz_ / nyquist, 0.0005, 0.85);
        double f = fc * 1.16;
        double fb = resonance_ * (1.0 - 0.15 * f * f);

        double in = static_cast<double>(input);
        in -= out4_ * fb;
        in *= 0.35013 * (f * f) * (f * f);

        out1_ = in + 0.3 * in1_ + (1.0 - f) * out1_;
        in1_ = in;
        out2_ = out1_ + 0.3 * in2_ + (1.0 - f) * out2_;
        in2_ = out1_;
        out3_ = out2_ + 0.3 * in3_ + (1.0 - f) * out3_;
        in3_ = out2_;
        out4_ = out3_ + 0.3 * in4_ + (1.0 - f) * out4_;
        in4_ = out3_;

        // Makeup gain: this filter's passband level drops noticeably at low
        // cutoff settings (the f^4 input scaling is part of how it gets its
        // characteristic ladder rolloff/resonance behavior), so compensate
        // roughly by ear rather than leaving quiet patches out of the box.
        double makeup = 1.0 + 6.0 * (1.0 - fc);
        return static_cast<float>(out4_ * makeup);
    }

private:
    double sampleRate_ = 44100.0;
    double cutoffHz_ = 2000.0;
    double resonance_ = 0.0;
    double in1_ = 0.0, in2_ = 0.0, in3_ = 0.0, in4_ = 0.0;
    double out1_ = 0.0, out2_ = 0.0, out3_ = 0.0, out4_ = 0.0;
};

} // namespace rasgo
