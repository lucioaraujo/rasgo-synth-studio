#include <cstdlib>
#include <iostream>

#include "../engine/ExpressiveABReport.hpp"

namespace {
void require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::abort();
    }
}
}

int main() {
    using namespace rasgo;
    const auto baseline = ExpressiveInterpretation::neutral();
    auto variant = baseline;
    variant.enabled = true;
    variant.melodic.livingMidi = 61.0;
    variant.material.noiseAmount = 0.2;
    variant.spatial.pan = 0.15;

    const auto report = compareExpressiveInterpretations(41u, baseline, 41u, variant);
    require(report.valid(), "A/B report is valid");
    require(report.sameSeed && report.variantEnabled, "report preserves seed and variant state");
    require(report.changed, "report detects expressive differences");

    const auto wrongSeed = compareExpressiveInterpretations(41u, baseline, 42u, variant);
    require(!wrongSeed.sameSeed, "report rejects mismatched seed identity");

    std::cout << "ExpressiveABReport tests passed\n";
    return 0;
}
