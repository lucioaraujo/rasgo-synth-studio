// Rasgo Synth Core -- Phase 123 CLI demo.
//
// This used to carry its own full copy of the render loop; that logic was
// later extracted into engine/PieceRenderer.hpp so the GUI app (rasgo-
// synth-app) could drive the exact same engine instead of duplicating it.
// This file is now a thin CLI wrapper around that shared function -- any
// module added to PieceRenderer.hpp (multiband filter, morph oscillator,
// envelope-as-oscillator, etc.) shows up here automatically, with no risk
// of the CLI demo and the app quietly drifting apart.

#include <cstdio>
#include <string>

#include "../engine/PieceRenderer.hpp"

using namespace rasgo;

int main(int argc, char **argv) {
    PieceConfig cfg;
    cfg.seed = (argc > 1) ? static_cast<unsigned>(std::stoul(argv[1])) : 2026u;
    cfg.outDir = (argc > 2) ? argv[2] : "recordings";
    cfg.bpm = (argc > 3) ? std::stod(argv[3]) : 100.0;
    cfg.titleOverride = (argc > 4) ? argv[4] : "";
    cfg.artistOverride = (argc > 5) ? argv[5] : "";
    // Optional audit tokens after artist can appear in either order.
    // "relations-off", "chamber-off", "attention-off", "narrative-mix-off",
    // "expression-ledger-off", "expression-actuation", "agency-off" and
    // "audibility-off" produce explicit counterfactuals; "stems-audit" runs
    // the expensive organism-level shadow mix and writes its metrics. "wild"
    // enables the deliberate hardware-entropy reseed path and exposes its
    // count in metadata.
    for (int arg = 6; arg < argc; ++arg) {
        const std::string token(argv[arg]);
        if (token == "relations-off") cfg.relationalOrchestraEnabled = false;
        if (token == "chamber-off") cfg.relationalChamberEnabled = false;
        if (token == "attention-off") cfg.narrativeAttentionEnabled = false;
        if (token == "narrative-mix-off") cfg.narrativeMixEnabled = false;
        if (token == "expression-ledger-off") cfg.expressiveEventLedgerEnabled = false;
        if (token == "expression-actuation") cfg.expressiveActuationEnabled = true;
        if (token == "agency-off") cfg.materialAgencyEnabled = false;
        if (token == "audibility-off") cfg.organismAudibilityEnabled = false;
        if (token == "stems-audit") cfg.organismStemAuditEnabled = true;
        if (token == "wild") cfg.wildMode = true;
    }

    std::string lastSection;
    auto result = renderIntegratedPiece(cfg, [&](const PieceProgress &p) {
        if (p.arcState.currentSectionName != lastSection) {
            std::fprintf(stderr, "[%.1fs] secao: %s (densidade=%.2f, filtro=%.2f, ring=%.2f, registro=%+d)\n",
                         p.seconds, p.arcState.currentSectionName.c_str(),
                         p.arcState.density, p.arcState.filterOpenness, p.ringAmount,
                         p.arcState.registerOffset);
            lastSection = p.arcState.currentSectionName;
        }
    });

    if (result.path.empty()) {
        std::fprintf(stderr, "Falha ao gravar arquivo em %s\n", cfg.outDir.c_str());
        return 1;
    }
    std::printf("Gravado: %s\n", result.path.c_str());
    std::printf("Modulos observaveis: %d/%d | wild=%s | reseeds=%d\n",
                result.info.observableModuleActiveCount,
                result.info.observableModuleCapacity,
                result.info.wildMode ? "sim" : "nao",
                result.info.wildReseedCount);
    return 0;
}
