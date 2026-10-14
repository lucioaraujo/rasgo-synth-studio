#pragma once
#include <algorithm>
#include <array>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include "../sequencer/Euclidean.hpp"

namespace rasgo {

// Fase 85 -- generalized, parametric rhythm generation, replacing (over
// several stages) the small fixed catalog in DrumArchetypes.hpp with real
// combinatorial/theoretical frameworks that each cover a genuine FAMILY of
// documented rhythms rather than one hand-authored pattern each. This file
// is stage 1: the mathematical pattern generators (A) and the metrical
// weight profile (B). Stage 2 (gesture/dynamics/repetition-with-difference)
// and stage 3 (orchestration/narrative/glitch coupling) build on top of
// these, in RhythmGenerator.hpp additions or sibling files, once this stage
// is validated. Fase 89 generalizes A and B further to variable bar
// lengths (see below), for real meter change.
//
// Honesty discipline (same as the rest of this project): nothing here
// claims the engine "understands" rhythm. Each generator below is a real,
// citable, testable mathematical structure; what varies piece to piece is
// which point in that structure gets sampled, not any judgment about what
// sounds good.

// ---------------------------------------------------------------------
// B. Metrical hierarchy (Lerdahl & Jackendoff, "A Generative Theory of
// Tonal Music", 1983) -- a real, well-documented model of how strongly
// each position in a bar is felt as a beat, built from recursive binary
// subdivision (whole -> half -> quarter -> eighth -> sixteenth). Over a
// 16-step cycle (2^4), this is exactly the "ruler sequence" (OEIS A001511:
// the 2-adic valuation of each integer) -- not a coincidence, binary
// metrical subdivision of a bar of 16 IS this exact structure, which is
// why a well-known integer sequence and the classical metrical grid line
// up perfectly. Step 0 is the top of the cycle, defined as one level
// stronger than the deepest subdivision (level 4) since it's shared by
// every level of the hierarchy at once.
//
// Kept exactly as originally written (fase 85) -- every existing call site
// that assumes the default 16-step bar keeps behaving identically.
inline int metricalLevel(int step) {
    if (step <= 0) return 4;
    int v = 0;
    int s = step;
    while ((s & 1) == 0) { ++v; s >>= 1; }
    return v; // 0 (weakest, odd steps) .. 3 (step 8)
}

// Normalized 0..1, level 4 (step 0) -> 1.0, level 0 (odd steps) -> 0.0.
inline double metricalStrength(int step) {
    return static_cast<double>(metricalLevel(step)) / 4.0;
}

// Fase 89 -- the same real theory (Lerdahl & Jackendoff explicitly cover
// meters built from real grouping structure, not only binary subdivision)
// generalized to bar lengths that aren't a power of two (12/20/14 -- see
// generateAksakPattern below for why these specific lengths). For the
// default 16-step bar this delegates to the exact original ruler-sequence
// model above (identical result, not an approximation). For any other
// length, strength comes from real GROUPING depth instead: strongest at
// the top of the cycle, next-strongest at every group start (groupSize
// steps apart), weakest everywhere else -- a real, if shallower (3-level
// instead of 5), metrical hierarchy for a compound/asymmetric meter,
// exactly matching how such meters are actually described (e.g. a 12-step
// bar felt as four beats of three, not as abstract binary subdivision).
inline int metricalLevelForBar(int step, int barLength, int groupSize) {
    if (barLength == 16) return metricalLevel(step);
    if (step <= 0) return 2;
    return (step % groupSize == 0) ? 1 : 0;
}

inline double metricalStrengthForBar(int step, int barLength, int groupSize) {
    if (barLength == 16) return metricalStrength(step);
    return static_cast<double>(metricalLevelForBar(step, barLength, groupSize)) / 2.0;
}

// ---------------------------------------------------------------------
// A.1 Generalized aksak / additive rhythm (Constantin Brăiloiu, Stoyan
// Dzhudzhev -- Balkan/Turkish aksak theory: a meter built from a sequence
// of "long" (3-unit) and "short" (2-unit) beat groups, rather than a
// single steady subdivision). DrumArchetypes.hpp already catalogs two
// fixed real cells (9+7 = 2+2+2+3+2+2+3, and 7+7+2) at the engine's
// default 16-step bar. This generalizes that same real theory to the full
// space of orderings that sum exactly to a given bar length: for any
// barLength, 3*longCount + 2*shortCount = barLength has one or more real
// solutions with longCount >= 2 (longCount = 0 degenerates to a plain
// duple pulse, not an aksak meter by definition, already covered elsewhere
// by the XOR clock) -- smaller longCount values are weighted more common,
// matching the majority of named, documented aksak meters (7, 9, 11,
// 13-count cells built from exactly two long beats); larger longCount
// covers the rarer, more insistent "more-long-beats" feel.
struct AksakPattern {
    std::vector<bool> groupStart;  // onset at the start of every group
    std::vector<bool> longAccent;  // onset at the start of "long" (3) groups only
    std::string description;
};

inline AksakPattern generateAksakPattern(std::mt19937 &rng, int barLength = 16) {
    std::uniform_real_distribution<double> unit(0.0, 1.0);

    std::vector<int> validLongCounts;
    for (int longCount = 2; 3 * longCount <= barLength; longCount += 2) {
        int rem = barLength - 3 * longCount;
        if (rem >= 0 && rem % 2 == 0) validLongCounts.push_back(longCount);
    }
    int longCount = validLongCounts.empty() ? 0 : validLongCounts.front();
    if (validLongCounts.size() > 1) {
        // Smaller longCount weighted more common (70%); otherwise uniform
        // over the remaining, rarer options.
        if (unit(rng) < 0.7) {
            longCount = validLongCounts.front();
        } else {
            std::uniform_int_distribution<size_t> pick(1, validLongCounts.size() - 1);
            longCount = validLongCounts[pick(rng)];
        }
    }
    int shortCount = longCount > 0 ? (barLength - 3 * longCount) / 2 : 0;

    std::vector<int> groups(static_cast<size_t>(longCount), 3);
    groups.insert(groups.end(), static_cast<size_t>(shortCount), 2);
    if (groups.empty()) groups.push_back(barLength); // degenerate fallback, shouldn't happen for the curated bar lengths
    std::shuffle(groups.begin(), groups.end(), rng);

    AksakPattern result;
    result.groupStart.assign(static_cast<size_t>(barLength), false);
    result.longAccent.assign(static_cast<size_t>(barLength), false);
    std::ostringstream desc;
    desc << "Aksak generalizado (";
    int pos = 0;
    for (size_t g = 0; g < groups.size(); ++g) {
        result.groupStart[static_cast<size_t>(pos)] = true;
        if (groups[g] == 3) result.longAccent[static_cast<size_t>(pos)] = true;
        desc << groups[g];
        if (g + 1 < groups.size()) desc << "+";
        pos += groups[g];
    }
    desc << ")";
    result.description = desc.str();
    return result;
}

// ---------------------------------------------------------------------
// A.2 Generalized Euclidean rhythm (Bjorklund's algorithm; Godfried
// Toussaint, "The Euclidean Algorithm Generates Traditional Musical
// Rhythms", 2005). DrumArchetypes.hpp's fase-57 cowbell already used this
// for one voice at a small, curated set of k values. This generalizes it
// to any k (a caller-chosen range) out of a given bar length, with a free
// rotation, reusing the exact same Bjorklund implementation
// (Euclidean.hpp) rather than a second one. barLength defaults to 16 so
// every existing call site (cymbal/maracas, which stay at the engine's
// default 16-step cycle regardless of any per-section meter change --
// see PieceRenderer.hpp fase 89) keeps behaving identically.
struct EuclideanPattern {
    std::vector<bool> onset;
    int pulses = 0;
    int rotation = 0;
    std::string description;
};

inline EuclideanPattern generateEuclideanPattern(std::mt19937 &rng, int minK = 2, int maxK = 11, int barLength = 16) {
    int clampedMaxK = std::min(maxK, barLength - 1);
    int clampedMinK = std::min(minK, clampedMaxK);
    std::uniform_int_distribution<int> kDist(clampedMinK, clampedMaxK);
    std::uniform_int_distribution<int> rotDist(0, barLength - 1);
    int k = kDist(rng);
    int rot = rotDist(rng);
    std::vector<bool> raw = euclideanRhythm(barLength, k);

    EuclideanPattern result;
    result.onset.assign(static_cast<size_t>(barLength), false);
    for (int i = 0; i < barLength; ++i) {
        result.onset[static_cast<size_t>(i)] = raw[static_cast<size_t>((i + rot) % barLength)];
    }
    result.pulses = k;
    result.rotation = rot;
    std::ostringstream desc;
    desc << "E(" << k << "," << barLength << ") rotacao " << rot;
    result.description = desc.str();
    return result;
}

// ---------------------------------------------------------------------
// D. Continuous dynamics (fase 85 stage 2) -- a real per-hit velocity
// instead of a flat on/off trigger, so a hit landing on a strong beat
// (step 0/8) reads audibly louder than one landing on a weak upbeat, the
// way a real drummer naturally accents the beat rather than striking
// every note with identical force. Built from the metrical weight above
// (B) plus a slow phrase-level "gesture" bias (C -- supplied by the
// caller from an already-running chaos/LFO source; this function stays a
// pure combination, no state of its own) and the piece's own arc density
// (louder, more insistent hits as the arc builds toward its climax).
// Centered near 1.0 so the existing per-voice gain balancing (kickGain
// etc. in PieceRenderer.hpp) still reads the same on average -- this
// modulates AROUND that baseline, not instead of it. Three human labels
// (forte/medio/fraco) describe regions of this same continuous range for
// documentation purposes; the engine itself only ever computes the
// continuous value.
inline double hitVelocity(int step, double gestureBias, double density) {
    double v = 0.6 + 0.5 * metricalStrength(step); // 0.6 (weak) .. 1.1 (step 0)
    v += gestureBias * 0.10;                       // phrase-level push/pull
    v += (density - 0.5) * 0.30;                   // arc: louder near the climax
    return std::clamp(v, 0.35, 1.35);
}

// Fase 89 variant -- same shape, but reads the bar-length-aware metrical
// model above so dynamics stay meaningful when a section's meter isn't
// the default 16.
inline double hitVelocityForBar(int step, int barLength, int groupSize, double gestureBias, double density) {
    double v = 0.6 + 0.5 * metricalStrengthForBar(step, barLength, groupSize);
    v += gestureBias * 0.10;
    v += (density - 0.5) * 0.30;
    return std::clamp(v, 0.35, 1.35);
}

// Same idea for voices with no meaningful step position (cymbal/maracas,
// triggered off chaos zero-crossings rather than the 16-step grid, see
// PieceRenderer.hpp) -- arc density alone still applies.
inline double hitVelocityByDensity(double density) {
    return std::clamp(0.75 + (density - 0.5) * 0.30, 0.5, 1.1);
}

} // namespace rasgo
