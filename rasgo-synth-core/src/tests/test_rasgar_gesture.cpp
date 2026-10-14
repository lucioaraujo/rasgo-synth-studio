#include <cassert>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>

#include "engine/RasgarGesture.hpp"

using namespace rasgo;

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif

struct GestureFixture {
    OpenModuleArchive archive;
    CompositionalTopology topology{archive};
    CompositionIdentity identity;
    GamePlan game;
    RegionId time;
    RegionId rhythm;
    RasgarGesture gesture;

    explicit GestureFixture(unsigned seed)
        : identity(makeCompositionIdentity(seed)),
          game(makeGamePlan(seed, identity, 0.25, 0.25, 0.25, 0.25)),
          time(topology.addRegion(RegionRole::Core, 1.0, game.energy, 0.45)),
          rhythm(topology.addRegion(RegionRole::Rhythm, 0.0, game.energy, 0.35)),
          gesture(seed, identity, game, topology, archive, time, rhythm) {}
};

int main() {
    std::set<int> motions;
    std::array<unsigned, 6> firstSeedForMotion{};
    for (unsigned seed = 1; seed <= 2048; ++seed) {
        GestureFixture fixture(seed);
        const int motion = static_cast<int>(fixture.gesture.motion());
        motions.insert(motion);
        if (firstSeedForMotion[static_cast<std::size_t>(motion)] == 0)
            firstSeedForMotion[static_cast<std::size_t>(motion)] = seed;
    }
    assert(motions.size() == 6);

    GestureFixture fixture(57);
    int firstTearBar = -1;
    double tearDeviation = 0.0;
    for (int bar = 0; bar < 24; ++bar) {
        RasgarFrame frame = fixture.gesture.advanceBar(
            {0.62, 0.48, 0.72, bar == 6 || bar == 14}, bar);
        if (frame.tearTriggered && firstTearBar < 0) {
            firstTearBar = bar;
            for (int step = 0; step < 16; ++step)
                tearDeviation += std::fabs(fixture.gesture.stepDurationScale(step, 16) - 1.0);
            assert(frame.cism.interior != 0);
            assert(frame.cism.exterior != 0);
            assert(frame.cism.opening != 0);
        }
    }
    assert(firstTearBar >= 0 && firstTearBar <= 10);
    assert(tearDeviation > 0.8);
    assert(fixture.gesture.tearCount() >= 1);
    assert(fixture.gesture.maxInstability() >= 0.99);
    assert(fixture.gesture.meanPulseDeviation() > 0.02);
    assert(fixture.topology.splitCount() == fixture.gesture.tearCount());
    assert(fixture.topology.traversalCount() > fixture.gesture.tearCount());
    assert(fixture.archive.count(ArchiveAction::Split) >=
           static_cast<std::size_t>(fixture.gesture.tearCount()));

    // Cism creates different read positions and only non-negative delays.
    assert(fixture.gesture.patternReadStep(5, 16, 1) >= 0);
    assert(fixture.gesture.patternReadStep(5, 16, 1) < 16);
    assert(fixture.gesture.voiceDelaySamples(2, 6000) >= 0);

    // Same seed and observations produce the same state history.
    GestureFixture repeat(57);
    for (int bar = 0; bar < 24; ++bar)
        repeat.gesture.advanceBar({0.62, 0.48, 0.72, bar == 6 || bar == 14}, bar);
    assert(repeat.gesture.motion() == fixture.gesture.motion());
    assert(repeat.gesture.tearCount() == fixture.gesture.tearCount());
    assert(std::fabs(repeat.gesture.meanPulseDeviation()
                   - fixture.gesture.meanPulseDeviation()) < 1.0e-12);

    std::cout << "rasgar gesture tests passed; first seed per motion:";
    for (std::size_t motion = 0; motion < firstSeedForMotion.size(); ++motion)
        std::cout << " " << rasgarMotionName(static_cast<RasgarMotion>(motion))
                  << "=" << firstSeedForMotion[motion];
    std::cout << "\n";
    return 0;
}
