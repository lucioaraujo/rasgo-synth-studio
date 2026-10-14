#include "../dsp/TruePeakDetector.hpp"
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>

int main() {
    int failures = 0;
    auto check = [&](bool value, const char* name) {
        if (!value) {
            if (failures < 12) std::printf("FAIL: %s\n", name);
            ++failures;
        }
    };
    for (double rate : {44100.0, 48000.0, 96000.0}) {
        rasgo::TruePeakDetector meter;
        meter.prepare(rate);
        for (float impulse : {1.0f, -1.0f, .2f, -.2f}) {
            meter.reset();
            float peak = meter.process(impulse);
            for (std::size_t i = 0; i < meter.kTapsPerPhase; ++i)
                peak = std::max(peak, meter.process(0));
            check(peak >= std::abs(impulse), "reconstructed estimate cannot under-read a source impulse");
        }
        meter.reset();
        check(meter.process(std::numeric_limits<float>::quiet_NaN()) == 0,
              "non-finite silence remains zero");
        check(meter.process(std::numeric_limits<float>::infinity()) == 0,
              "infinite silence remains zero");
        std::array<float, 1024> reference{};
        meter.reset();
        for (std::size_t i = 0; i < reference.size(); ++i) {
            const float input = static_cast<float>(std::sin(i * .71) * .9);
            reference[i] = meter.process(input);
            check(reference[i] >= std::abs(input) && std::isfinite(reference[i]),
                  "each finite input is a lower bound");
        }
        meter.reset();
        for (std::size_t i = 0; i < reference.size(); ++i) {
            const float input = static_cast<float>(std::sin(i * .71) * .9);
            check(meter.process(input) == reference[i], "reset determinism");
        }
    }
    std::printf("true peak: %d failures\n", failures);
    return failures != 0;
}
