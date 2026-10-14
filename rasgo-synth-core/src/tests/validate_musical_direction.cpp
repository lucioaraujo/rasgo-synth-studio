// Opt-in audio integration; no device. Caller chooses a local evidence folder.
#include "../engine/AlbumRenderer.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const std::string root = argv[1];
    rasgo::PieceConfig legacy;
    legacy.seed = 2026u; legacy.maxDurationSeconds = .01;
    legacy.outDir = root + "/legacy";
    auto before = rasgo::renderIntegratedPiece(legacy);
    assert(!before.path.empty());
    std::cout << "LEGACY " << before.path << '\n';
    before.samples.clear(); before.samples.shrink_to_fit();

    rasgo::AlbumConfig cfg;
    cfg.albumSeed = 2372793893u; cfg.trackCount = 2; cfg.outDir = root + "/directed";
    cfg.expressiveActuationEnabled = true;
    cfg.options.musicalDirectionEnabled = true;
    cfg.options.targetDurationSeconds = 60;
    cfg.options.varyBpm = true;
    // Prove the common configuration adapter, including audit selection.
    auto pc = rasgo::detail::makeTrackConfig(cfg, root, 3313973965u, 0, {.25,.25,.25,.25}, .5);
    assert(pc.expressiveActuationEnabled && pc.musicalDirectionEnabled && !pc.organismStemAuditEnabled);
    auto auditCfg = cfg; auditCfg.organismStemAuditEnabled = true;
    assert(rasgo::detail::makeTrackConfig(auditCfg, root, 1u, 0, {.25,.25,.25,.25}, .5).organismStemAuditEnabled);
    const auto plan = rasgo::makeAlbumPlan(cfg.albumSeed, cfg.trackCount, cfg.bpm, cfg.options);
    std::array<std::uint64_t, 2> calibration{}, final{};
    const auto tracks = rasgo::renderAlbum(cfg, [&](int index, int, const rasgo::PieceProgress& p, bool probe) {
        if (p.phase == rasgo::PieceRenderPhase::Complete) {
            assert(p.completedFrames == p.totalFrames);
            (probe ? calibration : final)[static_cast<std::size_t>(index)] = p.completedFrames;
        }
    });
    assert(tracks.size() == 2);
    std::set<std::string> files;
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        const auto& p = tracks[i].piece;
        assert(!p.path.empty() && files.insert(p.path).second && p.info.expressiveActuation && p.info.bassBehaviour);
        assert(p.info.bassPlanCount > 0);
        assert(std::abs(p.samples.size() / 96000.0 - plan.tracks[i].targetDurationSeconds) < .25);
        assert(final[i] == p.samples.size() / 2 && calibration[i] == final[i]);
        double energy = 0.0;
        for (float sample : p.samples) {
            assert(std::isfinite(sample) && std::abs(sample) <= 1.0);
            energy += double(sample) * sample;
        }
        assert(!p.samples.empty() && energy / p.samples.size() > 1e-8);
        std::cout << "DIRECTED " << p.path << " bassPlans=" << p.info.bassPlanCount << " events=" << p.info.bassEventCount << '\n';
    }
    std::cout << "Real album expressive/bass routing, duration, calibration/final, samples: PASS\n";
}
