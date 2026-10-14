#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "../engine/DrumArchetypes.hpp"
#include "../engine/RhythmGenerator.hpp"
#include "../engine/RhythmSieve.hpp"

namespace {
int failures = 0;

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

int count(const std::array<bool, 16> &pattern) {
    return static_cast<int>(std::count(pattern.begin(), pattern.end(), true));
}

int count(const std::vector<bool> &pattern) {
    return static_cast<int>(std::count(pattern.begin(), pattern.end(), true));
}

std::string signature(const rasgo::DrumArchetype &archetype) {
    std::string out;
    for (const auto *pattern : {&archetype.kick, &archetype.snare, &archetype.hihat}) {
        for (bool onset : *pattern) out += onset ? '1' : '0';
    }
    return out;
}
}

int main() {
    using namespace rasgo;

    const auto &catalog = drumArchetypes();
    std::set<std::string> signatures;
    for (const auto &archetype : catalog) {
        check(count(archetype.kick) > 0, "every named archetype has a kick voice");
        check(count(archetype.snare) > 0, "every named archetype has a snare voice");
        check(count(archetype.kick) <= 16 && count(archetype.snare) <= 16
                  && count(archetype.hihat) <= 16,
              "named archetype onset counts stay within one bar");
        check(archetype.swingAmount >= 0.0 && archetype.swingAmount < 1.0,
              "swing amount stays within one step");
        check(signatures.insert(signature(archetype)).second,
              "named archetypes have distinct kick/snare/hat structures");
    }
    check(catalog.size() == 7, "catalog contains the seven documented archetypes");
    check(catalog[3].swingAmount > 0.0,
          "Jazz archetype carries an actual swing delay");
    for (std::size_t i = 0; i < catalog.size(); ++i) {
        if (i != 3) check(catalog[i].swingAmount == 0.0,
                           "non-Jazz archetypes remain straight");
    }

    std::mt19937 rng(20260821u);
    for (int trial = 0; trial < 256; ++trial) {
        const auto sieve = generateRhythmSieve(rng);
        check(sieve.kick.size() == 16 && sieve.snare.size() == 16,
              "generated sieve has the default bar length");
        check(sieve.kick.front(), "generated sieve preserves the downbeat");
        check(count(sieve.kick) >= 4 && count(sieve.kick) <= 10,
              "generated sieve kick remains neither skeletal nor overcrowded");
        check(sieve.kick != sieve.snare,
              "generated sieve does not double kick and snare exactly");

        const auto aksak = generateAksakPattern(rng);
        check(aksak.groupStart.size() == 16 && aksak.longAccent.size() == 16,
              "generated aksak has the default bar length");
        check(aksak.groupStart.front(), "generated aksak starts on the downbeat");
        int position = 0;
        int starts = 0;
        for (int step = 1; step < 16; ++step) {
            if (!aksak.groupStart[static_cast<std::size_t>(step)]) continue;
            const int gap = step - position;
            check(gap == 2 || gap == 3, "aksak groups have long/short real durations");
            position = step;
            ++starts;
        }
        check(16 - position == 2 || 16 - position == 3,
              "aksak closes with a long or short group");
        check(starts >= 3, "aksak contains multiple independent groups");
        for (int step = 0; step < 16; ++step)
            check(!aksak.longAccent[static_cast<std::size_t>(step)]
                      || aksak.groupStart[static_cast<std::size_t>(step)],
                  "aksak long accents occur only at group starts");

        const auto euclid = generateEuclideanPattern(rng, 2, 11);
        check(euclid.onset.size() == 16,
              "generated Euclidean rhythm has the default bar length");
        check(count(euclid.onset) == euclid.pulses,
              "Euclidean metadata matches its actual onset count");
        check(euclid.pulses >= 2 && euclid.pulses <= 11,
              "Euclidean density stays in the requested range");
    }

    for (int step = 0; step < 16; ++step) {
        check(metricalLevel(step) >= 0 && metricalLevel(step) <= 4,
              "default metrical hierarchy stays bounded");
        check(hitVelocity(step, -1.0, 0.0) >= 0.35
                  && hitVelocity(step, 1.0, 1.0) <= 1.35,
              "per-hit dynamics stay within documented safety bounds");
    }

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "Drum rhythm contract tests passed\n";
    return EXIT_SUCCESS;
}
