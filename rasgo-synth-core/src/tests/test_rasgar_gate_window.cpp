#include "../engine/RasgarGateWindow.hpp"

#include <cstdlib>
#include <iostream>

namespace {
void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}
}

int main() {
    using namespace rasgo;
    RasgarGateWindow window;
    check(!window.tick(), "unarmed gate is closed");

    window.arm(100, 0.5);
    check(window.remainingSamples() == 50, "gate scales the step duration");
    for (int i = 0; i < 49; ++i) check(window.tick(), "gate remains open in its window");
    check(window.tick(), "gate includes its final sample");
    check(!window.tick(), "gate closes after its duration");

    window.arm(100, 2.0);
    check(window.remainingSamples() == 100, "gate clamps above one step");
    window.arm(100, 0.0);
    check(window.remainingSamples() == 5, "gate enforces a short minimum window");
    window.clear();
    check(!window.tick(), "clear closes an armed gate");

    std::cout << "RasgarGateWindow tests passed\n";
    return 0;
}
