#include <cstdlib>
#include <iostream>

#include "../engine/OrchestralSectionMemory.hpp"

namespace {
int failures = 0;
void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}
}

int main() {
    using namespace rasgo;
    OrchestralSectionMemory memory;

    memory.beginSection();
    memory.observe(OrganismStem::Tuba, {0.8, 0.55, 0.20, 0.05, 0.8, 43.0, true});
    memory.observe(OrganismStem::FmFlute, {0.4, 0.02, 0.10, 0.34, 0.7, 78.0, true});
    auto first = memory.finishSection();
    check(first.strongest == OrganismStem::Tuba, "strongest body is measured by RMS");
    check(first.activeCount == 2 && first.activeShare == 1.0,
          "active section coverage is measured");
    check(first.highConflictCount == 0, "one high layer creates no high conflict");

    memory.beginSection();
    memory.observe(OrganismStem::Tuba, {0.3, 0.20, 0.08, 0.02, 0.5, 43.0, true});
    memory.observe(OrganismStem::FmFlute, {0.5, 0.02, 0.08, 0.40, 0.8, 78.0, true});
    memory.observe(OrganismStem::Theremin, {0.2, 0.01, 0.04, 0.17, 0.7, 84.0, true});
    auto second = memory.finishSection();
    check(second.strongest == OrganismStem::FmFlute,
          "lead can move between sections");
    check(second.highConflictCount == 1,
          "simultaneous bright layers become an explicit conflict");
    check(!second.repeatedLeader, "leader change is distinguished from repetition");

    memory.beginSection();
    memory.observe(OrganismStem::FmFlute, {0.5, 0.02, 0.08, 0.40, 0.8, 78.0, true});
    auto third = memory.finishSection();
    check(third.repeatedLeader, "repeated leader is remembered causally");
    check(memory.sectionCount() == 3, "section memory counts completed sections");

    memory.beginSection();
    memory.observe(OrganismStem::Texture,
                   {std::numeric_limits<double>::quiet_NaN(), -1.0, 0.0,
                    0.0, 3.0, std::numeric_limits<double>::infinity(), true});
    auto safe = memory.finishSection();
    check(safe.strongest == OrganismStem::Other && safe.highBandShare == 0.0,
          "invalid observations are made safe without inventing activity");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "OrchestralSectionMemory tests passed\n";
    return EXIT_SUCCESS;
}
