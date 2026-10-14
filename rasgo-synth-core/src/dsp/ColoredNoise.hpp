#pragma once
#include <algorithm>
#include <random>

namespace rasgo {

// Frap Tools' Sapèl offers four dedicated noise colors (white/pink/red-
// brownian/blue) rather than just white -- a real gap this engine had
// everywhere it used noise (snare rattle, clap, maracas, granular texture
// all draw from plain white noise). Pink uses the well-known Paul Kellet
// "economy" three-stage approximation; red/brownian integrates pink
// further (heavier in the sub end, good for slow thermal-feeling
// modulation); blue differentiates white (emphasis rising with
// frequency, the inverse of pink).
class ColoredNoise {
public:
    enum class Color { White, Pink, Red, Blue };

    void setColor(Color c) { color_ = c; }
    void seed(unsigned s) { rng_.seed(s); }

    float tick() {
        float white = unit_(rng_) * 2.0f - 1.0f;

        b0_ = 0.99886f * b0_ + white * 0.0555179f;
        b1_ = 0.99332f * b1_ + white * 0.0750759f;
        b2_ = 0.96900f * b2_ + white * 0.1538520f;
        float pink = (b0_ + b1_ + b2_ + white * 0.5362f) * 0.11f;

        switch (color_) {
            case Color::White:
                return white;
            case Color::Pink:
                return pink;
            case Color::Red: {
                redState_ += pink * 0.05f;
                redState_ = std::clamp(redState_, -1.0f, 1.0f);
                redState_ *= 0.999f; // gentle leak so it doesn't wander to a DC rail
                return redState_;
            }
            case Color::Blue: {
                float blue = (white - lastWhite_) * 0.6f;
                lastWhite_ = white;
                return std::clamp(blue, -1.0f, 1.0f);
            }
        }
        return white;
    }

private:
    Color color_ = Color::White;
    float b0_ = 0.0f, b1_ = 0.0f, b2_ = 0.0f;
    float redState_ = 0.0f;
    float lastWhite_ = 0.0f;
    std::mt19937 rng_;
    std::uniform_real_distribution<float> unit_{0.0f, 1.0f};
};

} // namespace rasgo
