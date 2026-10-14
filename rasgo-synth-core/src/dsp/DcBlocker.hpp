#pragma once
#include <cmath>

namespace rasgo {

// A one-pole DC blocker (y[n] = x[n] - x[n-1] + R*y[n-1]), the standard
// cheap fix for DC bias before a final output stage. Worth having here
// specifically because DiodeShaper's diode-mix voice is asymmetric by
// design (that's the point of a diode mixer -- real diode mixing clips
// unevenly), which is exactly the kind of processing that can leave a
// measurable DC offset in the signal that nothing downstream removes on
// its own.
class DcBlocker {
public:
    void setSampleRate(double sr) {
        constexpr double cutoffHz = 20.0;
        constexpr double twoPi = 6.283185307179586;
        r_ = std::exp(-twoPi * cutoffHz / sr);
    }

    float process(float x) {
        float y = x - lastX_ + static_cast<float>(r_) * lastY_;
        lastX_ = x;
        lastY_ = y;
        return y;
    }

private:
    double r_ = 0.997;
    float lastX_ = 0.0f, lastY_ = 0.0f;
};

} // namespace rasgo
