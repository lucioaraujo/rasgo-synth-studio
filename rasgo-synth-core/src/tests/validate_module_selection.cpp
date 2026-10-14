// Opt-in, bounded audio integration. No sound device; fresh evidence folder.
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

static void checkAudio(const rasgo::PieceResult& p) {
    assert(!p.path.empty() && !p.samples.empty());
    double energy = 0.0;
    for (float sample : p.samples) {
        assert(std::isfinite(sample) && std::abs(sample) <= 1.0);
        energy += double(sample) * sample;
    }
    assert(energy / p.samples.size() > 1e-8);
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const std::string root = argv[1];
    rasgo::PieceConfig legacy;
    legacy.seed = 2026u; legacy.maxDurationSeconds = .01;
    legacy.outDir = root + "/legacy";
    auto before = rasgo::renderIntegratedPiece(legacy);
    checkAudio(before);
    std::cout << "LEGACY " << before.path << '\n';
    before.samples.clear(); before.samples.shrink_to_fit();

    rasgo::AlbumConfig cfg;
    cfg.albumSeed = 2372793893u; cfg.trackCount = 2; cfg.outDir = root + "/selected";
    cfg.options.targetDurationSeconds = 30;
    auto& selection = cfg.moduleSelection;
    using M = rasgo::StudioModule;
    using C = rasgo::ModuleChoice;
    for (auto module : {M::Core, M::Acid, M::Tuba, M::AtariPunk, M::NoiseBed,
                        M::Granular, M::Gendyn, M::Snare, M::Toms, M::Cymbal})
        selection.set(module, C::Exclude);
    selection.set(M::FmFlute, C::Include);
    selection.set(M::Drums, C::Include);
    auto pc = rasgo::detail::makeTrackConfig(cfg, root, 1u, 0, {.25,.25,.25,.25}, .5);
    assert(pc.moduleSelection.choices == cfg.moduleSelection.choices);
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
        checkAudio(p);
        assert(files.insert(p.path).second);
        assert(std::abs(p.samples.size() / 96000.0 - plan.tracks[i].targetDurationSeconds) < .25);
        assert(final[i] == p.samples.size() / 2 && calibration[i] == final[i]);
        assert(p.info.fmFlute && p.info.fmFluteNoteCount > 0);
        std::cout << "SELECTED " << p.path << '\n';
    }
    rasgo::PieceConfig blocked;
    blocked.seed = 2262550329u; blocked.targetDurationSeconds = 15;
    blocked.outDir = root + "/blocked";
    blocked.moduleSelection.set(M::Drums, C::Exclude);
    blocked.moduleSelection.set(M::Kick, C::Include);
    auto result = rasgo::renderIntegratedPiece(blocked);
    checkAudio(result);
    assert(!result.info.drumsEnabled);
    std::cout << "BLOCKED " << result.path << '\n';
    std::cout << "Selection adapter, album calibration/final, duration, samples, parent conflict: PASS\n";
}
