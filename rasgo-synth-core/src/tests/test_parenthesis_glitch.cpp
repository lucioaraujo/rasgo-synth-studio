#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include "../dsp/ParenthesisGlitch.hpp"

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
    using rasgo::ParenthesisGlitch;

    auto make = [] {
        ParenthesisGlitch glitch;
        glitch.setSampleRate(1000.0);
        for (int i = 0; i < 100; ++i) glitch.feed(static_cast<float>(i), static_cast<float>(i));
        return glitch;
    };

    {
        auto glitch = make();
        glitch.open(0.05, 0.01, 0.001, ParenthesisGlitch::PlaybackMode::Forward);
        float l = 0.0f, r = 0.0f;
        glitch.tick(l, r);
        check(std::fabs(l - 90.0f) < 0.001f, "forward starts at the captured window");
        check(std::string(ParenthesisGlitch::playbackModeName(glitch.playbackMode())) == "forward",
              "forward mode is exposed");
    }
    {
        auto glitch = make();
        glitch.open(0.05, 0.01, 0.001, ParenthesisGlitch::PlaybackMode::Reverse);
        float l = 0.0f, r = 0.0f;
        glitch.tick(l, r);
        check(std::fabs(l - 99.0f) < 0.001f, "reverse starts at the end of the captured window");
    }
    {
        auto glitch = make();
        glitch.open(0.025, 0.01, 0.001, ParenthesisGlitch::PlaybackMode::PingPong);
        float l = 0.0f, r = 0.0f;
        const float expected[] = {90.0f, 91.0f, 92.0f, 93.0f, 94.0f,
                                  95.0f, 96.0f, 97.0f, 98.0f, 99.0f,
                                  98.0f, 97.0f, 96.0f, 95.0f, 94.0f,
                                  93.0f, 92.0f, 91.0f, 90.0f, 91.0f};
        for (float value : expected) {
            glitch.tick(l, r);
            check(std::fabs(l - value) < 0.001f,
                  "ping-pong traverses the fragment in both directions");
        }
    }
    {
        auto glitch = make();
        glitch.setCycleVariation(true);
        glitch.open(0.025, 0.01, 0.001, ParenthesisGlitch::PlaybackMode::Forward);
        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < 10; ++i) glitch.tick(l, r);
        glitch.tick(l, r);
        check(std::fabs(l - 93.0f) < 0.001f,
              "cycle variation changes the second forward read phase");
    }
    {
        auto glitch = make();
        glitch.setCycleVariation(true, 3, 5);
        glitch.open(0.025, 0.01, 0.001, ParenthesisGlitch::PlaybackMode::Forward);
        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < 10; ++i) glitch.tick(l, r);
        glitch.tick(l, r);
        check(std::fabs(l - 98.0f) < 0.001f,
              "custom phase and stride alter the next forward cycle reproducibly");
    }

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "ParenthesisGlitch tests passed\n";
    return EXIT_SUCCESS;
}
