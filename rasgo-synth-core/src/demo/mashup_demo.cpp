// Rasgo Synth Core -- Fase 100: mashup/recombinacao estrutural de seeds.
//
// Not a mix of two seeds as numbers (that never works -- the whole point
// of a seed is that any change to it, even by one bit, cascades through
// every unit(rng) draw downstream into a completely different piece --
// the butterfly effect this engine's own "rasgo" name already points at).
// Phase 164 extends the original scale/game mashup into nine explicit,
// inspectable identity domains. It still creates a fresh child trajectory:
// no audio splicing and no direct forcing of optional module switches.
#include <cstdio>
#include <cstdlib>
#include <string>

#include "engine/PieceRenderer.hpp"
#include "engine/SeedLineage.hpp"

using namespace rasgo;

namespace {

// Scale/game-weight bias are both decided at setup, before the main
// per-sample render loop -- a short truncated render is enough to read
// them back, no need to spend minutes rendering a piece twice as long as
// necessary just to inspect its own early decisions.
PieceModuleInfo fetchTraits(unsigned seed, double bpm, const std::string &probeDir) {
    PieceConfig cfg;
    cfg.seed = seed;
    cfg.bpm = bpm;
    cfg.maxDurationSeconds = 3.0;
    cfg.outDir = probeDir;
    auto result = renderIntegratedPiece(cfg, nullptr);
    return result.info;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 5) {
        std::fprintf(stderr,
            "usage: %s seedA seedB outDir newSeed [crossfade 0..1] [bpm] [mutation 0..1] [generation]\n"
            "  seedA/seedB: the two source pieces to blend structural traits from\n"
            "  crossfade: 0.0 = all A, 1.0 = all B, 0.5 (default) = even blend\n",
            argv[0]);
        return 2;
    }
    unsigned seedA = static_cast<unsigned>(std::stoul(argv[1]));
    unsigned seedB = static_cast<unsigned>(std::stoul(argv[2]));
    std::string outDir = argv[3];
    unsigned newSeed = static_cast<unsigned>(std::stoul(argv[4]));
    double crossfade = (argc > 5) ? std::stod(argv[5]) : 0.5;
    double bpm = (argc > 6) ? std::stod(argv[6]) : 100.0;
    double mutation = (argc > 7) ? std::stod(argv[7]) : 0.0;
    unsigned generation = (argc > 8) ? static_cast<unsigned>(std::stoul(argv[8])) : 1u;

    std::string probeDir = outDir + "/.mashup_probe";
    auto traitsA = fetchTraits(seedA, bpm, probeDir);
    auto traitsB = fetchTraits(seedB, bpm, probeDir);

    std::fprintf(stderr, "Peca A (seed %u): escala=%s  pesos alea=%.3f agon=%.3f ilinx=%.3f mimicry=%.3f\n",
                 seedA, scaleName(traitsA.initialScaleValue),
                 traitsA.gameWeightAlea, traitsA.gameWeightAgon, traitsA.gameWeightIlinx, traitsA.gameWeightMimicry);
    std::fprintf(stderr, "Peca B (seed %u): escala=%s  pesos alea=%.3f agon=%.3f ilinx=%.3f mimicry=%.3f\n",
                 seedB, scaleName(traitsB.initialScaleValue),
                 traitsB.gameWeightAlea, traitsB.gameWeightAgon, traitsB.gameWeightIlinx, traitsB.gameWeightMimicry);

    SeedLineageParent parentA;
    parentA.seed = seedA;
    parentA.identity = traitsA.compositionIdentityValue;
    parentA.gameWeights = {{traitsA.gameWeightAlea, traitsA.gameWeightAgon,
                            traitsA.gameWeightIlinx, traitsA.gameWeightMimicry}};
    SeedLineageParent parentB;
    parentB.seed = seedB;
    parentB.identity = traitsB.compositionIdentityValue;
    parentB.gameWeights = {{traitsB.gameWeightAlea, traitsB.gameWeightAgon,
                            traitsB.gameWeightIlinx, traitsB.gameWeightMimicry}};
    SeedLineageConfig lineageConfig;
    lineageConfig.childSeed = newSeed;
    lineageConfig.parentBWeight = crossfade;
    lineageConfig.mutationProbability = mutation;
    lineageConfig.generation = generation;
    const auto lineage = recombineSeedLineage(parentA, parentB, lineageConfig);

    std::fprintf(stderr,
        "Descendente (seed %u, geracao=%u, B=%.2f, mutacao=%.2f): "
        "escala=%s, pesos alea=%.3f agon=%.3f ilinx=%.3f mimicry=%.3f\n",
        lineage.childSeed, lineage.generation, lineage.parentBWeight,
        lineage.mutationProbability, scaleName(lineage.identity.initialScale),
        lineage.gameWeights[0], lineage.gameWeights[1],
        lineage.gameWeights[2], lineage.gameWeights[3]);
    for (std::size_t i = 0; i < seedGeneSlotCount(); ++i) {
        const auto slot = static_cast<SeedGeneSlot>(i);
        std::fprintf(stderr, "  gene %-10s <- %s\n",
                     seedGeneSlotName(slot), seedGeneSourceName(lineage.sources[i]));
    }

    PieceConfig cfg;
    cfg.seed = newSeed;
    cfg.bpm = bpm;
    cfg.outDir = outDir;
    cfg.compositionIdentityOverride = true;
    cfg.compositionIdentityOverrideValue = lineage.identity;
    cfg.gameWeightOverride = true;
    cfg.gameWeightAleaOverride = lineage.gameWeights[0];
    cfg.gameWeightAgonOverride = lineage.gameWeights[1];
    cfg.gameWeightIlinxOverride = lineage.gameWeights[2];
    cfg.gameWeightMimicryOverride = lineage.gameWeights[3];
    cfg.seedLineageEnabled = true;
    cfg.seedLineageParentA = lineage.parentASeed;
    cfg.seedLineageParentB = lineage.parentBSeed;
    cfg.seedLineageGeneration = lineage.generation;
    cfg.seedLineageParentBWeight = lineage.parentBWeight;
    cfg.seedLineageMutationProbability = lineage.mutationProbability;
    cfg.seedLineageInheritedBMask = lineage.inheritedBMask;
    cfg.seedLineageBlendedMask = lineage.blendedMask;
    cfg.seedLineageMutationMask = lineage.mutationMask;

    auto result = renderIntegratedPiece(cfg, nullptr);
    if (result.path.empty()) {
        std::fprintf(stderr, "Falha ao gravar mashup em %s\n", cfg.outDir.c_str());
        return 1;
    }
    std::printf("Mashup gravado: %s\n", result.path.c_str());
    return 0;
}
