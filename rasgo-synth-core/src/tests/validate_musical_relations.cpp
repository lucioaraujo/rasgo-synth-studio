// Opt-in bounded real-audio evidence. No device; choose a fresh local folder.
#include "../engine/AlbumRenderer.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const std::string root = argv[1];
    rasgo::PieceConfig legacy;
    legacy.seed = 2026; legacy.maxDurationSeconds = .01; legacy.outDir = root + "/legacy";
    auto old = rasgo::renderIntegratedPiece(legacy);
    assert(!old.path.empty()); std::cout << "LEGACY " << old.path << '\n';
    old.samples.clear(); old.samples.shrink_to_fit();
    rasgo::AlbumConfig album;
    album.options.musicalDirectionEnabled = true; album.musicalRelationsEnabled = false;
    assert(!rasgo::detail::makeTrackConfig(album, root, 1, 0, {.25,.25,.25,.25}, .5).musicalRelationsEnabled);
    album.musicalRelationsEnabled = true;
    assert(rasgo::detail::makeTrackConfig(album, root, 1, 0, {.25,.25,.25,.25}, .5).musicalRelationsEnabled);
    rasgo::PieceConfig cfg;
    cfg.seed = 2001652317; cfg.bpm = 185; cfg.targetDurationSeconds = 24;
    cfg.expressiveActuationEnabled = true; cfg.musicalDirectionEnabled = true;
    cfg.moduleSelection.set(rasgo::StudioModule::FmFlute, rasgo::ModuleChoice::Include);
    cfg.moduleSelection.set(rasgo::StudioModule::Drums, rasgo::ModuleChoice::Include);
    cfg.moduleSelection.set(rasgo::StudioModule::Acid, rasgo::ModuleChoice::Exclude);
    cfg.moduleSelection.set(rasgo::StudioModule::Tuba, rasgo::ModuleChoice::Exclude);
    cfg.musicalRelationsEnabled = false; cfg.outDir = root + "/off";
    const auto before = rasgo::renderIntegratedPiece(cfg);
    cfg.musicalRelationsEnabled = true; cfg.outDir = root + "/on";
    const auto after = rasgo::renderIntegratedPiece(cfg);
    assert(before.path != after.path && before.samples != after.samples);
    for (const auto* piece : {&before, &after}) {
        assert(!piece->path.empty());
        assert(std::abs(piece->samples.size() / 96000.0 - 24.0) < .25);
        double energy = 0;
        for (float sample : piece->samples) {
            assert(std::isfinite(sample) && std::abs(sample) <= 1.0);
            energy += double(sample) * sample;
        }
        assert(energy / piece->samples.size() > 1e-8 && piece->info.bassEventCount > 0);
        assert(piece->info.fmFluteNoteCount > 0); // FM must recover after arranged rests.
    }
    std::cout << "OFF " << before.path << '\n' << "ON " << after.path << '\n';
    album.albumSeed = 2372793893u; album.trackCount = 2;
    album.outDir = root + "/album"; album.expressiveActuationEnabled = true;
    album.moduleSelection = cfg.moduleSelection;
    album.options.targetDurationSeconds = 30;
    std::array<std::uint64_t, 2> calibration{}, final{};
    const auto tracks = rasgo::renderAlbum(album, [&](int index, int, const rasgo::PieceProgress& p, bool probe) {
        if (p.phase == rasgo::PieceRenderPhase::Complete) {
            assert(p.completedFrames == p.totalFrames);
            (probe ? calibration : final)[static_cast<std::size_t>(index)] = p.completedFrames;
        }
    });
    assert(tracks.size() == 2);
    for (std::size_t index = 0; index < tracks.size(); ++index) {
        const auto& piece = tracks[index].piece;
        assert(!piece.path.empty() && !piece.samples.empty());
        assert(final[index] == calibration[index] && final[index] == piece.samples.size() / 2);
        for (float sample : piece.samples) assert(std::isfinite(sample) && std::abs(sample) <= 1);
        std::cout << "ALBUM " << piece.path << '\n';
    }
    std::cout << "Same-config counterfactual, real album passes, actual changed PCM, samples: PASS\n";
}
