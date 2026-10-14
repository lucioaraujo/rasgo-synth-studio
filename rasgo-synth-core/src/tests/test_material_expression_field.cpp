#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../engine/MaterialExpressionField.hpp"

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
    MaterialExpressionField field;
    const auto neutral = field.interpret(MaterialExpressionInput{});
    require(neutral.valid(), "neutral material plan is valid");
    require(neutral.noiseAmount == 0.0, "neutral input does not invent noise");

    MaterialExpressionInput stressed;
    stressed.brightness = 1.0;
    stressed.harmonicity = 1.0;
    stressed.noise = 1.0;
    stressed.degradation = 1.0;
    stressed.tension = 1.0;
    const auto bounded = field.interpret(stressed);
    require(bounded.valid(), "stressed material plan stays bounded");
    require(bounded.noiseAmount > neutral.noiseAmount && bounded.air > neutral.air,
            "material causes reach their intended destinations");

    MaterialExpressionInput invalid;
    invalid.brightness = std::nan("");
    const auto sanitized = field.interpret(invalid);
    require(sanitized.valid(), "non-finite input is sanitized");

    std::cout << "MaterialExpressionField tests passed\n";
    return 0;
}
