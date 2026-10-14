#include <cstdlib>
#include <iostream>

#include "../engine/SpatialExpressionField.hpp"

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
    SpatialExpressionField field;
    const auto neutral = field.interpret(SpatialExpressionInput{});
    require(neutral.valid(), "neutral spatial plan is valid");
    require(neutral.pan == 0.0, "neutral input stays centered");

    SpatialExpressionInput moving;
    moving.polarity = 1.0;
    moving.depth = 1.0;
    moving.drift = 1.0;
    moving.tension = 1.0;
    moving.monoSafety = 0.0;
    const auto safe = field.interpret(moving);
    require(safe.valid(), "spatial plan remains bounded");
    require(safe.width <= 0.85, "width has a mono-safe upper bound");
    require(safe.pan >= 0.0 && safe.driftRate >= 0.0,
            "polarity controls spatial direction");

    SpatialExpressionInput invalid;
    invalid.polarity = 7.0;
    invalid.depth = -4.0;
    invalid.drift = 9.0;
    const auto sanitized = field.interpret(invalid);
    require(sanitized.valid(), "out-of-range spatial input is sanitized");

    const SpatialFrame frame {0.8, -0.2};
    const auto widened = SpatialExpressionField::applyWidth(frame, 0.85);
    require(std::abs((widened.left + widened.right) - (frame.left + frame.right)) < 1.0e-12,
            "width transform preserves mono sum");
    require(widened.left != frame.left || widened.right != frame.right,
            "width transform changes stereo side when requested");

    std::cout << "SpatialExpressionField tests passed\n";
    return 0;
}
