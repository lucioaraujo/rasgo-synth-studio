#include <array>
#include <cmath>
#include <cstdio>
#include <set>
#include <string>

#include "../engine/GamePlan.hpp"
#include "../sequencer/StepSequencer.hpp"

int main() {
    int passed = 0, failed = 0;
    auto check = [&](bool condition, const char *name) {
        if (condition) ++passed;
        else { ++failed; std::printf("FAIL: %s\n", name); }
    };

    std::array<int, 6> strategyCounts{};
    std::array<int, 6> ruleCounts{};
    bool resourcesValid = true;
    bool deterministic = true;

    constexpr unsigned kSeeds = 4096;
    for (unsigned seed = 0; seed < kSeeds; ++seed) {
        auto identity = rasgo::makeCompositionIdentity(seed);
        double cuts[3] = {
            rasgo::identityUnit(seed, 130),
            rasgo::identityUnit(seed, 131),
            rasgo::identityUnit(seed, 132)
        };
        std::sort(cuts, cuts + 3);
        double alea = cuts[0];
        double agon = cuts[1] - cuts[0];
        double ilinx = cuts[2] - cuts[1];
        double mimicry = 1.0 - cuts[2];
        auto plan = rasgo::makeGamePlan(seed, identity, alea, agon, ilinx, mimicry);
        ++strategyCounts[static_cast<std::size_t>(plan.strategy)];
        ++ruleCounts[static_cast<std::size_t>(plan.rule)];

        const double resources[] = {
            plan.energy, plan.memory, plan.risk, plan.freedom, plan.hiddenInformation
        };
        for (double value : resources) {
            if (!std::isfinite(value) || value < 0.0 || value > 1.0) resourcesValid = false;
        }
        for (int bar = 0; bar < 8; ++bar) {
            double density = plan.composeDensity(0.1 * static_cast<double>(bar + 1), bar);
            if (!std::isfinite(density) || density < 0.03 || density > 1.0) resourcesValid = false;
        }

        auto repeat = rasgo::makeGamePlan(seed, identity, alea, agon, ilinx, mimicry);
        if (repeat.strategy != plan.strategy || repeat.rule != plan.rule
            || repeat.energy != plan.energy || repeat.memory != plan.memory
            || repeat.risk != plan.risk || repeat.freedom != plan.freedom
            || repeat.hiddenInformation != plan.hiddenInformation) {
            deterministic = false;
        }
    }

    auto everySeen = [](const auto &counts, int minimum) {
        for (int count : counts) if (count < minimum) return false;
        return true;
    };
    check(everySeen(strategyCounts, 300), "all game strategies have substantial coverage");
    check(everySeen(ruleCounts, 180), "all game rules have substantial coverage");
    check(resourcesValid, "resources and composed densities remain finite and bounded");
    check(deterministic, "game plan is deterministic for seed, identity and Caillois field");

    // At the same narrative density, the six strategies must not collapse
    // into the same generator instruction.
    std::set<int> densitySignatures;
    for (int strategy = 0; strategy < 6; ++strategy) {
        rasgo::GamePlan plan;
        plan.strategy = static_cast<rasgo::GameStrategy>(strategy);
        plan.energy = 0.63;
        plan.memory = 0.47;
        plan.risk = 0.72;
        plan.freedom = 0.58;
        plan.hiddenInformation = 0.66;
        int signature = 0;
        for (int bar = 0; bar < 4; ++bar) {
            signature = signature * 1001
                      + static_cast<int>(std::lround(plan.composeDensity(0.62, bar) * 1000.0));
        }
        densitySignatures.insert(signature);
    }
    check(densitySignatures.size() == 6, "each strategy has a distinct temporal consequence");

    rasgo::StepSequencer lowMemory, highMemory;
    lowMemory.configure(16, rasgo::Scale::Dorian, 52, 991);
    highMemory.configure(16, rasgo::Scale::Dorian, 52, 991);
    lowMemory.applyGameCharacter(0.05, 0.85, 0.65);
    highMemory.applyGameCharacter(0.95, 0.20, 0.15);
    check(highMemory.lockAmount() > lowMemory.lockAmount(),
          "game memory directly increases melodic persistence");
    check(lowMemory.jumpiness() > highMemory.jumpiness(),
          "freedom and risk directly increase melodic mobility");

    std::printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
