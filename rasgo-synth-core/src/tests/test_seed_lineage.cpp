#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "../engine/SeedLineage.hpp"

namespace {
int failures = 0;

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

bool sameSpectral(const rasgo::SpectralPlan &a, const rasgo::SpectralPlan &b) {
    return a.generatedScale == b.generatedScale
        && a.microtonal == b.microtonal
        && a.spectralObject == b.spectralObject
        && a.spectralGhost == b.spectralGhost
        && a.sievePolyphony == b.sievePolyphony;
}

bool sameIdentity(const rasgo::CompositionIdentity &a,
                  const rasgo::CompositionIdentity &b) {
    return a.focus == b.focus
        && a.scaleFamily == b.scaleFamily
        && a.coreVoice == b.coreVoice
        && a.rhythmModel == b.rhythmModel
        && a.leadVoice == b.leadVoice
        && a.harmonicLanguage == b.harmonicLanguage
        && a.ensembleModel == b.ensembleModel
        && a.initialScale == b.initialScale
        && a.rootMidiNote == b.rootMidiNote
        && std::fabs(a.scaleBlendProbability - b.scaleBlendProbability) < 1.0e-12
        && sameSpectral(a.spectral, b.spectral);
}

bool sameResult(const rasgo::SeedLineageResult &a,
                const rasgo::SeedLineageResult &b) {
    return sameIdentity(a.identity, b.identity)
        && a.gameWeights == b.gameWeights
        && a.sources == b.sources
        && a.inheritedBMask == b.inheritedBMask
        && a.blendedMask == b.blendedMask
        && a.mutationMask == b.mutationMask;
}
}

int main() {
    using namespace rasgo;
    SeedLineageParent a;
    a.seed = 11u;
    a.identity = makeCompositionIdentity(a.seed);
    a.gameWeights = {{0.10, 0.20, 0.30, 0.40}};
    a.manifestedModuleCount = 2;
    SeedLineageParent b;
    b.seed = 97u;
    b.identity = makeCompositionIdentity(b.seed);
    b.gameWeights = {{0.40, 0.30, 0.20, 0.10}};
    b.manifestedModuleCount = 89;

    SeedLineageConfig config;
    config.childSeed = 20260822u;
    config.parentBWeight = 0.45;
    config.mutationProbability = 0.2;
    config.generation = 3;
    const auto first = recombineSeedLineage(a, b, config);
    const auto repeated = recombineSeedLineage(a, b, config);
    check(sameResult(first, repeated), "same ancestry and config are deterministic");
    check(first.finite(), "lineage result remains finite and normalized");

    config.parentBWeight = 0.0;
    config.mutationProbability = 0.0;
    const auto endpointA = recombineSeedLineage(a, b, config);
    check(sameIdentity(endpointA.identity, a.identity),
          "zero crossfade preserves parent A identity exactly");
    check(endpointA.gameWeights == a.gameWeights,
          "zero crossfade preserves normalized parent A game weights");
    check(endpointA.inheritedBMask == 0 && endpointA.mutationMask == 0,
          "parent A endpoint reports no B inheritance or mutation");

    config.parentBWeight = 1.0;
    const auto endpointB = recombineSeedLineage(a, b, config);
    check(sameIdentity(endpointB.identity, b.identity),
          "one crossfade preserves parent B identity exactly");
    check(endpointB.gameWeights == b.gameWeights,
          "one crossfade preserves normalized parent B game weights");
    const auto allGenes = static_cast<std::uint16_t>((1U << seedGeneSlotCount()) - 1U);
    check(endpointB.inheritedBMask == allGenes,
          "parent B endpoint reports every inherited gene");

    config.parentBWeight = 0.5;
    config.mutationProbability = 1.0;
    const auto mutant = recombineSeedLineage(a, b, config);
    check(mutant.mutationMask == allGenes,
          "full mutation probability marks all nine domains");
    check(mutant.finite(), "mutated game simplex remains normalized");

    const auto childA = deriveDescendantSeed(a.seed, b.seed, 7u, 2u);
    const auto childAgain = deriveDescendantSeed(a.seed, b.seed, 7u, 2u);
    const auto childOther = deriveDescendantSeed(a.seed, b.seed, 8u, 2u);
    check(childA == childAgain, "descendant seed derivation is deterministic");
    check(childA != childOther, "descendant seed changes with nonce");

    SeedLineageParent aTelemetry = a;
    SeedLineageParent bTelemetry = b;
    aTelemetry.manifestedModuleCount = 89;
    bTelemetry.manifestedModuleCount = 0;
    config.mutationProbability = 0.15;
    check(sameResult(recombineSeedLineage(a, b, config),
                     recombineSeedLineage(aTelemetry, bTelemetry, config)),
          "parent module counts are observation only");

    config.parentBWeight = std::numeric_limits<double>::quiet_NaN();
    config.mutationProbability = std::numeric_limits<double>::infinity();
    const auto sanitized = recombineSeedLineage(a, b, config);
    check(sanitized.parentBWeight == 0.5
              && sanitized.mutationProbability == 0.0
              && sanitized.finite(),
          "non-finite controls receive neutral safe defaults");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "SeedLineage tests passed\n";
    return EXIT_SUCCESS;
}
