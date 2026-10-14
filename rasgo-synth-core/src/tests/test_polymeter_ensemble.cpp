#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <set>
#include "../sequencer/PolymeterEnsemble.hpp"

using namespace rasgo;

int main() {
    int pass = 0, fail = 0;
    auto check = [&](bool cond, const char *name) {
        if (cond) { ++pass; } else { ++fail; std::printf("FAIL: %s\n", name); }
    };

    // Eixo A: 4 distinct cycle lengths from the curated coprime pool, and
    // the reported macroperiod is genuinely the real LCM of those four
    // (computed independently here, not trusting the class's own math).
    for (unsigned seed = 1; seed <= 20; ++seed) {
        PolymeterEnsemble ens;
        ens.setSampleRate(44100.0);
        ens.configure(60, seed);
        std::set<int> lengths;
        for (int v = 0; v < PolymeterEnsemble::kNumVoices; ++v) lengths.insert(ens.cycleLength(v));
        check(lengths.size() == PolymeterEnsemble::kNumVoices, "polymeter: four genuinely distinct cycle lengths");
        int expectedLcm = ens.cycleLength(0);
        for (int v = 1; v < PolymeterEnsemble::kNumVoices; ++v) expectedLcm = std::lcm(expectedLcm, ens.cycleLength(v));
        check(ens.macroperiodSteps() == expectedLcm, "polymeter: reported macroperiod is the real LCM");
    }

    // Eixo B: every voice's non-isochronous gaps sum EXACTLY to its own
    // cycle length -- the literal claim ("300+200+350+150=1000"), and
    // every onset step lands inside [0, cycleLength), starting at 0 (a
    // real downbeat every voice shares at its own cycle's start).
    for (unsigned seed = 1; seed <= 20; ++seed) {
        PolymeterEnsemble ens;
        ens.setSampleRate(44100.0);
        ens.configure(60, seed);
        for (int v = 0; v < PolymeterEnsemble::kNumVoices; ++v) {
            const auto &g = ens.gaps(v);
            int sum = 0; for (int x : g) sum += x;
            check(sum == ens.cycleLength(v), "polymeter: voice's non-isochronous gaps sum exactly to its own cycle length");
            check(!g.empty(), "polymeter: every voice has at least one real onset");
            // Real non-isochrony: not every gap is forced identical (this
            // is a real property to check, not just a definition -- a
            // buggy implementation could still produce all-equal gaps by
            // accident on some seeds, so check across the sample instead
            // of asserting it for every single seed individually).
        }
    }
    // Across the sampled seeds, at least some voice on some seed shows
    // genuinely unequal gaps -- the whole point of Eixo B.
    {
        bool sawUnequal = false;
        for (unsigned seed = 1; seed <= 20 && !sawUnequal; ++seed) {
            PolymeterEnsemble ens;
            ens.setSampleRate(44100.0);
            ens.configure(60, seed);
            for (int v = 0; v < PolymeterEnsemble::kNumVoices; ++v) {
                const auto &g = ens.gaps(v);
                for (size_t i = 1; i < g.size(); ++i) {
                    if (g[i] != g[0]) { sawUnequal = true; break; }
                }
            }
        }
        check(sawUnequal, "polymeter: real unequal gaps appear across seeds (genuine non-isochrony)");
    }

    // tickStep(): realignment is counted EXACTLY at multiples of the real
    // macroperiod, verified by literally stepping through several full
    // macroperiods and counting -- not trusting the modular-arithmetic
    // claim without simulating it.
    {
        PolymeterEnsemble ens;
        ens.setSampleRate(44100.0);
        ens.configure(60, 7);
        int macro = ens.macroperiodSteps();
        int numCycles = 3;
        for (size_t s = 0; s < static_cast<size_t>(macro * numCycles); ++s) ens.tickStep(s);
        // Realignment fires at step 0, macro, 2*macro -- exactly numCycles times
        // within [0, macro*numCycles).
        check(ens.realignmentCount() == numCycles, "polymeter: realignment count matches real simulated multiples of the macroperiod");
    }

    // Each voice's pulse count over exactly its own cycle length matches
    // its own real onset count (k).
    {
        PolymeterEnsemble ens;
        ens.setSampleRate(44100.0);
        ens.configure(60, 3);
        for (int v = 0; v < PolymeterEnsemble::kNumVoices; ++v) {
            PolymeterEnsemble solo;
            solo.setSampleRate(44100.0);
            solo.configure(60, 3);
            int len = solo.cycleLength(v);
            for (size_t s = 0; s < static_cast<size_t>(len); ++s) solo.tickStep(s);
            check(solo.pulseCount(v) == static_cast<int>(solo.gaps(v).size()), "polymeter: voice pulse count over one own-cycle matches its real onset count");
        }
    }

    // Output stays bounded, no NaN/Inf, across a real run with all voices
    // actively pulsing.
    {
        PolymeterEnsemble ens;
        ens.setSampleRate(44100.0);
        ens.configure(60, 5);
        ens.setGain(0.2f);
        bool anyNaN = false;
        float peak = 0.0f;
        size_t stepEvery = 200; // roughly step-rate at a plausible tempo
        for (size_t s = 0; s < 44100 * 2; ++s) {
            if (s % stepEvery == 0) ens.tickStep(s / stepEvery);
            float v = ens.tick();
            if (std::isnan(v) || std::isinf(v)) anyNaN = true;
            peak = std::max(peak, std::fabs(v));
        }
        check(!anyNaN, "polymeter: no NaN/Inf across a real 2-second run");
        check(peak > 0.0f && peak < 5.0f, "polymeter: bounded, non-silent output");
    }

    std::printf("\n%d passed, %d failed\n", pass, fail);
    return fail == 0 ? 0 : 1;
}
