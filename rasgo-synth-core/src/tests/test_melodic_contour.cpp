#include <cmath>
#include <cstdio>
#include <array>
#include <set>
#include <string>

#include "../sequencer/MelodicContourMemory.hpp"
#include "../sequencer/StepSequencer.hpp"

int main() {
    int passed = 0;
    int failed = 0;
    auto check = [&](bool condition, const char *name) {
        if (condition) ++passed;
        else { ++failed; std::printf("FAIL: %s\n", name); }
    };

    rasgo::MelodicContourMemory first;
    rasgo::MelodicContourMemory second;
    first.configure(20260821u, 4);
    second.configure(20260821u, 4);
    first.beginPhrase();
    second.beginPhrase();
    check(first.direction() == second.direction()
              && first.apexBar() == second.apexBar(),
          "contour phrase decisions are deterministic");

    bool bounded = true;
    bool directionallyConsistent = true;
    const int direction = first.direction();
    for (int bar = 0; bar < 4; ++bar) {
        const double upward = first.pitchWeight(7, 3, bar);
        const double downward = first.pitchWeight(1, 3, bar);
        if (upward < 0.8 || upward > 1.35 || downward < 0.8 || downward > 1.35)
            bounded = false;
        if (bar != first.apexBar() && direction != 0) {
            const int desired = bar < first.apexBar() ? direction : -direction;
            if (desired > 0 && upward <= downward) directionallyConsistent = false;
            if (desired < 0 && downward <= upward) directionallyConsistent = false;
        }
    }
    check(bounded, "contour weighting remains bounded");
    check(directionallyConsistent, "contour preference turns around the apex");

    int upwardPhrases = 0;
    int downwardPhrases = 0;
    for (std::uint32_t seed = 1; seed <= 4096; ++seed) {
        rasgo::MelodicContourMemory memory;
        memory.configure(seed, 4);
        memory.beginPhrase();
        if (memory.direction() > 0) ++upwardPhrases;
        if (memory.direction() < 0) ++downwardPhrases;
    }
    check(upwardPhrases > 900 && downwardPhrases > 900,
          "contour seeds do not collapse into one direction");

    rasgo::MelodicContourMemory phraseCycle;
    phraseCycle.configure(5150u, 4);
    phraseCycle.beginPhrase();
    for (int bar = 0; bar < 4; ++bar) {
        phraseCycle.beginBar();
        phraseCycle.finishBar();
    }
    check(phraseCycle.phraseBar() == 4,
          "contour records completion of a full phrase");
    phraseCycle.beginBar();
    check(phraseCycle.phraseBar() == 0
              && phraseCycle.apexBar() >= 1 && phraseCycle.apexBar() < 4
              && (phraseCycle.direction() == -1 || phraseCycle.direction() == 1),
          "contour starts a valid new phrase after rollover");

    rasgo::StepSequencer deterministicA;
    rasgo::StepSequencer deterministicB;
    deterministicA.configure(16, rasgo::Scale::Dorian, 60, 8080u,
        rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
        rasgo::StepSequencer::HarmonicModel::Modal);
    deterministicB.configure(16, rasgo::Scale::Dorian, 60, 8080u,
        rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
        rasgo::StepSequencer::HarmonicModel::Modal);
    bool deterministicSequence = true;
    for (int bar = 0; bar < 4; ++bar) {
        deterministicA.regenerate(0.65, 0.15);
        deterministicB.regenerate(0.65, 0.15);
        for (int step = 0; step < 16; ++step) {
            if (deterministicA.noteAt(step) != deterministicB.noteAt(step)
                || deterministicA.gateAt(step) != deterministicB.gateAt(step))
                deterministicSequence = false;
        }
    }
    check(deterministicSequence,
          "contour integration preserves same-seed sequence determinism");

    std::set<std::array<int, 16>> trajectories;
    bool directionsBalanced = true;
    int contourUp = 0;
    int contourDown = 0;
    for (unsigned seed = 1; seed <= 512; ++seed) {
        rasgo::StepSequencer sequence;
        sequence.configure(16, rasgo::Scale::Dorian, 60, seed,
            rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
            rasgo::StepSequencer::HarmonicModel::Modal);
        if (sequence.melodicContourDirection() > 0) ++contourUp;
        if (sequence.melodicContourDirection() < 0) ++contourDown;
        std::array<int, 16> firstBar{};
        for (int step = 0; step < 16; ++step) firstBar[step] = sequence.noteAt(step);
        trajectories.insert(firstBar);
        for (int bar = 0; bar < 3; ++bar) sequence.regenerate(0.65);
    }
    directionsBalanced = contourUp > 100 && contourDown > 100;
    check(trajectories.size() > 400,
          "contour-enabled sequencer keeps broad seed trajectory diversity");
    check(directionsBalanced,
          "integrated contour directions remain statistically balanced");

    bool blendedCollectionsStayValid = true;
    int blendedPieces = 0;
    for (unsigned seed = 1; seed <= 256; ++seed) {
        rasgo::StepSequencer blended;
        blended.configure(16, rasgo::Scale::Dorian, 60, seed,
            rasgo::StepSequencer::RhythmModel::Euclidean, 1.0,
            rasgo::StepSequencer::HarmonicModel::Modal);
        const std::string description = blended.scaleDescription();
        const std::size_t plus = description.find('+');
        if (plus == std::string::npos) continue;
        ++blendedPieces;
        const std::size_t end = description.rfind('(');
        const std::string secondaryName = description.substr(
            plus + 1, end == std::string::npos ? std::string::npos : end - plus - 1);
        const rasgo::ScaleTable *secondaryTable = nullptr;
        for (int scale = 0; scale < static_cast<int>(rasgo::Scale::Count); ++scale) {
            const auto candidate = static_cast<rasgo::Scale>(scale);
            if (secondaryName == rasgo::scaleName(candidate)) {
                secondaryTable = &rasgo::scaleTable(candidate);
                break;
            }
        }
        if (secondaryTable == nullptr) {
            blendedCollectionsStayValid = false;
            continue;
        }
        for (int bar = 0; bar < 3; ++bar) {
            blended.regenerate(0.7);
            for (int step = 0; step < 16; ++step) {
                const int rawOffset = blended.noteAt(step) - 60;
                const int offset = (rawOffset % 12 + 12) % 12;
                bool inPrimary = false;
                bool inSecondary = false;
                for (int degree = 0; degree < rasgo::scaleTable(rasgo::Scale::Dorian).length; ++degree)
                    if (rasgo::scaleTable(rasgo::Scale::Dorian).degrees[degree] == offset) inPrimary = true;
                for (int degree = 0; degree < secondaryTable->length; ++degree)
                    if (secondaryTable->degrees[degree] == offset) inSecondary = true;
                if (!inPrimary && !inSecondary) blendedCollectionsStayValid = false;
            }
        }
    }
    check(blendedPieces > 200 && blendedCollectionsStayValid,
          "contour remains inside the union of primary and secondary collections");

    const rasgo::ScaleTable custom{{0, 1, 4, 6, 9, 10, 10, 10, 10, 10, 10, 10}, 6};
    rasgo::StepSequencer generatedCollection;
    generatedCollection.configure(16, rasgo::Scale::Major, 60, 9191u,
        rasgo::StepSequencer::RhythmModel::Euclidean, 0.0,
        rasgo::StepSequencer::HarmonicModel::Modal);
    generatedCollection.setCustomPrimaryScale(custom, "test generated collection");
    bool staysInCustomCollection = true;
    for (int bar = 0; bar < 5; ++bar) {
        generatedCollection.regenerate(0.65);
        for (int step = 0; step < 16; ++step) {
            const int rawOffset = generatedCollection.noteAt(step) - 60;
            const int offset = (rawOffset % 12 + 12) % 12;
            bool found = false;
            for (int degree = 0; degree < custom.length; ++degree)
                if (custom.degrees[degree] == offset) found = true;
            if (!found) staysInCustomCollection = false;
        }
    }
    check(staysInCustomCollection,
          "contour does not push notes outside a generated primary collection");

    const int contourDirectionBeforeModulation = generatedCollection.melodicContourDirection();
    const int contourApexBeforeModulation = generatedCollection.melodicContourApexBar();
    generatedCollection.modulateTo(62, rasgo::Scale::Locrian);
    check(generatedCollection.melodicContourDirection() == contourDirectionBeforeModulation
              && generatedCollection.melodicContourApexBar() == contourApexBeforeModulation,
          "tonal changes preserve current phrase contour identity");
    bool customCollectionSurvivesModulation = true;
    for (int bar = 0; bar < 3; ++bar) {
        generatedCollection.regenerate(0.65);
        for (int step = 0; step < 16; ++step) {
            const int rawOffset = generatedCollection.noteAt(step) - 62;
            const int offset = (rawOffset % 12 + 12) % 12;
            bool found = false;
            for (int degree = 0; degree < custom.length; ++degree)
                if (custom.degrees[degree] == offset) found = true;
            if (!found) customCollectionSurvivesModulation = false;
        }
    }
    check(generatedCollection.rootMidiNote() == 62,
          "modulation moves the active tonal centre");
    check(customCollectionSurvivesModulation,
          "modulation preserves the active generated collection");

    std::printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
