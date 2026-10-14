#include "../engine/RasgarAccentField.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}
}

int main() {
    using namespace rasgo;
    RasgarAccentField field;

    const auto neutral = field.evaluate({});
    check(neutral.velocity >= 0.0 && neutral.velocity <= 1.0
              && neutral.gate >= 0.05 && neutral.gate <= 1.0,
          "neutral accent remains in playable bounds");

    const auto strong = field.evaluate({1.0, 1.0, 0.0, 1.0, 1.0}, 0.7, 0.75, 0.2);
    check(strong.velocity > neutral.velocity && strong.gate > neutral.gate,
          "combined accent raises energy and duration");
    check(strong.timbre >= 0.0 && strong.timbre <= 0.2
              && std::abs(strong.microPitchSemitones) <= 0.25,
          "accent translation respects timbre and micro-pitch budgets");

    const auto clamped = field.evaluate({-20.0, 20.0, 20.0, -20.0, 20.0},
                                         2.0, -2.0, -1.0);
    check(clamped.velocity >= 0.0 && clamped.velocity <= 1.0
              && clamped.gate >= 0.05 && clamped.gate <= 1.0
              && clamped.timbre == 0.0,
          "invalid ranges are contained without leaking into output");

    std::cout << "RasgarAccentField tests passed\n";
    return 0;
}
