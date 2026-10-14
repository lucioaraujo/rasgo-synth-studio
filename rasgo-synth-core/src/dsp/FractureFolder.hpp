#pragma once
#include <random>

namespace rasgo {

// West Coast wavefolding (Buchla 259/Serge Wave Multiplier lineage, the
// same family of algorithm Mutable Instruments' Plaits/Tides run digitally)
// -- but pushed past the point where the fold stays a clean, periodic
// reflection. An ordinary folder always reflects at the exact same
// threshold, so driving it harder just adds more, still perfectly
// periodic, harmonics. Past fractureAmount > 0, this folder jitters its own
// reflection point a little on every single crossing, so the fold stops
// being a predictable wave-shaping curve and starts behaving like the wave
// is tearing against an uneven edge -- non-periodic, a little different
// every cycle, on purpose. fractureAmount = 0 is a perfectly ordinary,
// clean folder; nothing here changes unless a piece's seed asks for it.
class FractureFolder {
public:
    void seed(unsigned s) { rng_.seed(s); }
    void setFoldAmount(double drive) { foldAmount_ = drive; }
    void setFractureAmount(double amount) { fractureAmount_ = amount; }

    // x expected roughly -1..1; output always bounded to the same range
    // regardless of drive, so this can sit in a signal chain that already
    // assumes a folder can't be the thing that introduces clipping.
    double process(double x) {
        double y = x * (1.0 + foldAmount_ * 5.0);
        for (int i = 0; i < kMaxFolds; ++i) {
            double hi = 1.0 - (fractureAmount_ > 0.0 ? fractureAmount_ * 0.5 * unit_(rng_) : 0.0);
            double lo = -1.0 + (fractureAmount_ > 0.0 ? fractureAmount_ * 0.5 * unit_(rng_) : 0.0);
            if (y > hi) { y = 2.0 * hi - y; continue; }
            if (y < lo) { y = 2.0 * lo - y; continue; }
            break;
        }
        if (y > 1.0) y = 1.0;
        if (y < -1.0) y = -1.0;
        return y;
    }

private:
    static constexpr int kMaxFolds = 16;
    double foldAmount_ = 0.0;
    double fractureAmount_ = 0.0;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> unit_{0.0, 1.0};
};

} // namespace rasgo
