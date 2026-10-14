#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../engine/PerformerState.hpp"

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

    PerformerState state;
    check(state.valid(), "default performer state is valid");

    const PerformerState initial = state;
    for (int i = 0; i < 24; ++i) {
        state = PerformerStateModel::advance(
            state, {1.2, 0.0, 0.9, 0.92, 0.75, 0.82, false, false});
        check(state.valid(), "sustained difficult work remains bounded");
    }
    check(state.fatigue > initial.fatigue,
          "dense effort accumulates performer fatigue");
    check(state.energy < initial.energy,
          "dense effort consumes performer energy");

    const PerformerState tired = state;
    state = PerformerStateModel::advance(
        state, {0.3, 4.0, 0.1, 0.2, 0.1, 1.0, true, true});
    check(state.fatigue < tired.fatigue,
          "rest and phrase boundary recover fatigue");
    check(state.energy > tired.energy,
          "rest restores performer energy");
    check(state.confidence >= tired.confidence,
          "successful recovery does not reduce confidence");

    PerformerState invalid;
    invalid.energy = std::nan("");
    invalid.fatigue = 4.0;
    const auto sanitized = PerformerStateModel::advance(
        invalid, {std::nan(""), -2.0, 8.0, -1.0, 3.0, 2.0, false, false});
    check(sanitized.valid(), "invalid inputs are sanitized to a bounded state");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "PerformerState tests passed\n";
    return EXIT_SUCCESS;
}
