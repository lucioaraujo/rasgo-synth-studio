#include <cstdlib>
#include <iostream>

#include "../engine/MelodicCharacterIntent.hpp"

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
    MelodicCharacterIntent interpreter;
    const auto neutral = interpreter.interpret({});
    require(neutral.valid(), "neutral character plan is valid");

    const auto restless = interpreter.interpret({MelodicCharacter::Restless, 1.0, 1.0, 0.0});
    const auto spacious = interpreter.interpret({MelodicCharacter::Spacious, 0.0, 1.0, 1.0});
    require(restless.valid() && spacious.valid(), "character plans stay bounded");
    require(restless.pitchInstability > spacious.pitchInstability,
            "character changes pitch instability tendency");
    require(spacious.densityBias < restless.densityBias,
            "character changes density tendency");

    const auto sanitized = interpreter.interpret({MelodicCharacter::Assertive, 8.0, -2.0, 7.0});
    require(sanitized.valid(), "intent inputs are clamped");

    std::cout << "MelodicCharacterIntent tests passed\n";
    return 0;
}
