// Opt-in full-render regression: run serially with external resource limits.
// No recordings; target 12 s. This isolates the fallback from other instruments
// (the older bass timbre integration forced FM on, hiding inactive-family gain).
#include "../engine/PieceRenderer.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif

int main() {
    using namespace rasgo;
    PieceConfig cfg;
    cfg.seed = 1466468054u; // Planches track 3: events existed but were muted.
    cfg.bpm = 120;
    cfg.targetDurationSeconds = 12;
    cfg.maxDurationSeconds = 16;
    cfg.recordOutput = false;
    cfg.probabilityMarketOverride = true;
    cfg.expressiveActuationEnabled = true;
    cfg.musicalDirectionEnabled = true;
    cfg.organismStemAuditEnabled = true;
    for (auto module : {StudioModule::Acid, StudioModule::Tuba,
             StudioModule::Theremin, StudioModule::FmFlute,
             StudioModule::Galo, StudioModule::Footnote})
        cfg.moduleSelection.set(module, ModuleChoice::Exclude);
    cfg.moduleSelection.set(StudioModule::Core, ModuleChoice::Include);
    const auto bass = static_cast<std::size_t>(OrganismStem::BassFallback);
    const auto instrument = static_cast<std::size_t>(AgencyFamily::Instrument);
    for (bool agency : {true, false}) {
        cfg.materialAgencyEnabled = agency;
        auto piece = renderIntegratedPiece(cfg);
        const auto& metrics = piece.info.organismStemMetrics[bass];
        std::cout << "agency=" << agency
            << " events=" << piece.info.bassCoreSubEventCount
            << " bassPreRms=" << metrics.preSpaceRms
            << " bassPostRms=" << metrics.postSpaceRms
            << " instrumentRms=" << piece.info.agencyRms[instrument]
            << std::endl;
        assert(piece.path.empty() && !piece.samples.empty());
        assert(piece.info.bassCoreSubEventCount > 0);
        assert(!piece.info.atropelo); // No hidden Instrument keeps the family alive.
        assert(metrics.preSpaceRms > 1e-7 && metrics.postSpaceRms > 1e-7);
        assert(piece.info.agencyRms[instrument] > 1e-7);
        for (float sample : piece.samples) assert(std::isfinite(sample));
        auto repeat = renderIntegratedPiece(cfg);
        assert(repeat.samples == piece.samples);
    }
    cfg.expressiveActuationEnabled = false;
    auto bypass = renderIntegratedPiece(cfg);
    assert(bypass.info.bassCoreSubEventCount == 0);
    assert(bypass.info.organismStemMetrics[bass].preSpaceRms == 0);
    std::cout << "PASS fallback: audible signal, repeat PCM, expressive bypass\n";
}
