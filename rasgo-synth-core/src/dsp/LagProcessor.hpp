#pragma once
#include <cmath>
#include <algorithm>

namespace rasgo {

// Another circuit from the same schematic archive (resultat_fusion.pdf)
// that Buchla Associates apparently sketched but never built -- a "Dual
// Control Voltage Integrator" (Model 155), with "Need circuit" handwritten
// right on the page. Functionally it's a lag/slew processor: it turns a
// control voltage's instant jumps into glides, independently timed for
// rising vs. falling changes. Rasgo's stepped modulation source
// (ClockedRandomSource, modeling Buchla's Random Voltage Source/Model 208)
// jumps instantly on every clock tick; running its output through one of
// these first turns that hard step into a portamento-like glide -- exactly
// the kind of secondary processing the two circuits were clearly meant to
// be chained with, on the same page, in the same never-built batch.
class LagProcessor {
public:
    void setSampleRate(double sr) { sampleRate_ = sr; updateCoeffs(); }
    void setTimes(double riseMs, double fallMs) { riseMs_ = riseMs; fallMs_ = fallMs; updateCoeffs(); }
    void setInitial(double v) { value_ = v; }

    float process(float target) {
        double coeff = target > value_ ? riseCoeff_ : fallCoeff_;
        value_ += (target - value_) * coeff;
        return static_cast<float>(value_);
    }

private:
    void updateCoeffs() {
        riseCoeff_ = coeffFor(riseMs_);
        fallCoeff_ = coeffFor(fallMs_);
    }
    double coeffFor(double ms) const {
        double samples = std::max(1.0, (ms / 1000.0) * sampleRate_);
        return 1.0 - std::exp(-1.0 / samples);
    }

    double sampleRate_ = 44100.0;
    double riseMs_ = 40.0, fallMs_ = 40.0;
    double riseCoeff_ = 0.05, fallCoeff_ = 0.05;
    double value_ = 0.0;
};

} // namespace rasgo
