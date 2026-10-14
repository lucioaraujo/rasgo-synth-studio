// Opt-in, serial A/B. Preserve the preceding headers before integration;
// compile with RASGO_KICK_BASELINE and RASGO_BASELINE_RENDERER to capture PCM.
#ifdef RASGO_KICK_BASELINE
#include RASGO_BASELINE_RENDERER
#else
#include "../engine/AlbumRenderer.hpp"
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif
namespace fs = std::filesystem;
using namespace rasgo;

static PieceConfig config(unsigned seed) {
    PieceConfig cfg;
    cfg.seed = seed; cfg.bpm = 120;
    cfg.targetDurationSeconds = 12; cfg.maxDurationSeconds = 16;
    cfg.recordOutput = false; cfg.probabilityMarketOverride = true;
    cfg.expressiveActuationEnabled = true; cfg.musicalDirectionEnabled = true;
    cfg.organismStemAuditEnabled = false;
    for (auto module : {StudioModule::Acid, StudioModule::Tuba, StudioModule::Theremin,
             StudioModule::FmFlute, StudioModule::Galo, StudioModule::Footnote})
        cfg.moduleSelection.set(module, ModuleChoice::Exclude);
    cfg.moduleSelection.set(StudioModule::Core, ModuleChoice::Include);
    cfg.moduleSelection.set(StudioModule::Drums, ModuleChoice::Include);
    cfg.moduleSelection.set(StudioModule::Kick, ModuleChoice::Include);
    return cfg;
}

#ifndef RASGO_KICK_BASELINE
// Opt-in album mode, independent of the preceding-PCM fixtures. Observes
// completion in both real passes, not the unexposed calibration kick trace.
static void checkAlbum(const fs::path& root) {
    AlbumConfig album;
    album.albumSeed = 1466468054u;
    album.bpm = 120;
    album.trackCount = 2;
    album.maxDurationSeconds = 16;
    album.options.targetDurationSeconds = 24;
    album.options.musicalDirectionEnabled = true;
    album.expressiveActuationEnabled = true;
    album.organismStemAuditEnabled = false;
    album.bassKickInteractionEnabled = true;
    album.moduleSelection = config(album.albumSeed).moduleSelection;
    album.outDir = (root / "kick-album").string();
    const auto reportPath = root / "album-kick-report.json";
    assert(!fs::exists(album.outDir) && !fs::exists(reportPath));
    std::array<std::uint64_t, 2> calibrationFrames{}, finalFrames{};
    std::array<unsigned, 2> calibrationCompletions{}, finalCompletions{};
    auto results = renderAlbum(album, [&](int index, int count,
            const PieceProgress& progress, bool calibration) {
        assert(count == 2 && index >= 0 && index < count);
        if (progress.phase != PieceRenderPhase::Complete) return;
        assert(progress.completedFrames > 0);
        if (calibration) {
            assert(!fs::exists(album.outDir));
            assert(++calibrationCompletions.at(index) == 1);
            calibrationFrames.at(index) = progress.completedFrames;
        } else {
            assert(++finalCompletions.at(index) == 1);
            finalFrames.at(index) = progress.completedFrames;
            assert(finalFrames.at(index) == calibrationFrames.at(index));
        }
    });
    assert(results.size() == 2);
    std::ofstream report(reportPath);
    report << "{\"albumSeed\":" << album.albumSeed
        << ",\"shadowAudit\":false,\"calibrationTraceCompared\":false,\"tracks\":[";
    for (std::size_t index = 0; index < results.size(); ++index) {
        const auto& track = results.at(index);
        const auto& piece = track.piece;
        assert(calibrationCompletions.at(index) == 1 && finalCompletions.at(index) == 1);
        assert(piece.samples.size() / 2 == finalFrames.at(index));
        assert(!piece.path.empty() && fs::is_regular_file(piece.path));
        for (float sample : piece.samples) assert(std::isfinite(sample));
        assert(piece.info.bassKickInteraction);
        assert(piece.info.bassKickInteractionTriggers > 0);
        assert(piece.info.bassKickReducedFrames > 0
            && piece.info.bassKickReducedFrames <= finalFrames.at(index));
        assert(piece.info.bassCoreSubEventCount > 0);
        if (index) report << ',';
        report << "{\"seed\":" << track.trackSeed
            << ",\"calibrationFrames\":" << calibrationFrames.at(index)
            << ",\"finalFrames\":" << finalFrames.at(index)
            << ",\"acceptedKicks\":" << piece.info.bassKickInteractionTriggers
            << ",\"reducedBassFrames\":" << piece.info.bassKickReducedFrames << '}';
    }
    const auto folder = fs::path(results.front().piece.path).parent_path();
    assert(fs::is_regular_file(folder / "album_plan.json"));
    report << "]}\n";
    report.close();
    assert(report.good());
    std::cout << "PASS two-track kick/bass album: calibration/final frames match; "
        << "calibration creates no output, accepted kicks active, finite PCM\n";
}
#endif

int main(int argc, char** argv) {
#ifdef RASGO_KICK_BASELINE
    assert(argc == 2);
#else
    assert(argc == 2 || argc == 3);
#endif
    const fs::path root(argv[1]);
    assert(fs::is_directory(root));
#ifndef RASGO_KICK_BASELINE
    if (argc == 3) {
        assert(std::string(argv[2]) == "--album");
        checkAlbum(root);
        return 0;
    }
    const auto reportPath = root / "kick-bass-report.json";
    assert(!fs::exists(reportPath));
    std::ofstream report(reportPath);
    report << "{\"shadowAudit\":false,\"seeds\":[";
#endif
    unsigned index = 0;
    for (unsigned seed : {1466468054u, 2026u}) {
        auto cfg = config(seed);
        const auto reference = root / ("preceding-" + std::to_string(seed) + ".pcm");
#ifdef RASGO_KICK_BASELINE
        assert(!fs::exists(reference));
        auto piece = renderIntegratedPiece(cfg);
        assert(!piece.samples.empty());
        for (float sample : piece.samples) assert(std::isfinite(sample));
        std::ofstream file(reference, std::ios::binary);
        file.write(reinterpret_cast<const char*>(piece.samples.data()),
            static_cast<std::streamsize>(piece.samples.size() * sizeof(float)));
        assert(file.good());
        std::cout << "BASELINE seed=" << seed << " frames=" << piece.samples.size()/2 << '\n';
#else
        cfg.bassKickInteractionEnabled = false;
        cfg.recordOutput = true;
        cfg.outDir = (root / ("before-" + std::to_string(seed))).string();
        assert(!fs::exists(cfg.outDir));
        auto off = renderIntegratedPiece(cfg);
        assert(!off.info.bassKickInteraction && off.info.bassKickInteractionTriggers == 0);
        assert(fs::file_size(reference) == off.samples.size() * sizeof(float));
        std::vector<float> preceding(off.samples.size());
        std::ifstream file(reference, std::ios::binary);
        file.read(reinterpret_cast<char*>(preceding.data()),
            static_cast<std::streamsize>(preceding.size() * sizeof(float)));
        assert(file.good() && preceding == off.samples);
        preceding.clear(); preceding.shrink_to_fit();

        cfg.bassKickInteractionEnabled = true;
        cfg.outDir = (root / ("after-" + std::to_string(seed))).string();
        assert(!fs::exists(cfg.outDir));
        auto on = renderIntegratedPiece(cfg);
        assert(on.info.bassKickInteraction && on.info.bassKickInteractionTriggers > 0);
        assert(on.info.bassKickReducedFrames > 0 && on.info.bassCoreSubEventCount > 0);
        assert(on.samples.size() == off.samples.size());
        for (float sample : on.samples) assert(std::isfinite(sample));
        cfg.recordOutput = false;
        auto repeat = renderIntegratedPiece(cfg);
        assert(repeat.samples == on.samples);
        repeat.samples.clear(); repeat.samples.shrink_to_fit();
        double aa = 0, bb = 0, ab = 0;
        for (std::size_t i = 0; i < on.samples.size(); ++i) {
            aa += static_cast<double>(on.samples[i]) * on.samples[i];
            bb += static_cast<double>(off.samples[i]) * off.samples[i];
            ab += static_cast<double>(on.samples[i]) * off.samples[i];
        }
        const double gain = ab / std::max(bb, 1e-30);
        double residual = 0;
        for (std::size_t i = 0; i < on.samples.size(); ++i) {
            const double value = on.samples[i] - gain * off.samples[i];
            residual += value * value;
        }
        const double contrast = std::sqrt(residual / std::max(aa, 1e-30));
        assert(contrast > 1e-4);
        if (index++) report << ',';
        report << "{\"seed\":" << seed << ",\"frames\":" << on.samples.size()/2
            << ",\"acceptedKicks\":" << on.info.bassKickInteractionTriggers
            << ",\"reducedBassFrames\":" << on.info.bassKickReducedFrames
            << ",\"gainMatchedResidual\":" << contrast << '}';
        std::cout << "PASS exact preceding PCM bypass, repeat, contrast seed=" << seed
            << " residual=" << contrast << '\n';
#endif
    }
#ifndef RASGO_KICK_BASELINE
    // Direction/Expressive bypasses retain the legacy path exactly.
    for (bool expressive : {false, true}) {
        auto cfg = config(2026);
        cfg.targetDurationSeconds = 4;
        cfg.expressiveActuationEnabled = expressive;
        cfg.musicalDirectionEnabled = !expressive;
        auto on = renderIntegratedPiece(cfg);
        cfg.bassKickInteractionEnabled = false;
        auto off = renderIntegratedPiece(cfg);
        assert(!on.info.bassKickInteraction && on.samples == off.samples);
    }
    auto cfg = config(2026);
    cfg.targetDurationSeconds = 4;
    cfg.moduleSelection.set(StudioModule::Kick, ModuleChoice::Exclude);
    auto noKick = renderIntegratedPiece(cfg);
    assert(noKick.info.bassKickInteractionTriggers == 0 && noKick.info.bassKickReducedFrames == 0);
    AlbumConfig album;
    album.bassKickInteractionEnabled = false;
    auto track = detail::makeTrackConfig(album, root.string(), 42, 0, {}, .5);
    assert(!track.bassKickInteractionEnabled);
    album.bassKickInteractionEnabled = true;
    track = detail::makeTrackConfig(album, root.string(), 42, 0, {}, .5);
    assert(track.bassKickInteractionEnabled);
    report << "]}\n"; assert(report.good());
    std::cout << "PASS Direction/Expressive bypass, excluded kick neutral, album option propagation\n";
#endif
}
