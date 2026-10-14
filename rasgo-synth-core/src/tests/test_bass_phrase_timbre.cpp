#include "../dsp/BassPhraseTimbre.hpp"

#include <array>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif

namespace {
constexpr double pi = 3.14159265358979323846;
constexpr double step = 2.0 * pi / 512.0; // 93.75 Hz at 48 kHz; coherent bins.
float fundamental(unsigned frame) { return static_cast<float>(std::sin(frame * step)); }
float second(unsigned frame) { return static_cast<float>(std::sin(frame * step * 2.0)); }
}

int main() {
    using rasgo::BassPhraseTimbre;
    static_assert(sizeof(BassPhraseTimbre) < 256, "Bounded per-render state");
    std::array<bool, 3> seen{};
    std::array<double, 3> thirdAmplitudes{};
    for (unsigned seed = 0; seed < 32; ++seed) {
        BassPhraseTimbre a(seed, true, 48000), b(seed, true, 48000);
        BassPhraseTimbre off(seed, false, 48000);
        unsigned previousProfile = 0;
        for (unsigned bar = 0; bar < 12; ++bar) {
            a.noteOn(bar, .6, bar % 2 == 0, false);
            b.noteOn(bar, .6, bar % 2 == 0, false);
            off.noteOn(bar, .6, true, false);
            assert(a.profile() == b.profile());
            // Identity persists inside the four-bar phrase, not each note.
            if (bar % 4 != 0) assert(a.profile() == previousProfile);
            previousProfile = a.profile();
            for (unsigned frame = 0; frame < 512; ++frame) {
                const float s = fundamental(frame), h2 = second(frame);
                const float x = a.process(s, h2, -.8 + bar * .13, .65);
                assert(x == b.process(s, h2, -.8 + bar * .13, .65));
                assert(std::isfinite(x) && std::abs(x) <= 1.000001f);
                assert(off.process(s, h2, .8, .95) == s);
            }
        }
        assert(a.frames() == 12 * 512 && off.frames() == 0);
        assert((off.notes() == std::array<std::uint64_t, 3>{}));

        BassPhraseTimbre spectral(seed, true, 48000);
        spectral.noteOn(0, .6, false, false);
        const unsigned profile = spectral.profile();
        if (!seen[profile]) {
            seen[profile] = true;
            // Let note transient and coefficient slew settle, then measure
            // relative spectral structure; this is not a listening judgment.
            for (unsigned frame = 0; frame < 48000; ++frame)
                spectral.process(fundamental(frame), second(frame), 0, .5);
            double amplitude3 = 0.0, amplitude1 = 0.0, amplitude7 = 0.0;
            constexpr unsigned count = 8192;
            for (unsigned frame = 48000; frame < 48000 + count; ++frame) {
                const double x = spectral.process(fundamental(frame), second(frame), 0, .5);
                amplitude1 += x * std::sin(frame * step);
                amplitude3 += x * std::sin(frame * step * 3.0);
                amplitude7 += x * std::sin(frame * step * 7.0);
            }
            thirdAmplitudes[profile] = amplitude3 * 2.0 / count;
            assert(amplitude1 * 2.0 / count > .4);
            assert(std::abs(amplitude7 * 2.0 / count) < 1e-5);
        }
    }
    assert(seen[0] && seen[1] && seen[2]);
    assert(thirdAmplitudes[1] > thirdAmplitudes[0] * 5.0);
    assert(thirdAmplitudes[2] > thirdAmplitudes[0] * 3.0);

    // Rate/controls are sanitized; invalid oscillator samples do not poison
    // state. A subsequent valid frame must recover.
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    BassPhraseTimbre invalid(7, true, nan);
    invalid.noteOn(0, nan, true, false);
    assert(invalid.process(static_cast<float>(nan), 0, nan, nan) == 0);
    assert(std::isfinite(invalid.process(.5f, -.3f, nan, nan)));

    BassPhraseTimbre transition(11, true, 48000);
    transition.noteOn(0, .5, false, false);
    for (unsigned i = 0; i < 48000; ++i) transition.process(.5f, .25f, 0, .5);
    const auto before = transition.weights();
    transition.noteOn(4, 1, true, false);
    transition.process(.5f, .25f, .8, .95);
    for (unsigned i = 0; i < before.size(); ++i)
        assert(std::abs(transition.weights()[i] - before[i]) < .002);
    std::ostringstream metadata;
    transition.writeJson(metadata);
    transition.writeScore(metadata);
    assert(metadata.str().find("bassPhraseTimbreVersion\":1") != std::string::npos);
    assert(metadata.str().find("bassPhraseTimbreNotes") != std::string::npos);
    std::cout << "Bass phrase timbre: deterministic spectra, bounds, slew, bypass and metadata PASS\n";
}
