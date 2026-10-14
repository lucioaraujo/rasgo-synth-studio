#include <array>
#include <cstdio>
#include <set>
#include <string>

#include "../sequencer/Scales.hpp"
#include "../sequencer/SieveScale.hpp"
#include "../sequencer/StepSequencer.hpp"
#include "../engine/CompositionIdentity.hpp"

int main() {
    int passed = 0;
    int failed = 0;
    auto check = [&](bool condition, const char *name) {
        if (condition) ++passed;
        else { ++failed; std::printf("FAIL: %s\n", name); }
    };

    std::set<std::set<int>> pitchSets;
    std::set<std::string> names;
    bool tablesValid = true;
    for (int i = 0; i < static_cast<int>(rasgo::Scale::Count); ++i) {
        auto scale = static_cast<rasgo::Scale>(i);
        const auto &table = rasgo::scaleTable(scale);
        const std::string name = rasgo::scaleName(scale);
        names.insert(name);
        if (name.empty() || table.length < 4 || table.length > 12
            || table.degrees[0] != 0) {
            tablesValid = false;
        }
        std::set<int> pcs;
        for (int degree = 0; degree < table.length; ++degree) {
            if (table.degrees[degree] < 0 || table.degrees[degree] > 11)
                tablesValid = false;
            pcs.insert(table.degrees[degree]);
        }
        if (static_cast<int>(pcs.size()) != table.length) tablesValid = false;
        pitchSets.insert(pcs);
    }
    check(static_cast<int>(rasgo::Scale::Count) == 45,
          "fixed catalog contains 45 named scales");
    check(static_cast<int>(names.size()) == static_cast<int>(rasgo::Scale::Count),
          "every fixed scale has a unique name");
    check(static_cast<int>(pitchSets.size()) == static_cast<int>(rasgo::Scale::Count),
          "fixed scales have unique pitch-class collections");
    check(tablesValid, "fixed scale tables have valid ordered degrees");

    std::set<std::set<int>> catalog = rasgo::cataloguedPitchClassSets();
    bool generatedOutsideCatalog = true;
    std::mt19937 rng(20260821u);
    for (int i = 0; i < 2000; ++i) {
        const auto generated = rasgo::generateSieveScale(rng, catalog);
        std::set<int> pcs;
        for (int degree = 0; degree < generated.table.length; ++degree)
            pcs.insert(generated.table.degrees[degree]);
        if (catalog.count(pcs) != 0) generatedOutsideCatalog = false;
    }
    check(generatedOutsideCatalog,
          "generated sieve collections remain outside the fixed catalog");

    std::array<int, static_cast<std::size_t>(rasgo::Scale::Count)> seen{};
    for (unsigned seed = 0; seed < 4096; ++seed)
        ++seen[static_cast<std::size_t>(rasgo::makeCompositionIdentity(seed).initialScale)];
    bool everyScaleReachable = true;
    for (int count : seen) if (count == 0) everyScaleReachable = false;
    check(everyScaleReachable,
          "every fixed scale is reachable from the identity seed selection");

    bool everyScaleRegeneratesValidNotes = true;
    bool everyScaleProvidesValidChordOffsets = true;
    for (int scaleIndex = 0; scaleIndex < static_cast<int>(rasgo::Scale::Count); ++scaleIndex) {
        const auto scale = static_cast<rasgo::Scale>(scaleIndex);
        const auto &table = rasgo::scaleTable(scale);
        rasgo::StepSequencer generated;
        generated.configure(16, scale, 60, 40000u + static_cast<unsigned>(scaleIndex),
            rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
            rasgo::StepSequencer::HarmonicModel::Modal);
        const auto offsets = generated.currentChordOffsets();
        for (int offset : offsets) {
            bool found = false;
            for (int degree = 0; degree < table.length; ++degree)
                if (table.degrees[degree] == offset) found = true;
            if (!found) everyScaleProvidesValidChordOffsets = false;
        }
        for (int bar = 0; bar < 3; ++bar) {
            generated.regenerate(0.7);
            for (int step = 0; step < 16; ++step) {
                const int rawOffset = generated.noteAt(step) - 60;
                const int offset = (rawOffset % 12 + 12) % 12;
                bool found = false;
                for (int degree = 0; degree < table.length; ++degree)
                    if (table.degrees[degree] == offset) found = true;
                if (!found) everyScaleRegeneratesValidNotes = false;
            }
        }
    }
    check(everyScaleRegeneratesValidNotes,
          "all fixed scales regenerate notes inside their own collection");
    check(everyScaleProvidesValidChordOffsets,
          "all fixed scales provide chord offsets inside their own collection");

    bool everyScaleSupportsVoiceLeading = true;
    bool everyScaleSupportsProgressionOffsets = true;
    for (int scaleIndex = 0; scaleIndex < static_cast<int>(rasgo::Scale::Count); ++scaleIndex) {
        const auto scale = static_cast<rasgo::Scale>(scaleIndex);
        const auto &table = rasgo::scaleTable(scale);
        rasgo::StepSequencer harmonic;
        harmonic.configure(16, scale, 60, 50000u + static_cast<unsigned>(scaleIndex),
            rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
            rasgo::StepSequencer::HarmonicModel::Jazz);
        for (int bar = 0; bar < 5; ++bar) {
            harmonic.regenerate(0.72);
            for (int step = 0; step < 16; ++step) {
                const int rawOffset = harmonic.noteAt(step) - 60;
                const int offset = (rawOffset % 12 + 12) % 12;
                bool found = false;
                for (int degree = 0; degree < table.length; ++degree)
                    if (table.degrees[degree] == offset) found = true;
                if (!found) everyScaleSupportsVoiceLeading = false;
            }
            harmonic.advanceChord();
            if (!harmonic.neoRiemannianEnabled()) {
                for (int offset : harmonic.currentChordOffsets()) {
                    bool found = false;
                    for (int degree = 0; degree < table.length; ++degree)
                        if (table.degrees[degree] == offset) found = true;
                    if (!found) everyScaleSupportsProgressionOffsets = false;
                }
            }
        }
    }
    check(everyScaleSupportsVoiceLeading,
          "all fixed scales support absolute voice-leading notes");
    check(everyScaleSupportsProgressionOffsets,
          "all fixed scales support progression chord offsets");

    bool everyHarmonicModelKeepsScaleNotes = true;
    const std::array<rasgo::StepSequencer::HarmonicModel, 5> harmonicModels = {
        rasgo::StepSequencer::HarmonicModel::Modal,
        rasgo::StepSequencer::HarmonicModel::Functional,
        rasgo::StepSequencer::HarmonicModel::Jazz,
        rasgo::StepSequencer::HarmonicModel::Chromatic,
        rasgo::StepSequencer::HarmonicModel::Drone
    };
    for (std::size_t modelIndex = 0; modelIndex < harmonicModels.size(); ++modelIndex) {
        for (int scaleIndex = 0; scaleIndex < static_cast<int>(rasgo::Scale::Count); ++scaleIndex) {
            const auto scale = static_cast<rasgo::Scale>(scaleIndex);
            const auto &table = rasgo::scaleTable(scale);
            rasgo::StepSequencer modelSequencer;
            modelSequencer.configure(16, scale, 60,
                60000u + static_cast<unsigned>(modelIndex * 100 + scaleIndex),
                rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
                harmonicModels[modelIndex]);
            for (int bar = 0; bar < 3; ++bar) {
                modelSequencer.regenerate(0.68);
                for (int step = 0; step < 16; ++step) {
                    const int rawOffset = modelSequencer.noteAt(step) - 60;
                    const int offset = (rawOffset % 12 + 12) % 12;
                    bool found = false;
                    for (int degree = 0; degree < table.length; ++degree)
                        if (table.degrees[degree] == offset) found = true;
                    if (!found) everyHarmonicModelKeepsScaleNotes = false;
                }
                modelSequencer.advanceChord();
            }
        }
    }
    check(everyHarmonicModelKeepsScaleNotes,
          "all harmonic models keep fixed-scale notes inside their collection");

    bool modulatedCatalogNotesValid = true;
    for (int scaleIndex = 0; scaleIndex < static_cast<int>(rasgo::Scale::Count); ++scaleIndex) {
        const auto initial = static_cast<rasgo::Scale>(scaleIndex);
        const auto target = static_cast<rasgo::Scale>((scaleIndex + 11)
            % static_cast<int>(rasgo::Scale::Count));
        const auto &targetTable = rasgo::scaleTable(target);
        rasgo::StepSequencer modulated;
        modulated.configure(16, initial, 60, 80000u + static_cast<unsigned>(scaleIndex),
            rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
            rasgo::StepSequencer::HarmonicModel::Functional);
        modulated.applyGameCharacter(1.0, 0.0, 0.0);
        modulated.modulateTo(62, target);
        modulated.regenerate(0.7);
        for (int step = 0; step < 16; ++step) {
            const int rawOffset = modulated.noteAt(step) - 62;
            const int offset = (rawOffset % 12 + 12) % 12;
            bool found = false;
            for (int degree = 0; degree < targetTable.length; ++degree)
                if (targetTable.degrees[degree] == offset) found = true;
            if (!found) modulatedCatalogNotesValid = false;
        }
    }
    check(modulatedCatalogNotesValid,
          "catalog modulation does not retain notes from the former scale");

    rasgo::StepSequencer unchangedModulation;
    unchangedModulation.configure(16, rasgo::Scale::Major, 60, 81000u,
        rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
        rasgo::StepSequencer::HarmonicModel::Modal);
    std::array<int, 16> beforeNoOp{};
    std::array<bool, 16> gatesBeforeNoOp{};
    for (int step = 0; step < 16; ++step) {
        beforeNoOp[step] = unchangedModulation.noteAt(step);
        gatesBeforeNoOp[step] = unchangedModulation.gateAt(step);
    }
    unchangedModulation.modulateTo(60, rasgo::Scale::Major);
    bool noOpPreservesPattern = true;
    for (int step = 0; step < 16; ++step) {
        if (beforeNoOp[step] != unchangedModulation.noteAt(step)
            || gatesBeforeNoOp[step] != unchangedModulation.gateAt(step))
            noOpPreservesPattern = false;
    }
    check(noOpPreservesPattern,
          "unchanged modulation does not reset the current pattern");

    rasgo::StepSequencer rootOnlyModulation;
    rootOnlyModulation.configure(16, rasgo::Scale::Major, 60, 82000u,
        rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
        rasgo::StepSequencer::HarmonicModel::Modal);
    rootOnlyModulation.applyGameCharacter(1.0, 0.0, 0.0);
    rootOnlyModulation.modulateTo(65, rasgo::Scale::Major);
    rootOnlyModulation.regenerate(0.7);
    bool rootOnlyNotesValid = true;
    for (int step = 0; step < 16; ++step) {
        const int rawOffset = rootOnlyModulation.noteAt(step) - 65;
        const int offset = (rawOffset % 12 + 12) % 12;
        const auto &major = rasgo::scaleTable(rasgo::Scale::Major);
        bool found = false;
        for (int degree = 0; degree < major.length; ++degree)
            if (major.degrees[degree] == offset) found = true;
        if (!found) rootOnlyNotesValid = false;
    }
    check(rootOnlyNotesValid,
          "root-only modulation rebuilds notes around the new tonal centre");

    rasgo::StepSequencer modulatedA;
    rasgo::StepSequencer modulatedB;
    modulatedA.configure(16, rasgo::Scale::Major, 60, 83000u,
        rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
        rasgo::StepSequencer::HarmonicModel::Functional);
    modulatedB.configure(16, rasgo::Scale::Major, 60, 83000u,
        rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
        rasgo::StepSequencer::HarmonicModel::Functional);
    modulatedA.modulateTo(67, rasgo::Scale::Lydian);
    modulatedB.modulateTo(67, rasgo::Scale::Lydian);
    bool modulatedDeterministic = true;
    for (int bar = 0; bar < 3; ++bar) {
        modulatedA.regenerate(0.68, 0.2);
        modulatedB.regenerate(0.68, 0.2);
        for (int step = 0; step < 16; ++step) {
            if (modulatedA.noteAt(step) != modulatedB.noteAt(step)
                || modulatedA.gateAt(step) != modulatedB.gateAt(step))
                modulatedDeterministic = false;
        }
    }
    check(modulatedDeterministic,
          "same-seed modulation remains deterministic after the reset boundary");

    bool secondaryCanBecomePrimary = false;
    for (unsigned seed = 1; seed < 256 && !secondaryCanBecomePrimary; ++seed) {
        rasgo::StepSequencer blended;
        blended.configure(16, rasgo::Scale::Dorian, 60, seed,
            rasgo::StepSequencer::RhythmModel::Euclidean, 1.0,
            rasgo::StepSequencer::HarmonicModel::Modal);
        const std::string description = blended.scaleDescription();
        const std::size_t plus = description.find('+');
        const std::size_t end = description.rfind('(');
        if (plus == std::string::npos || end <= plus + 1) continue;
        const std::string secondaryName = description.substr(plus + 1, end - plus - 1);
        for (int scale = 0; scale < static_cast<int>(rasgo::Scale::Count); ++scale) {
            const auto candidate = static_cast<rasgo::Scale>(scale);
            if (secondaryName != rasgo::scaleName(candidate)) continue;
            blended.modulateTo(60, candidate);
            secondaryCanBecomePrimary = blended.scaleDescription().find('+') == std::string::npos;
            break;
        }
    }
    check(secondaryCanBecomePrimary,
          "modulating to the secondary scale removes redundant self-blending");

    bool everyBlendedHarmonicModelKeepsUnion = true;
    int blendedModelPairs = 0;
    for (std::size_t modelIndex = 0; modelIndex < harmonicModels.size(); ++modelIndex) {
        for (unsigned seed = 1; seed <= 128; ++seed) {
            rasgo::StepSequencer blended;
            blended.configure(16, rasgo::Scale::Dorian, 60,
                70000u + static_cast<unsigned>(modelIndex * 1000) + seed,
                rasgo::StepSequencer::RhythmModel::Euclidean, 1.0,
                harmonicModels[modelIndex]);
            const std::string description = blended.scaleDescription();
            const std::size_t plus = description.find('+');
            const std::size_t end = description.rfind('(');
            if (plus == std::string::npos || end <= plus + 1) {
                everyBlendedHarmonicModelKeepsUnion = false;
                continue;
            }
            ++blendedModelPairs;
            const std::string secondaryName = description.substr(plus + 1, end - plus - 1);
            const rasgo::ScaleTable *secondaryTable = nullptr;
            for (int scale = 0; scale < static_cast<int>(rasgo::Scale::Count); ++scale) {
                const auto candidate = static_cast<rasgo::Scale>(scale);
                if (secondaryName == rasgo::scaleName(candidate)) {
                    secondaryTable = &rasgo::scaleTable(candidate);
                    break;
                }
            }
            if (secondaryTable == nullptr) {
                everyBlendedHarmonicModelKeepsUnion = false;
                continue;
            }
            const auto &primaryTable = rasgo::scaleTable(rasgo::Scale::Dorian);
            for (int bar = 0; bar < 2; ++bar) {
                blended.regenerate(0.7);
                for (int step = 0; step < 16; ++step) {
                    const int rawOffset = blended.noteAt(step) - 60;
                    const int offset = (rawOffset % 12 + 12) % 12;
                    bool allowed = false;
                    for (int degree = 0; degree < primaryTable.length; ++degree)
                        if (primaryTable.degrees[degree] == offset) allowed = true;
                    for (int degree = 0; degree < secondaryTable->length; ++degree)
                        if (secondaryTable->degrees[degree] == offset) allowed = true;
                    if (!allowed) everyBlendedHarmonicModelKeepsUnion = false;
                }
                blended.advanceChord();
            }
        }
    }
    check(blendedModelPairs == 640 && everyBlendedHarmonicModelKeepsUnion,
          "all harmonic models keep blended notes inside the allowed union");

    rasgo::StepSequencer sequencer;
    sequencer.configure(16, rasgo::Scale::Major, 60, 17,
        rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
        rasgo::StepSequencer::HarmonicModel::Modal);
    const rasgo::ScaleTable custom{{0, 1, 4, 6, 9, 10, 10, 10, 10, 10, 10, 10}, 6};
    sequencer.setCustomPrimaryScale(custom, "test sieve");
    const auto offsets = sequencer.currentChordOffsets();
    check(offsets[0] == 0 && offsets[1] == 4 && offsets[2] == 9,
          "chord offsets follow the active custom primary collection");
    check(sequencer.scaleDescription().find("test sieve") != std::string::npos,
          "custom primary description remains visible in sequencer metadata");
    sequencer.modulateTo(62, rasgo::Scale::Locrian);
    check(sequencer.rootMidiNote() == 62
              && sequencer.scaleDescription().find("test sieve") != std::string::npos,
          "custom collection description survives tonal modulation");
    const auto modulatedOffsets = sequencer.currentChordOffsets();
    check(modulatedOffsets[0] == 0 && modulatedOffsets[1] == 4
              && modulatedOffsets[2] == 9,
          "custom chord offsets remain relative to the new tonal centre");

    rasgo::StepSequencer customJazz;
    customJazz.configure(16, rasgo::Scale::Major, 60, 17017u,
        rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
        rasgo::StepSequencer::HarmonicModel::Jazz);
    customJazz.setCustomPrimaryScale(custom, "test sieve jazz");
    bool customJazzNotesValid = true;
    for (int bar = 0; bar < 4; ++bar) {
        customJazz.regenerate(0.72);
        for (int step = 0; step < 16; ++step) {
            const int rawOffset = customJazz.noteAt(step) - 60;
            const int offset = (rawOffset % 12 + 12) % 12;
            bool found = false;
            for (int degree = 0; degree < custom.length; ++degree)
                if (custom.degrees[degree] == offset) found = true;
            if (!found) customJazzNotesValid = false;
        }
        customJazz.advanceChord();
    }
    check(customJazzNotesValid,
          "generated primary collection supports Jazz voice-leading");

    rasgo::StepSequencer customRootOnly;
    customRootOnly.configure(16, rasgo::Scale::Major, 60, 17019u,
        rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
        rasgo::StepSequencer::HarmonicModel::Modal);
    customRootOnly.setCustomPrimaryScale(custom, "test sieve root-only");
    customRootOnly.regenerate(0.72);
    customRootOnly.modulateTo(65, rasgo::Scale::Major);
    customRootOnly.regenerate(0.72);
    bool customRootOnlyValid = true;
    for (int step = 0; step < 16; ++step) {
        const int rawOffset = customRootOnly.noteAt(step) - 65;
        const int offset = (rawOffset % 12 + 12) % 12;
        bool found = false;
        for (int degree = 0; degree < custom.length; ++degree)
            if (custom.degrees[degree] == offset) found = true;
        if (!found) customRootOnlyValid = false;
    }
    check(customRootOnlyValid,
          "root-only modulation preserves the custom primary collection");

    bool customAdvancedModelsValid = true;
    for (const auto model : {rasgo::StepSequencer::HarmonicModel::Jazz,
                             rasgo::StepSequencer::HarmonicModel::Chromatic}) {
        rasgo::StepSequencer customModel;
        customModel.configure(16, rasgo::Scale::Major, 60, 18018u,
            rasgo::StepSequencer::RhythmModel::Euclidean, 0.0, model);
        customModel.setCustomPrimaryScale(custom, "test sieve advanced");
        for (int bar = 0; bar < 3; ++bar) {
            customModel.regenerate(0.72);
            for (int step = 0; step < 16; ++step) {
                const int rawOffset = customModel.noteAt(step) - 60;
                const int offset = (rawOffset % 12 + 12) % 12;
                bool found = false;
                for (int degree = 0; degree < custom.length; ++degree)
                    if (custom.degrees[degree] == offset) found = true;
                if (!found) customAdvancedModelsValid = false;
            }
            customModel.advanceChord();
        }
    }
    check(customAdvancedModelsValid,
          "generated collection supports Jazz and Chromatic voice-leading");

    bool neoRiemannianIntervalsValid = true;
    int neoPieces = 0;
    for (unsigned seed = 19000u; seed < 20000u && neoPieces < 20; ++seed) {
        const int scaleIndex = static_cast<int>(seed % static_cast<unsigned>(rasgo::Scale::Count));
        rasgo::StepSequencer neo;
        neo.configure(16, static_cast<rasgo::Scale>(scaleIndex), 60, seed,
            rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
            rasgo::StepSequencer::HarmonicModel::Chromatic);
        if (!neo.neoRiemannianEnabled()) continue;
        ++neoPieces;
        for (int operation = 0; operation < 4; ++operation) {
            neo.advanceChord();
            const auto offsets = neo.currentChordOffsets();
            const auto interval = [](int a, int b) {
                return (b - a) % 12 < 0 ? (b - a) % 12 + 12 : (b - a) % 12;
            };
            const int third = interval(offsets[0], offsets[1]);
            const int fifth = interval(offsets[0], offsets[2]);
            const int seventh = interval(offsets[0], offsets[3]);
            if (!((third == 3) || (third == 4)) || fifth != 7
                || !((seventh == 10) || (seventh == 11))) {
                neoRiemannianIntervalsValid = false;
            }
        }
    }
    check(neoPieces > 0 && neoRiemannianIntervalsValid,
          "Neo-Riemannian chord offsets retain their chromatic interval language");

    bool functionalProgressionCanRest = false;
    bool functionalProgressionCanMove = false;
    for (unsigned seed = 30000u; seed < 30256u; ++seed) {
        rasgo::StepSequencer functional;
        functional.configure(16, rasgo::Scale::Major, 60, seed,
            rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
            rasgo::StepSequencer::HarmonicModel::Functional);
        if (functional.chordProgressionEnabled()) functionalProgressionCanMove = true;
        else functionalProgressionCanRest = true;
    }
    check(functionalProgressionCanRest && functionalProgressionCanMove,
          "Functional harmony retains both progression and progression-free trajectories");

    bool jazzAlwaysProgresses = true;
    for (unsigned seed = 31000u; seed < 31064u; ++seed) {
        rasgo::StepSequencer jazz;
        jazz.configure(16, rasgo::Scale::Major, 60, seed,
            rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
            rasgo::StepSequencer::HarmonicModel::Jazz);
        if (!jazz.chordProgressionEnabled()) jazzAlwaysProgresses = false;
    }
    check(jazzAlwaysProgresses, "Jazz harmony keeps its moving chord bed");

    std::printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
