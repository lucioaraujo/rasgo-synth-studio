#include <array>
#include <cmath>
#include <cstdio>

#include "../dsp/ModulationBus.hpp"

namespace {
bool approx(float left, float right, float tolerance = 0.0001f) {
    return std::abs(left - right) <= tolerance;
}
}

int main() {
    int passed = 0;
    int failed = 0;
    const auto check = [&](bool condition, const char* name) {
        if (condition) ++passed;
        else { ++failed; std::printf("FAIL: %s\n", name); }
    };

    rasgo::ModulationBus bus;
    bus.prepare(48000.0);
    const std::array<float, rasgo::ModulationBus::kDestinationCount> base { 0.2f, 0.3f, 0.4f, 0.5f };
    const rasgo::ModulationBus::Sources sources { 0.8f, -0.5f, 0.25f, 0.7f, 0.1f };

    check(bus.process(sources, base) == base, "disabled bus is bit-transparent");

    rasgo::ModulationBus::Route route;
    route.enabled = true;
    route.source = rasgo::ModulationBus::Source::Henon;
    route.destination = rasgo::ModulationBus::Destination::OscillatorShape;
    route.depth = 0.5f;
    route.minimum = 0.0f;
    route.maximum = 1.0f;
    route.slewMs = 0.0f;
    bus.setRoute(0, route);
    auto values = bus.process(sources, base);
    check(approx(values[0], 0.6f), "linear route reaches its declared depth");
    check(approx(values[1], base[1]), "route changes only its declared destination");

    route.source = rasgo::ModulationBus::Source::Lorenz;
    route.destination = rasgo::ModulationBus::Destination::FolderAmount;
    route.curve = rasgo::ModulationBus::Curve::Rectified;
    route.depth = 1.0f;
    route.minimum = 0.0f;
    route.maximum = 0.65f;
    bus.setRoute(1, route);
    values = bus.process(sources, base);
    check(approx(values[1], 0.65f), "route clamps to its own safe range");

    route.source = rasgo::ModulationBus::Source::PercussionEnvelope;
    route.destination = rasgo::ModulationBus::Destination::SpaceMix;
    route.curve = rasgo::ModulationBus::Curve::Square;
    route.depth = 0.4f;
    route.minimum = 0.0f;
    route.maximum = 1.0f;
    route.slewMs = 50.0f;
    bus.setRoute(2, route);
    const auto first = bus.process(sources, base)[3];
    const auto later = bus.process(sources, base)[3];
    check(first > base[3] && first < base[3] + 0.4f, "slew avoids an instantaneous modulation jump");
    check(later > first, "slew moves monotonically toward the target");

    std::printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
