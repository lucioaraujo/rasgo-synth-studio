#pragma once
#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "NarrativeArc.hpp"

namespace rasgo {

// Superseded by GenerativeArc.hpp (phase 13): a fixed-length list generated
// once and scaled to fit a duration chosen before compose can't let the
// duration itself be an emergent property of the piece, which is what phase
// 13 needed. Kept here rather than deleted -- the reasoning below is real
// project history (the manual-override removal it documents actually
// happened) and nothing else in the tree still calls generateSections().
//
// Answers one of the questions the very first planning pass left open:
// should the narrative arc be a fixed template (abertura/desenvolvimento/
// climax/resolucao, always in that order, always the same target values)
// or something the instrument itself decides per piece? Every render
// before this used the same four sections with the same numbers every
// time -- only the notes/rhythm/chaos internals varied by seed, which
// meant every piece had an identical shape even though it never sounded
// identical. This generates the arc itself from the seed: a variable
// number of sections (4 to 6), an energy contour that rises from a quiet
// opening to a peak somewhere in the piece and settles by the end (just
// enough shape to still read as "a piece," not a random walk), and names
// drawn from a wider vocabulary so two pieces don't necessarily narrate
// the same journey.
//
// This used to accept two optional manual overrides (forced section count,
// forced peak intensity) so a player could sculpt the arc's shape directly
// instead of relying purely on the seed. They were removed from the
// desktop app's UI (see the plan document) on the grounds that letting
// only these two parameters out of dozens be hand-set, with everything
// else auto, didn't hold together as a design idea -- an instrument is
// either steered live or seeded and left alone, not both at once for two
// arbitrary knobs. Once the UI stopped setting them they were dead
// parameters carried by every caller for no reason, so they're gone from
// here too: the seed decides the section count and the peak intensity,
// same as it decides everything else.
inline std::vector<Section> generateSections(unsigned seed) {
    std::mt19937 rng(seed + 1); // +1 so this doesn't consume the same first
                                 // draws as the sequencer/chaos seeding elsewhere
    std::uniform_real_distribution<double> unit(0.0, 1.0);

    static const std::vector<std::string> openingNames   = {"abertura", "emergencia", "deriva inicial"};
    static const std::vector<std::string> risingNames    = {"desenvolvimento", "acumulo", "adensamento", "espiral"};
    static const std::vector<std::string> peakNames       = {"climax", "ruptura", "vertice", "colapso"};
    static const std::vector<std::string> fallingNames   = {"decantacao", "arrefecimento", "eco"};
    static const std::vector<std::string> closingNames   = {"resolucao", "repouso", "saldo", "silencio parcial"};

    auto pick = [&](const std::vector<std::string> &pool) {
        return pool[static_cast<size_t>(unit(rng) * pool.size()) % pool.size()];
    };

    int numSections = 4 + static_cast<int>(unit(rng) * 3.0); // 4, 5, or 6
    int peakIndex = std::max(1, numSections / 2 + (unit(rng) < 0.5 ? 0 : 1) - (numSections >= 6 ? 1 : 0));
    peakIndex = std::min(peakIndex, numSections - 2); // keep at least one section after the peak

    double peakEnergyCap = 0.85;

    std::vector<double> energy(numSections);
    for (int i = 0; i < numSections; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(std::max(1, numSections - 1));
        double distFromPeak = std::abs(i - peakIndex) / static_cast<double>(std::max(1, numSections - 1));
        double base = peakEnergyCap - distFromPeak * 1.4; // triangular contour peaking at peakIndex
        base = std::max(0.12, base);
        // A little jitter so two seeds with the same section count still
        // don't produce identical energy curves.
        double jitter = (unit(rng) - 0.5) * 0.12;
        energy[i] = std::clamp(base + jitter, 0.08, 0.95);
        (void)t;
    }
    // Force the ending noticeably below the peak -- a piece that never
    // settles doesn't read as "ending," it just stops.
    energy[numSections - 1] = std::min(energy[numSections - 1], energy[peakIndex] * 0.45);

    std::vector<Section> sections;
    sections.reserve(numSections);
    for (int i = 0; i < numSections; ++i) {
        Section s;
        if (i == 0) s.name = pick(openingNames);
        else if (i == peakIndex) s.name = pick(peakNames);
        else if (i == numSections - 1) s.name = pick(closingNames);
        else if (i < peakIndex) s.name = pick(risingNames);
        else s.name = pick(fallingNames);

        double e = energy[i];
        s.relativeDuration = 0.8 + unit(rng) * 1.0; // 0.8 .. 1.8, arbitrary weights normalised by NarrativeArc
        s.density = std::clamp(0.15 + e * 0.75, 0.1, 0.95);
        s.filterOpenness = std::clamp(0.10 + e * 0.85, 0.08, 0.95);
        s.resonance = std::clamp(0.9 + e * 2.0, 0.8, 3.0);
        s.registerOffset = static_cast<int>(-12.0 + e * 20.0); // -12 .. +8 semitones, roughly

        sections.push_back(s);
    }

    return sections;
}

} // namespace rasgo
