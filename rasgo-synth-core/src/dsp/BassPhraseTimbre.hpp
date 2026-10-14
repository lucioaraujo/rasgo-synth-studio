#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <ostream>

namespace rasgo {

// Original Studio implementation: finite additive spectra, not a port of
// Cardinal/Rack. Inputs must be the existing unit sine at f and sine at 2f.
// sin(3x) and sin(5x) use trigonometric identities, not an unbounded shaper.
// No new pitch generator, RNG consumption, heap storage or audio feedback.
class BassPhraseTimbre {
public:
    BassPhraseTimbre(unsigned seed, bool enabled, double sampleRate) noexcept
        : seed_(seed), enabled_(enabled) {
        const double rate = std::isfinite(sampleRate) && sampleRate >= 8000.0
            ? sampleRate : 48000.0;
        slew_ = 1.0 - std::exp(-1.0 / (rate * .020));
        transientDecay_ = std::exp(-1.0 / (rate * .080));
    }

    // Four-bar families retain their identity across notes. Only spectral
    // articulation restarts on a note; phase and the amplitude envelope do not.
    void noteOn(unsigned bar, double pressure, bool accent, bool dead) noexcept {
        if (!enabled_) return;
        profile_ = hash(seed_ ^ ((bar / 4u) * 1031u + 719u)) % 3u;
        pressure_ = finiteClamp(pressure, 0.0, 1.0, .5);
        transient_ = dead ? .25 : accent ? 1.0 : .65;
        ++notes_[profile_];
    }

    float process(float fundamental, float second, double material,
                  double articulation) noexcept {
        // This class' bypass is neutral. The renderer keeps the complete
        // legacy two-oscillator sum outside it for the disabled branch.
        if (!enabled_) return fundamental;
        if (!std::isfinite(fundamental) || !std::isfinite(second)) return 0.0f;
        const double s = std::clamp(static_cast<double>(fundamental), -1.0, 1.0);
        const double h2 = std::clamp(static_cast<double>(second), -1.0, 1.0);
        const double s2 = s * s;
        const double h3 = s * (3.0 - 4.0 * s2);
        const double h5 = s * (5.0 + s2 * (-20.0 + 16.0 * s2));
        // Round, hollow and edged are spectra, not claims of acoustic replicas.
        static constexpr double profiles[3][3] {
            {.20, .025, .005}, {.045, .42, .16}, {.30, .20, .10}
        };
        const double color = finiteClamp(material, -.8, .8, 0.0);
        const double touch = finiteClamp(articulation, .1, .95, .5);
        const double opening = .65 + (color + .8) * .48
            + transient_ * (.15 + pressure_ * .25) * (.5 + touch * .5);
        for (unsigned i = 0; i < weights_.size(); ++i) {
            const double target = profiles[profile_][i] * opening;
            weights_[i] += (target - weights_[i]) * slew_;
        }
        transient_ *= transientDecay_;
        ++frames_;
        // Absolute partial-weight normalization bounds the output and leaves
        // the fundamental at least as strong as any individual added partial.
        const double denominator = 1.0 + weights_[0] + weights_[1] + weights_[2];
        return static_cast<float>((s + weights_[0] * h2 + weights_[1] * h3
                                  + weights_[2] * h5) / denominator);
    }

    bool enabled() const noexcept { return enabled_; }
    unsigned profile() const noexcept { return profile_; }
    const std::array<std::uint64_t, 3>& notes() const noexcept { return notes_; }
    std::uint64_t frames() const noexcept { return frames_; }
    const std::array<double, 3>& weights() const noexcept { return weights_; }

    void writeJson(std::ostream& out) const {
        out << "\"bassPhraseTimbre\":" << (enabled_ ? "true" : "false")
            << ",\"bassPhraseTimbreVersion\":1,\"bassPhraseTimbreNotes\":["
            << notes_[0] << ',' << notes_[1] << ',' << notes_[2]
            << "],\"bassPhraseTimbreFrames\":" << frames_
            << ",\"bassPhraseTimbrePartials\":[1,2,3,5],";
    }
    void writeScore(std::ostream& out) const {
        if (!enabled_) return;
        out << "\nBAIXO CORE -- espectro por frase, versao experimental 1\n"
            << "  Notas por perfil round/hollow/edged: " << notes_[0] << '/'
            << notes_[1] << '/' << notes_[2] << "; amostras processadas: " << frames_
            << ". Contagens nao certificam presenca perceptiva.\n";
    }

private:
    static unsigned hash(unsigned x) noexcept {
        x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15;
        x *= 0x846ca68bu; return x ^ (x >> 16);
    }
    static double finiteClamp(double x, double low, double high,
                              double fallback) noexcept {
        return std::isfinite(x) ? std::clamp(x, low, high) : fallback;
    }
    unsigned seed_, profile_ = 0;
    bool enabled_;
    double slew_ = 0.0, transientDecay_ = 0.0;
    double transient_ = 0.0, pressure_ = .5;
    std::array<double, 3> weights_{};
    std::array<std::uint64_t, 3> notes_{};
    std::uint64_t frames_ = 0;
};

} // namespace rasgo
