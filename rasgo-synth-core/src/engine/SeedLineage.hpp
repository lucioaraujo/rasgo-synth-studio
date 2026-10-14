#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "CompositionIdentity.hpp"

namespace rasgo {

// Phase 164: explicit, inspectable seed lineage. These are stable domains of
// compositional identity, not switches for the observable mechanisms. The
// mechanisms remain consequences of the child seed, inherited identity and
// the renderer's relational trajectory.
enum class SeedGeneSlot : std::uint8_t {
    Material,
    Pitch,
    CoreVoice,
    Rhythm,
    LeadVoice,
    Harmony,
    Ensemble,
    Spectral,
    Game,
    Count
};

enum class SeedGeneSource : std::uint8_t {
    ParentA,
    ParentB,
    Blended,
    Mutation
};

inline constexpr std::size_t seedGeneSlotCount() {
    return static_cast<std::size_t>(SeedGeneSlot::Count);
}

inline constexpr std::uint16_t seedGeneBit(SeedGeneSlot slot) {
    return static_cast<std::uint16_t>(1U << static_cast<unsigned>(slot));
}

inline const char *seedGeneSlotName(SeedGeneSlot slot) {
    static constexpr const char *names[] = {
        "material", "pitch", "core-voice", "rhythm", "lead-voice",
        "harmony", "ensemble", "spectral", "game"
    };
    const auto index = static_cast<std::size_t>(slot);
    return index < seedGeneSlotCount() ? names[index] : "unknown";
}

inline const char *seedGeneSourceName(SeedGeneSource source) {
    switch (source) {
        case SeedGeneSource::ParentA: return "parent-a";
        case SeedGeneSource::ParentB: return "parent-b";
        case SeedGeneSource::Blended: return "blended";
        case SeedGeneSource::Mutation: return "mutation";
    }
    return "unknown";
}

struct SeedLineageParent {
    unsigned seed = 0;
    CompositionIdentity identity{};
    std::array<double, 4> gameWeights{{0.25, 0.25, 0.25, 0.25}};
    // Observation only. It deliberately never participates in recombination.
    int manifestedModuleCount = -1;
};

struct SeedLineageConfig {
    unsigned childSeed = 0;
    double parentBWeight = 0.5;
    double mutationProbability = 0.0;
    unsigned generation = 1;
};

struct SeedLineageResult {
    unsigned childSeed = 0;
    unsigned parentASeed = 0;
    unsigned parentBSeed = 0;
    unsigned generation = 1;
    double parentBWeight = 0.5;
    double mutationProbability = 0.0;
    CompositionIdentity identity{};
    std::array<double, 4> gameWeights{{0.25, 0.25, 0.25, 0.25}};
    std::array<SeedGeneSource, seedGeneSlotCount()> sources{};
    std::uint16_t inheritedBMask = 0;
    std::uint16_t blendedMask = 0;
    std::uint16_t mutationMask = 0;

    bool finite() const {
        double sum = 0.0;
        for (double value : gameWeights) {
            if (!std::isfinite(value) || value < 0.0) return false;
            sum += value;
        }
        return std::isfinite(parentBWeight)
            && std::isfinite(mutationProbability)
            && std::fabs(sum - 1.0) < 1.0e-9;
    }
};

inline std::array<double, 4> normalizeSeedGameWeights(
        const std::array<double, 4> &input) {
    std::array<double, 4> result{};
    double sum = 0.0;
    for (std::size_t i = 0; i < result.size(); ++i) {
        result[i] = std::isfinite(input[i]) && input[i] > 0.0 ? input[i] : 0.0;
        sum += result[i];
    }
    if (!(sum > 0.0) || !std::isfinite(sum))
        return {{0.25, 0.25, 0.25, 0.25}};
    if (std::fabs(sum - 1.0) < 1.0e-12) return result;
    for (double &value : result) value /= sum;
    return result;
}

inline unsigned deriveDescendantSeed(unsigned parentA, unsigned parentB,
                                     std::uint64_t nonce, unsigned generation) {
    std::uint64_t mixed = identityBits(parentA, 1640ULL + nonce);
    mixed ^= identityBits(parentB, 1641ULL + static_cast<std::uint64_t>(generation));
    mixed ^= 0x6c696e686167656dULL * (nonce + 1ULL);
    mixed ^= mixed >> 32U;
    return static_cast<unsigned>(mixed);
}

namespace seed_lineage_detail {

inline void copyIdentitySlot(CompositionIdentity &destination,
                             const CompositionIdentity &source,
                             SeedGeneSlot slot) {
    switch (slot) {
        case SeedGeneSlot::Material:
            destination.focus = source.focus;
            destination.scaleBlendProbability = source.scaleBlendProbability;
            break;
        case SeedGeneSlot::Pitch:
            destination.scaleFamily = source.scaleFamily;
            destination.initialScale = source.initialScale;
            destination.rootMidiNote = source.rootMidiNote;
            break;
        case SeedGeneSlot::CoreVoice: destination.coreVoice = source.coreVoice; break;
        case SeedGeneSlot::Rhythm: destination.rhythmModel = source.rhythmModel; break;
        case SeedGeneSlot::LeadVoice: destination.leadVoice = source.leadVoice; break;
        case SeedGeneSlot::Harmony:
            destination.harmonicLanguage = source.harmonicLanguage;
            break;
        case SeedGeneSlot::Ensemble: destination.ensembleModel = source.ensembleModel; break;
        case SeedGeneSlot::Spectral: destination.spectral = source.spectral; break;
        case SeedGeneSlot::Game:
        case SeedGeneSlot::Count:
            break;
    }
}

inline std::array<double, 4> generatedGameWeights(unsigned seed) {
    std::array<double, 4> values{};
    for (std::size_t i = 0; i < values.size(); ++i)
        values[i] = 0.05 + identityUnit(seed, 1700ULL + i);
    return normalizeSeedGameWeights(values);
}

} // namespace seed_lineage_detail

inline SeedLineageResult recombineSeedLineage(const SeedLineageParent &parentA,
                                              const SeedLineageParent &parentB,
                                              const SeedLineageConfig &config) {
    SeedLineageResult result;
    result.childSeed = config.childSeed;
    result.parentASeed = parentA.seed;
    result.parentBSeed = parentB.seed;
    result.generation = config.generation;
    result.parentBWeight = std::clamp(
        std::isfinite(config.parentBWeight) ? config.parentBWeight : 0.5, 0.0, 1.0);
    result.mutationProbability = std::clamp(
        std::isfinite(config.mutationProbability) ? config.mutationProbability : 0.0,
        0.0, 1.0);
    result.identity = parentA.identity;

    const unsigned mutantSeed = deriveDescendantSeed(
        config.childSeed, parentA.seed ^ parentB.seed,
        0x164ULL, config.generation + 1U);
    const CompositionIdentity mutant = makeCompositionIdentity(mutantSeed);

    for (std::size_t i = 0; i < seedGeneSlotCount(); ++i) {
        const auto slot = static_cast<SeedGeneSlot>(i);
        const bool mutate = result.mutationProbability >= 1.0
            || (result.mutationProbability > 0.0
                && identityUnit(config.childSeed, 16400ULL + i
                    + static_cast<std::uint64_t>(config.generation) * 31ULL)
                    < result.mutationProbability);

        if (mutate) {
            result.sources[i] = SeedGeneSource::Mutation;
            result.mutationMask |= seedGeneBit(slot);
            if (slot != SeedGeneSlot::Game)
                seed_lineage_detail::copyIdentitySlot(result.identity, mutant, slot);
            continue;
        }

        const bool chooseB = result.parentBWeight >= 1.0
            || (result.parentBWeight > 0.0
                && identityUnit(config.childSeed, 16500ULL + i
                    + static_cast<std::uint64_t>(config.generation) * 37ULL)
                    < result.parentBWeight);
        result.sources[i] = chooseB ? SeedGeneSource::ParentB : SeedGeneSource::ParentA;
        if (chooseB) {
            result.inheritedBMask |= seedGeneBit(slot);
            if (slot != SeedGeneSlot::Game)
                seed_lineage_detail::copyIdentitySlot(result.identity, parentB.identity, slot);
        }
    }

    const auto weightsA = normalizeSeedGameWeights(parentA.gameWeights);
    const auto weightsB = normalizeSeedGameWeights(parentB.gameWeights);
    const auto gameIndex = static_cast<std::size_t>(SeedGeneSlot::Game);
    if ((result.mutationMask & seedGeneBit(SeedGeneSlot::Game)) != 0U) {
        result.gameWeights = seed_lineage_detail::generatedGameWeights(mutantSeed);
    } else if (result.parentBWeight <= 0.0) {
        result.gameWeights = weightsA;
        result.sources[gameIndex] = SeedGeneSource::ParentA;
    } else if (result.parentBWeight >= 1.0) {
        result.gameWeights = weightsB;
        result.sources[gameIndex] = SeedGeneSource::ParentB;
        result.inheritedBMask |= seedGeneBit(SeedGeneSlot::Game);
    } else {
        for (std::size_t i = 0; i < result.gameWeights.size(); ++i)
            result.gameWeights[i] = weightsA[i]
                + (weightsB[i] - weightsA[i]) * result.parentBWeight;
        result.gameWeights = normalizeSeedGameWeights(result.gameWeights);
        result.sources[gameIndex] = SeedGeneSource::Blended;
        result.inheritedBMask &= static_cast<std::uint16_t>(
            ~seedGeneBit(SeedGeneSlot::Game));
        result.blendedMask |= seedGeneBit(SeedGeneSlot::Game);
    }

    return result;
}

} // namespace rasgo
