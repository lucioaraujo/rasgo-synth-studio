#include <array>
#include <cstdio>
#include <set>
#include <string>

#include "../engine/CompositionIdentity.hpp"
#include "../sequencer/StepSequencer.hpp"

int main() {
    int passed = 0, failed = 0;
    auto check = [&](bool condition, const char *name) {
        if (condition) ++passed;
        else { ++failed; std::printf("FAIL: %s\n", name); }
    };

    std::array<int, 6> focusCounts{};
    std::array<int, 6> familyCounts{};
    std::array<int, 5> voiceCounts{};
    std::array<int, 4> rhythmCounts{};
    std::array<int, 5> leadCounts{};
    std::array<int, 5> harmonyCounts{};
    std::array<int, 4> ensembleCounts{};
    std::array<bool, 12> pitchClasses{};
    int spectralPresence = 0;
    bool allBudgetsValid = true;
    bool allRootsSafe = true;

    constexpr unsigned kSeeds = 4096;
    for (unsigned seed = 0; seed < kSeeds; ++seed) {
        auto value = rasgo::makeCompositionIdentity(seed);
        ++focusCounts[static_cast<std::size_t>(value.focus)];
        ++familyCounts[static_cast<std::size_t>(value.scaleFamily)];
        ++voiceCounts[static_cast<std::size_t>(value.coreVoice)];
        ++rhythmCounts[static_cast<std::size_t>(value.rhythmModel)];
        ++leadCounts[static_cast<std::size_t>(value.leadVoice)];
        ++harmonyCounts[static_cast<std::size_t>(value.harmonicLanguage)];
        ++ensembleCounts[static_cast<std::size_t>(value.ensembleModel)];
        pitchClasses[static_cast<std::size_t>(value.rootMidiNote % 12)] = true;
        if (value.spectral.count() > 0) ++spectralPresence;
        if (value.spectral.count() > 2) allBudgetsValid = false;
        if (value.focus == rasgo::MaterialFocus::Spectral && value.spectral.count() != 2)
            allBudgetsValid = false;
        if (value.rootMidiNote < 43 || value.rootMidiNote > 67) allRootsSafe = false;

        auto repeat = rasgo::makeCompositionIdentity(seed);
        if (repeat.rootMidiNote != value.rootMidiNote
            || repeat.initialScale != value.initialScale
            || repeat.focus != value.focus || repeat.coreVoice != value.coreVoice
            || repeat.spectral.count() != value.spectral.count()) {
            allBudgetsValid = false;
        }
    }

    auto everySeen = [](const auto &counts, int minimum) {
        for (int count : counts) if (count < minimum) return false;
        return true;
    };
    bool everyPitchClass = true;
    for (bool seen : pitchClasses) if (!seen) everyPitchClass = false;

    check(everySeen(focusCounts, 300), "all material focuses have substantial coverage");
    check(everySeen(familyCounts, 120), "scale families are selected before catalog names");
    check(everySeen(voiceCounts, 500), "all core voice topologies have coverage");
    check(everySeen(rhythmCounts, 700), "all sequence rhythm models have coverage");
    check(everySeen(leadCounts, 350), "all lead voice roles have coverage");
    check(everySeen(harmonyCounts, 500), "all harmonic languages have coverage");
    check(everySeen(ensembleCounts, 500), "all ensemble organizations have coverage");
    check(everyPitchClass, "seed-derived roots cover all twelve pitch classes");
    check(allRootsSafe, "root register stays in the declared safe range");
    check(allBudgetsValid, "spectral family budget and determinism hold");
    check(spectralPresence > 1000 && spectralPresence < 1800,
          "spectral techniques are identities, neither absent nor near-universal");

    // The four gate generators must each remain valid. Their exact pattern is
    // intentionally not golden-filed: the contract is structural diversity,
    // not freezing one implementation forever.
    std::set<std::string> patterns;
    for (int model = 0; model < 4; ++model) {
        rasgo::StepSequencer sequencer;
        sequencer.configure(16, rasgo::Scale::Dorian, 52, 2026,
            static_cast<rasgo::StepSequencer::RhythmModel>(model), 0.0,
            rasgo::StepSequencer::HarmonicModel::Modal);
        std::string pattern;
        int active = 0;
        for (int i = 0; i < 16; ++i) {
            bool gate = sequencer.gateAt(i);
            pattern.push_back(gate ? 'x' : '.');
            if (gate) ++active;
        }
        check(active > 0 && active < 16, "rhythm model produces a non-degenerate gate");
        patterns.insert(pattern);
    }
    check(patterns.size() >= 3, "rhythm models produce structurally distinct patterns");

    std::printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
