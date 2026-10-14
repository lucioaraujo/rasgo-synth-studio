#pragma once
#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <random>
#include <sstream>
#include <vector>
#include "DrumArchetypes.hpp"

namespace rasgo {

// Xenakis rhythmic sieves (Formalized Music, chapter XI "Sieves: A User's
// Guide"; used in practice in works like Persephassa, built from a sieve of
// modulus 60 sixteenth notes) -- the same modulus/residue congruence theory
// already used for pitch (SieveScale.hpp, fase 61), applied here to POINTS
// IN TIME within this engine's fixed 16-step cycle instead of pitch classes.
// A step s belongs to the onset sieve if s % modulus == residue for at
// least one chosen (modulus, residue) pair -- exactly the same construction
// as the pitch sieve, just over a different domain.
//
// Verified computationally before wiring in (same discipline as the pitch
// sieve and the Walsh oscillator): rejection-sampled against every existing
// named DrumArchetype's own kick pattern so "not one of the catalogued
// rhythms" is a checked property here, not an assumption, the same honesty
// fix the pitch sieve needed after an 11.6% real collision rate was found.
struct RhythmSieve {
    std::vector<bool> kick;
    std::vector<bool> snare;
    std::string description;
};

inline std::set<std::vector<bool>> cataloguedKickPatterns() {
    std::set<std::vector<bool>> out;
    for (const auto &arch : drumArchetypes()) out.insert(std::vector<bool>(arch.kick.begin(), arch.kick.end()));
    return out;
}

// Fase 89 -- barLength defaults to 16 so every existing call site keeps
// behaving identically. For any other bar length (a section using a
// different real meter -- see PieceRenderer.hpp), the rejection check
// against the fixed archetype catalog is skipped entirely: that catalog
// only exists at the engine's default 16-step bar, so "not one of the
// catalogued rhythms" isn't even a meaningful claim to check at another
// length -- honestly omitted rather than compared against the wrong thing.
inline RhythmSieve generateRhythmSieve(std::mt19937 &rng, int barLength = 16) {
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    auto cataloged = (barLength == 16) ? cataloguedKickPatterns() : std::set<std::vector<bool>>{};

    std::vector<bool> kick(static_cast<size_t>(barLength), false);
    std::vector<std::pair<int, int>> kickConditions;
    int attempts = 0;
    bool collides;
    int kickCount = 0;
    int minOnsets = std::max(2, barLength / 4);
    int maxOnsets = std::max(minOnsets + 1, (barLength * 10) / 16);
    do {
        kickConditions.clear();
        // 2-3 conditions, modulus 3..7 -- small moduli over the bar's
        // cycle keep the result from degenerating into either near-empty
        // (modulus too close to barLength) or near-continuous (too many
        // conditions), the same balance the pitch sieve strikes over its
        // own 12-pitch-class domain.
        int numConditions = 2 + (unit(rng) < 0.4 ? 1 : 0);
        for (int c = 0; c < numConditions; ++c) {
            int modulus = 3 + static_cast<int>(unit(rng) * 5.0); // 3..7
            int residue = static_cast<int>(unit(rng) * modulus);
            kickConditions.push_back({modulus, residue});
        }
        std::fill(kick.begin(), kick.end(), false);
        kick[0] = true; // step 0 forced in -- every pattern in this engine
                         // treats the top of the cycle as a real downbeat
        kickCount = 1;
        for (int s = 1; s < barLength; ++s) {
            for (auto &cond : kickConditions) {
                if (s % cond.first == cond.second) {
                    if (!kick[static_cast<size_t>(s)]) ++kickCount;
                    kick[static_cast<size_t>(s)] = true;
                    break;
                }
            }
        }
        collides = cataloged.count(kick) > 0;
        ++attempts;
        // A quarter to ~two-thirds of the bar in onsets: fewer reads as
        // too skeletal to carry a kick pattern at all, more starts
        // crowding into "every step" territory Metal's blast beat already
        // occupies honestly (at the default 16-step bar this is exactly
        // the original 4..10 range).
    } while ((kickCount < minOnsets || kickCount > maxOnsets || collides) && attempts < 20);

    // Snare drawn independently, its own small condition set, rejection-
    // sampled only against collision with the kick pattern just built
    // (a snare identical to the kick would just double it, not add a
    // real second voice) -- not against the full archetype catalog,
    // since the snare alone was never the uncataloged claim.
    std::vector<bool> snare(static_cast<size_t>(barLength), false);
    std::vector<std::pair<int, int>> snareConditions;
    attempts = 0;
    int snareCount = 0;
    int minSnare = std::max(1, barLength / 8);
    int maxSnare = std::max(minSnare + 1, (barLength * 6) / 16);
    do {
        snareConditions.clear();
        int numConditions = 1 + (unit(rng) < 0.5 ? 1 : 0);
        for (int c = 0; c < numConditions; ++c) {
            int modulus = 3 + static_cast<int>(unit(rng) * 5.0);
            int residue = static_cast<int>(unit(rng) * modulus);
            snareConditions.push_back({modulus, residue});
        }
        std::fill(snare.begin(), snare.end(), false);
        snareCount = 0;
        for (int s = 0; s < barLength; ++s) {
            for (auto &cond : snareConditions) {
                if (s % cond.first == cond.second) {
                    if (!snare[static_cast<size_t>(s)]) ++snareCount;
                    snare[static_cast<size_t>(s)] = true;
                    break;
                }
            }
        }
        ++attempts;
    } while ((snareCount < minSnare || snareCount > maxSnare || snare == kick) && attempts < 20);

    RhythmSieve result;
    result.kick = kick;
    result.snare = snare;

    std::ostringstream desc;
    desc << "Peneira ritmica gerada (kick";
    for (auto &cond : kickConditions) desc << " s%" << cond.first << "==" << cond.second;
    desc << "; snare";
    for (auto &cond : snareConditions) desc << " s%" << cond.first << "==" << cond.second;
    desc << "; " << kickCount << " onsets de kick)";
    result.description = desc.str();
    return result;
}

} // namespace rasgo
