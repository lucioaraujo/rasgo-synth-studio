#pragma once
#include <random>
#include <cstdint>

namespace rasgo {

// Two rupture techniques fused into one original stage. First, the amount and
// symmetry of full-wave rectification is a moving target, so the fundamental
// hollows out and the wave mirrors against itself instead of settling into one
// fixed timbre. Nonlinearcircuits' correctly named Difference Rectifier /
// Neuron was part of the wider research context, but this code is not a port or
// circuit model of that product. Second, Xaoc Devices'
// Leibniz system (Drezno/Lipsk): the already-rectified sample is quantized
// to an 8-bit code and a handful of its bits are deliberately flipped
// before decoding back to audio -- the same "corrupted data" character
// associated with performer-controlled binary manipulation, here driven by
// the seed instead. Rectify first, corrupt second: corruption riding on
// top of an already-hollowed harmonic series reads as a controlled digital
// wound, not just noise on top of a clean tone.
class LeibnizRectifier {
public:
    void seed(unsigned s) { rng_.seed(s); }
    void setRectifyAmount(double amount) { rectifyAmount_ = amount; }  // 0 = dry, 1 = full rectify
    void setAsymmetry(double asym) { asymmetry_ = asym; }              // -1..1, biases which half folds harder
    void setBitCorruption(double amount) { corruptAmount_ = amount; }  // 0 = clean 8-bit code, ~1 = up to 4 bits flipped/sample

    double process(double x) {
        double posBias = 1.0 + asymmetry_ * 0.5;
        double negBias = 1.0 - asymmetry_ * 0.5;
        double rectified = x >= 0.0 ? x * posBias : -x * negBias;
        double wet = x * (1.0 - rectifyAmount_) + rectified * rectifyAmount_;
        if (wet > 1.0) wet = 1.0;
        if (wet < -1.0) wet = -1.0;
        if (corruptAmount_ <= 0.0) return wet;

        auto code = static_cast<uint8_t>((wet * 0.5 + 0.5) * 255.0);
        int flips = static_cast<int>(corruptAmount_ * 3.0 + unit_(rng_));
        for (int i = 0; i < flips; ++i) {
            int bit = static_cast<int>(unit_(rng_) * 8.0);
            code ^= static_cast<uint8_t>(1u << bit);
        }
        double corrupted = (static_cast<double>(code) / 255.0) * 2.0 - 1.0;
        return wet * (1.0 - corruptAmount_) + corrupted * corruptAmount_;
    }

private:
    double rectifyAmount_ = 0.0;
    double asymmetry_ = 0.0;
    double corruptAmount_ = 0.0;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> unit_{0.0, 1.0};
};

} // namespace rasgo
