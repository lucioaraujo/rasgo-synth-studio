#include "../engine/RasgarTimingMemory.hpp"

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
    RasgarTimingMemory memory;

    check(memory.predict(RasgarTimingVoice::Kick, 6.0) == 0.0,
          "new memory starts on grid");
    memory.beginSection(RasgarTimingVoice::Kick, 0);
    const double first = memory.observe(RasgarTimingVoice::Kick, 20.0, 6.0);
    check(first > 0.0 && first <= 6.0,
          "normal timing memory is bounded by its residual budget");
    const double second = memory.observe(RasgarTimingVoice::Kick, -20.0, 6.0);
    check(second < first && second >= -6.0,
          "correlated memory can change direction without leaving bounds");

    memory.beginSection(RasgarTimingVoice::Kick, 1, 6.0, 0.5);
    check(std::abs(memory.predict(RasgarTimingVoice::Kick, 6.0)) < 6.0,
          "section transition morphs residue instead of copying it whole");
    check(memory.predict(RasgarTimingVoice::Snare, 6.0) == 0.0,
          "voice memories remain independent");

    memory.observe(RasgarTimingVoice::Hihat, -40.0, 12.0);
    check(memory.predict(RasgarTimingVoice::Hihat, 12.0) >= -12.0,
          "wild budget remains bounded on the early side");
    memory.reset();
    check(memory.predict(RasgarTimingVoice::Hihat, 12.0) == 0.0,
          "reset clears temporal residue");

    std::cout << "RasgarTimingMemory tests passed\n";
    return 0;
}
