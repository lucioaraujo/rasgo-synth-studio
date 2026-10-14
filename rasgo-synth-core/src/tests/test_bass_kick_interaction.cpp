#include "../engine/BassKickInteraction.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>

static void check(bool condition) { if (!condition) std::abort(); }

int main() {
    using rasgo::BassKickInteraction;
    for (const double rate : {8000.0, 44100.0, 48000.0, 96000.0}) {
        BassKickInteraction active(true, rate), repeat(true, rate), off(false, rate);
        for (int frame = 0; frame < 100; ++frame) {
            active.tick(); check(active.gain(.7) == 1.0f);
        }
        // Restore a matching repeat: silent frames leave exactly neutral state.
        for (int frame = 0; frame < static_cast<int>(rate); ++frame) {
            if (frame == 0) { active.kick(.8); repeat.kick(.8); off.kick(.8); }
            active.tick(); repeat.tick(); off.tick();
            const auto value = active.gain(.7);
            check(value == repeat.gain(.7) && value >= .58f && value <= 1.0f);
            check(off.gain(.7) == static_cast<float>(1.0 - .7 * .42));
            if (frame == static_cast<int>(rate * .01)) check(value < .95f);
            if (frame == static_cast<int>(rate * .5)) check(value > .999f);
        }
        check(active.triggers() == 1 && active.reducedFrames() > 0);
        check(!off.enabled() && off.triggers() == 0 && off.reducedFrames() == 0);
        active.kick(0); active.kick(-1);
        active.kick(std::numeric_limits<double>::quiet_NaN());
        active.kick(std::numeric_limits<double>::infinity());
        check(active.triggers() == 1);
        for (int frame = 0; frame < static_cast<int>(rate * 2); ++frame) active.tick();
        check(active.gain(.7) == 1.0f); // Recovery occurs during bass rests, too.
        active.kick(99);
        for (int frame = 0; frame < static_cast<int>(rate * .25); ++frame) {
            active.tick(); check(active.gain(99) >= .58f && active.gain(-1) == 1);
            check(active.gain(std::numeric_limits<double>::quiet_NaN()) == 1);
        }
    }
    BassKickInteraction invalidRate(true, std::numeric_limits<double>::quiet_NaN());
    invalidRate.kick(1); invalidRate.tick();
    check(std::isfinite(invalidRate.gain(1)));
    std::cout << "PASS kick-only reduction/recovery, exact legacy bypass, rates/bounds/repeat/nonfinite\n";
}
