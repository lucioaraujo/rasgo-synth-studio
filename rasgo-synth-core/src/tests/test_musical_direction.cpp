#include "../engine/MusicalDirection.hpp"
#include "../engine/KitInterpretation.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif

int main() {
    using namespace rasgo;
    assert(MusicalDirection::advanceNoteCountdown(0, 48000) == 0);
    assert(MusicalDirection::advanceNoteCountdown(24000, 48000) == 0);
    assert(MusicalDirection::advanceNoteCountdown(48000, 48000) == 0);
    assert(MusicalDirection::advanceNoteCountdown(96000, 48000) == 48000);
    assert(MusicalDirection::advanceNoteCountdown(10, 0) == 10);
    assert(MusicalDirection::melodicRegisterOffset(-12) == -12);
    assert(MusicalDirection::melodicRegisterOffset(-2) == 0);
    assert(MusicalDirection::melodicRegisterOffset(2) == 0);
    assert(MusicalDirection::melodicRegisterOffset(8) == 12);
    AgencyContributionMeter meter;
    meter.setBarObservationEnabled(true);
    for (int i = 0; i < 16; ++i) { meter.add(AgencyFamily::Rhythm, .25, .25); meter.finishFrame(); }
    const auto observed = meter.takeBarObservation();
    assert(observed.frames == 16 && observed.rms[static_cast<unsigned>(AgencyFamily::Rhythm)] == .25);
    assert(meter.takeSectionObservation().frames == 16); // bar observation cannot consume section history
    assert(meter.takeBarObservation().frames == 0);

    std::array<bool, static_cast<std::size_t>(OrganismStem::Count)> active{};
    for (auto stem : {OrganismStem::Core, OrganismStem::Drums, OrganismStem::BassFallback,
                      OrganismStem::FmFlute, OrganismStem::Texture}) active[static_cast<unsigned>(stem)] = true;
    AgencyObservation none;
    MusicalDirection off(42, false, 48000);
    off.beginBar(0, 1, true, false, .9, observed); off.tick();
    assert(off.gain(OrganismStem::Drums) == 1 && off.mayStart(OrganismStem::Drums) && off.barCount() == 0);
    int responses = 0, rests = 0, flashes = 0;
    for (unsigned seed = 0; seed < 32; ++seed) {
        MusicalDirection a(seed, true, 48000), b(seed, true, 48000), silent(seed, true, 48000);
        a.setAvailable(active); b.setAvailable(active); silent.setAvailable(active);
        int lastFlash = -4;
        for (int bar = 0; bar < 600; ++bar) {
            const bool coda = bar >= 590;
            a.beginBar(bar, bar / 12, true, coda, .7, observed);
            b.beginBar(bar, bar / 12, true, coda, .7, observed);
            silent.beginBar(bar, bar / 12, true, coda, .7, none);
            assert(a.plan().presence == b.plan().presence && a.plan().lead == b.plan().lead);
            for (unsigned note = 0; note < 12; ++note)
                for (int scale = 1; scale <= 12; ++scale) {
                    const int degree = a.motifDegree(note, scale);
                    assert(degree >= 0 && degree < scale && degree == b.motifDegree(note, scale));
                }
            assert(active[static_cast<unsigned>(OrganismStem::Core)]);
            for (double gain : a.plan().presence) assert(std::isfinite(gain) && gain >= 0 && gain <= 1.12);
            if (a.plan().cause == DirectionCause::HeardRhythm) {
                assert(silent.plan().cause != DirectionCause::HeardRhythm); ++responses;
            }
            if (!a.mayStart(OrganismStem::FmFlute)) ++rests;
            if (a.plan().highFlash) { assert(!coda && bar - lastFlash >= 4); lastFlash = bar; ++flashes; }
            if (coda) assert(!a.mayStart(OrganismStem::Drums) && !a.mayStart(OrganismStem::BassFallback));
            const double before = a.gain(OrganismStem::FmFlute);
            a.tick(); b.tick();
            assert(std::abs(a.gain(OrganismStem::FmFlute) - before) < .002);
            assert(a.gain(OrganismStem::FmFlute) == b.gain(OrganismStem::FmFlute));
            assert(a.gain(OrganismStem::ThermalStatic) == 1);
        }
        assert(a.barCount() == 600 && a.traceCount() == MusicalDirection::kTraceLimit);
        std::ostringstream out; a.writeJson(out);
        assert(out.str().find("\"directionTraceOmitted\":344") != std::string::npos);
        assert(out.str().size() < 131072);
    }
    assert(responses > 0 && rests > 0 && flashes > 0);
    MusicalDirection unavailable(42, true, 48000);
    decltype(active) empty{};
    unavailable.setAvailable(empty);
    unavailable.beginBar(0, 0, true, false, .8, none);
    for (double presence : unavailable.plan().presence) assert(presence == 0.0);
    empty[static_cast<unsigned>(OrganismStem::Choir)] = true;
    unavailable.setAvailable(empty);
    unavailable.beginBar(0, 0, true, false, .8, none);
    assert(unavailable.plan().lead == MusicalDesk::Event);
    for (unsigned i = 0; i < 5; ++i) assert(unavailable.plan().presence[i] == 0.0);
    for (double material : {-0.8, 0.0, 0.8, std::numeric_limits<double>::quiet_NaN()})
        for (double articulation : {0.0, .5, 1.0}) {
            auto kit = KitInterpretation::interpret(material, articulation, .8);
            assert(std::isfinite(kit.decay) && kit.decay >= .45 && kit.decay <= 1.75);
            assert(kit.brightness >= .55 && kit.brightness <= 1.40);
            assert(kit.noise >= .40 && kit.noise <= 1.10);
        }
    auto shortKit = KitInterpretation::interpret(-.7, .9, .8);
    auto longKit = KitInterpretation::interpret(.7, .1, .8);
    assert(shortKit.decay < longKit.decay && shortKit.brightness < longKit.brightness);
    std::cout << "32 seeds x 600 bars: measured response, rest, bounded trace, smoothing, coda, flashes; kit bounds: PASS\n";
}
