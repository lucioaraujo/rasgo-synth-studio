#pragma once
#include <algorithm>
#include <random>

namespace rasgo {

// Ports the "grainy noise" idea from Ray Wilson's Noise Cornucopia
// (MFOS): rather than white noise's smooth, continuous hiss, a comparator
// with hysteresis watching that same noise source only flips its output
// when the noise crosses a threshold -- turning a continuous signal into
// an irregular train of discrete ticks/pulses. The real circuit's
// "graininess" pots (more ticks / less ticks) are the hysteresis width:
// narrow hysteresis means the noise crosses back and forth constantly
// (dense, crackly), wide hysteresis means it takes a bigger swing to flip
// (sparse, poppy). Genuinely different in character from this engine's
// existing smooth/filtered noise uses (snare rattle, clap, maracas) --
// bursty and irregular rather than continuous.
class GrainyNoise {
public:
    void seed(unsigned s) { rng_.seed(s); }

    // 0..1: higher = denser/crackier (narrower hysteresis), lower =
    // sparser/poppier (wider hysteresis).
    void setGraininess(float g) {
        hysteresis_ = 0.65f - std::clamp(g, 0.0f, 1.0f) * 0.6f; // 0.65 down to 0.05
    }

    float tick() {
        float noise = unit_(rng_) * 2.0f - 1.0f;
        if (state_) {
            if (noise < -hysteresis_) state_ = false;
        } else {
            if (noise > hysteresis_) state_ = true;
        }
        return state_ ? 1.0f : -1.0f;
    }

private:
    float hysteresis_ = 0.3f;
    bool state_ = false;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> unit_{0.0f, 1.0f};
};

} // namespace rasgo
