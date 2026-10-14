#include <cstdlib>
#include <iostream>

#include "../engine/ExpressiveInterpretation.hpp"

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
    auto neutral = ExpressiveInterpretation::neutral();
    require(!neutral.enabled, "neutral interpretation is disabled");
    require(neutral.valid(), "neutral interpretation is valid");

    MelodicInterpreter interpreter(23u);
    MelodicEvent event;
    event.midi = 72.0;
    event.duration = 0.8;
    event.density = 0.35;
    event.accent = 0.7;
    event.phrasePosition = 0.5;

    ExpressiveInterpretation active;
    active.enabled = true;
    active.melodic = interpreter.interpret(event);
    active.material = MaterialExpressionField{}.interpret({0.8, 0.7, 0.3, 0.2, 0.6});
    active.spatial = SpatialExpressionField{}.interpret({1.0, 0.4, 0.2, 0.6, 1.0});
    require(active.valid(), "active interpretation stays within all contracts");

    active.spatial.width = 2.0;
    require(!active.valid(), "invalid spatial destination is rejected");

    std::cout << "ExpressiveInterpretation tests passed\n";
    return 0;
}
