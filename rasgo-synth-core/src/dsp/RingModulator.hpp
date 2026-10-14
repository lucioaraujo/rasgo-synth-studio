#pragma once
#include <algorithm>

namespace rasgo {

// Ring modulation: multiply two signals together instead of adding them.
// Where a mixer combines the spectra of its inputs, multiplication produces
// new sum-and-difference frequencies that weren't in either input -- the
// classic "bell/clangorous/inharmonic" character, and mathematically the
// most literal expression yet of "flows that cross and generate new
// modulations." Modeled loosely on the CBS Musical Instruments "Dual Ring
// Modulator No. 111" schematic (Fullerton, CA, 1970): that circuit used
// matched transistor pairs as a switching bridge instead of the more common
// diode ring, which in practice means the carrier isn't perfectly
// suppressed -- some of it leaks through alongside the sum/difference
// products. `balance` models that: 1.0 is a mathematically pure multiply,
// lower values let some raw carrier bleed into the output, same as an
// imperfectly matched real bridge would.
class RingModulator {
public:
    void setBalance(double balance) { balance_ = std::clamp(balance, 0.0, 1.0); }

    float process(float signal, float carrier) {
        float pureProduct = signal * carrier;
        float carrierLeak = carrier * static_cast<float>(1.0 - balance_) * 0.35f;
        return pureProduct + carrierLeak;
    }

private:
    double balance_ = 1.0;
};

} // namespace rasgo
