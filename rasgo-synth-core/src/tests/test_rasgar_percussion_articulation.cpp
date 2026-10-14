#include "../engine/RasgarPercussionArticulation.hpp"

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
    RasgarPercussionArticulationAdapter adapter;
    RasgarAccentPlan plan;
    plan.gate = 0.4;
    plan.microPitchSemitones = 0.5;
    plan.timbre = 0.8;

    const auto kick = adapter.translate(RasgarPercussionVoice::Kick, plan);
    check(kick.gate == 0.4 && std::abs(kick.microPitchSemitones) <= 0.18,
          "kick articulation preserves gate and applies its pitch budget");
    check(kick.decayScale >= 0.70 && kick.decayScale <= 1.30,
          "decay remains bounded");

    const auto hihat = adapter.translate(RasgarPercussionVoice::Hihat, plan, 0.0);
    check(hihat.microPitchSemitones == 0.0 && hihat.timbreAmount == 0.0,
          "zero articulation budget is neutral for optional fields");
    check(hihat.gate == 0.4,
          "gate remains an explicit articulation field");

    const auto cowbell = adapter.translate(RasgarPercussionVoice::Cowbell, plan, 2.0);
    check(std::abs(cowbell.microPitchSemitones) <= 0.12,
          "cowbell uses its narrower pitch budget");

    std::cout << "RasgarPercussionArticulation tests passed\n";
    return 0;
}
