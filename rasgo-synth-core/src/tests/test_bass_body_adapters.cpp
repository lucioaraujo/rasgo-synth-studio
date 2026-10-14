#include <cstdlib>
#include <iostream>

#include "../engine/BassBodyAdapters.hpp"

namespace {
int failures = 0;

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

rasgo::BassPerformancePlan makePerformance(std::uint32_t seed,
                                           double gravity,
                                           double density,
                                           double tension) {
    rasgo::BassFunctionRequest function;
    function.seed = seed;
    function.gravity = gravity;
    function.density = density;
    function.narrativeTension = tension;
    function.allowSilence = false;
    function.kickPattern = {true, false, false, false, true, false, false, false,
                            true, false, false, false, true, false, false, false};
    rasgo::BassPhraseRequest phrase;
    phrase.function = rasgo::BassFunctionPlanner::plan(function);
    phrase.kickPattern = function.kickPattern;
    rasgo::BassPerformanceRequest performance;
    performance.phrase = rasgo::BassPhrasePlanner::plan(phrase);
    return rasgo::BassPerformancePlanner::plan(performance);
}
}

int main() {
    using namespace rasgo;

    BassPerformancePlan foundation;
    for (std::uint32_t seed = 1; seed <= 512; ++seed) {
        const auto candidate = makePerformance(seed, 1.0, 0.42, 0.35);
        if (candidate.role == BassFunctionRole::Foundation
            || candidate.role == BassFunctionRole::Pedal) {
            foundation = candidate;
            break;
        }
    }
    check(foundation.valid(), "foundation fixture is valid");
    const auto tuba = BassBodyAdapterPlanner::plan(
        foundation, {true, true, true});
    check(tuba.valid() && tuba.primaryBody == BassBodyKind::Tuba
          && tuba.tubaEventCount == static_cast<int>(tuba.activeStepCount),
          "foundation chooses wind body when tuba is available");

    BassPerformancePlan energetic;
    for (std::uint32_t seed = 1; seed <= 512; ++seed) {
        const auto candidate = makePerformance(seed, 0.45, 1.0, 1.0);
        if (candidate.role == BassFunctionRole::Energy
            || candidate.role == BassFunctionRole::Ostinato
            || candidate.role == BassFunctionRole::Interlock) {
            energetic = candidate;
            break;
        }
    }
    check(energetic.valid(), "energetic fixture is valid");
    const auto acid = BassBodyAdapterPlanner::plan(
        energetic, {true, true, true});
    check(acid.valid() && acid.primaryBody == BassBodyKind::Acid
          && acid.acidEventCount == static_cast<int>(acid.activeStepCount),
          "rhythmic/energetic role preserves acid identity");

    const auto fallback = BassBodyAdapterPlanner::plan(
        energetic, {false, false, true});
    check(fallback.valid() && fallback.primaryBody == BassBodyKind::CoreSub
          && fallback.coreSubEventCount == static_cast<int>(fallback.activeStepCount),
          "core-sub guarantees a body when rare voices are absent");

    std::size_t index = 0;
    while (index < acid.stepCount && !acid.steps[index].active) ++index;
    check(index < acid.stepCount, "body plans contain an active directive");
    if (index < acid.stepCount) {
        check(tuba.steps[index].breath > 0.0 && acid.steps[index].breath == 0.0,
              "tuba breath is not copied into acid body");
        check(fallback.steps[index].kickDuck >= 0.0
              && fallback.steps[index].drive <= acid.steps[index].drive,
              "fallback is restrained and kick-aware");
    }

    const auto unavailable = BassBodyAdapterPlanner::plan(
        energetic, {false, false, false});
    check(!unavailable.valid() && !unavailable.sourceValid,
          "missing all bodies is exposed instead of silently dropping events");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "BassBodyAdapters tests passed\n";
    return EXIT_SUCCESS;
}
