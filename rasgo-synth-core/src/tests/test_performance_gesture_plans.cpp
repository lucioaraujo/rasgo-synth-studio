#include <cstdlib>
#include <iostream>

#include "../engine/PerformanceGesturePlans.hpp"

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
    PerformerState performer;
    performer.energy = 0.82;
    performer.effort = 0.70;
    performer.confidence = 0.66;
    performer.precision = 0.72;
    performer.tension = 0.58;
    performer.fatigue = 0.18;
    performer.risk = 0.62;
    performer.expressivity = 0.76;
    performer.attention = 0.80;

    const auto hit = PerformanceGesturePlanner::planHit(performer, 0.9, 0.72, 0.38);
    const auto texture = PerformanceGesturePlanner::planTexture(
        performer, 0.68, 0.74, 0.42, 0.44);
    check(hit.valid(), "hit plan remains bounded");
    check(texture.valid(), "texture plan remains bounded");
    check(hit.impact > hit.decay, "hit distinguishes impact from decay");
    check(texture.spectralBrightness != hit.impact,
          "texture has spectral trajectory rather than impact semantics");
    check(texture.memory > 0.0 && texture.drift > 0.0,
          "texture carries memory and slow drift");

    PerformerState fragile = performer;
    fragile.confidence = 0.16;
    fragile.precision = 0.20;
    fragile.fatigue = 0.92;
    fragile.attention = 0.24;
    const auto fragileHit = PerformanceGesturePlanner::planHit(fragile, 0.9, 0.72, 0.82, 0.66);
    const auto fragileTexture = PerformanceGesturePlanner::planTexture(
        fragile, 0.68, 0.74, 0.42, 0.82);
    check(fragileHit.failureLikelihood > hit.failureLikelihood,
          "hit failure follows state and difficulty");
    check(fragileHit.recoveryStrength > 0.0,
          "hit carries a recovery path");
    check(fragileTexture.instability > texture.instability,
          "texture instability follows fragile state");
    check(fragileTexture.dissipation > texture.dissipation,
          "texture dissipation follows fatigue");

    PerformerState invalid;
    invalid.energy = 4.0;
    invalid.fatigue = -2.0;
    check(PerformanceGesturePlanner::planHit(invalid, 9.0, -2.0, 7.0).valid(),
          "invalid hit inputs are sanitized");
    check(PerformanceGesturePlanner::planTexture(invalid, 9.0, -2.0, 8.0, 7.0).valid(),
          "invalid texture inputs are sanitized");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "PerformanceGesturePlans tests passed\n";
    return EXIT_SUCCESS;
}
