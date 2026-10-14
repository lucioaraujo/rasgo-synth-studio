#pragma once
#include <cmath>
#include <algorithm>

namespace rasgo {

// Dual VCA, Polivoks-architecture inspired (the real module's rare Russian
// differential-pair transistors are these days replaced with an AS3330 in
// modern clones -- same topology, different silicon). A bipolar
// differential pair used as a voltage-controlled amplifier does not behave
// like an idealized multiplier: its large-signal transfer function is the
// textbook translinear-circuit equation for a differential pair,
// I_out ~= I_tail * tanh(V_in / (2 * V_thermal)) -- standard electronics
// (Gilbert, "Translinear Circuits"; any BJT differential-amplifier
// derivation), not copied from any specific schematic or open-source code.
// The audible consequence: quiet passages stay close to a clean linear
// gain (tanh(x) ~= x for small x), but loud transients compress/saturate
// against the pair's own thermal-voltage limit instead of scaling forever
// -- the real character a differential-pair VCA has that a clean
// multiplier-based one (SSM2164, that/etc.) doesn't. "Dual" here means
// both channels share the exact same transfer curve, the same way two
// halves of one physical chip would.
class PolivoksVCA {
public:
    // driveAmount sets how hard a full-scale (+-1) input pushes into the
    // tanh curve's nonlinear region -- higher values mean more audible
    // compression/grit on loud passages, closer to the real chip's own
    // relatively low thermal-voltage headroom.
    void configure(float driveAmount) {
        drive_ = std::max(0.1f, driveAmount);
        // Makeup gain keeps a quiet, low-gain signal close to unity so this
        // reads as "a VCA with character," not "a VCA that's just quieter."
        makeup_ = 1.0f / std::tanh(drive_ * 0.05f) * 0.05f;
    }

    // in: audio sample, roughly [-1, 1]. gain: control voltage
    // equivalent, 0..1 (0 = fully closed, 1 = fully open).
    float process(float in, float gain) const {
        float g = std::clamp(gain, 0.0f, 1.0f);
        return std::tanh(in * g * drive_) * makeup_;
    }

private:
    float drive_ = 1.0f;
    float makeup_ = 1.0f;
};

// Two independent audio paths through the identical transfer curve above --
// the actual "dual" of the real module (one chip, two matched channels),
// not two independently-configured VCAs.
class DualPolivoksVCA {
public:
    void configure(float driveAmount) { vca_.configure(driveAmount); }
    float processA(float in, float gain) const { return vca_.process(in, gain); }
    float processB(float in, float gain) const { return vca_.process(in, gain); }

private:
    PolivoksVCA vca_;
};

} // namespace rasgo
