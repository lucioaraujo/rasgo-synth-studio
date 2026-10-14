#include <cstdlib>
#include <iostream>

#include "../engine/StructuralTransformation.hpp"

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
    StructuralTransformation interpreter;
    const auto variation = interpreter.interpret({
        StructuralTransformationKind::Variation, 0.8, 0.7, 0.2, 0.9, true});
    require(variation.valid(), "declared variation is valid");
    require(variation.residual <= 0.35, "variation residual stays modest");

    const auto glitch = interpreter.interpret({
        StructuralTransformationKind::Glitch, 0.8, 0.3, 0.6, 0.0, true});
    require(glitch.valid() && glitch.residual >= 0.09,
            "glitch leaves an observable residual");

    const auto weakRupture = interpreter.interpret({
        StructuralTransformationKind::Rupture, 0.8, 0.7, 0.2, 0.0, true});
    require(!weakRupture.valid(), "weak rupture is rejected");

    const auto rupture = interpreter.interpret({
        StructuralTransformationKind::Rupture, 0.9, 0.2, 0.8, 0.3, true});
    require(rupture.valid(), "declared rupture has magnitude and residue");

    std::cout << "StructuralTransformation tests passed\n";
    return 0;
}
