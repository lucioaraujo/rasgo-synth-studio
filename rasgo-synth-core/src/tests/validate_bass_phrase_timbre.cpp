// Opt-in integration. Fresh caller-owned output root, <=12 s pieces and a
// 30 s planned album; external timeout/memory limits are mandatory.
#include "../engine/AlbumRenderer.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <numeric>

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif

namespace fs = std::filesystem;
using namespace rasgo;

static void ensureFinite(const PieceResult& piece) {
    assert(!piece.samples.empty());
    for (const auto sample : piece.samples) assert(std::isfinite(sample));
}
static PieceConfig testPiece(const fs::path& root) {
    PieceConfig cfg;
    cfg.seed = 2026; cfg.bpm = 120; cfg.maxDurationSeconds = 16;
    cfg.targetDurationSeconds = 12;
    cfg.recordOutput = false; cfg.outDir = (root / "memory-only").string();
    cfg.probabilityMarketOverride = true;
    cfg.expressiveActuationEnabled = true; cfg.musicalDirectionEnabled = true;
    cfg.moduleSelection.set(StudioModule::Acid, ModuleChoice::Exclude);
    cfg.moduleSelection.set(StudioModule::Tuba, ModuleChoice::Exclude);
    cfg.moduleSelection.set(StudioModule::Core, ModuleChoice::Include);
    cfg.moduleSelection.set(StudioModule::Drums, ModuleChoice::Include);
    cfg.moduleSelection.set(StudioModule::FmFlute, ModuleChoice::Include);
    return cfg;
}

static void checkPiece(const fs::path& root) {
    auto cfg = testPiece(root);
    cfg.recordOutput = true; cfg.outDir = (root / "A-active").string();
    auto active = renderIntegratedPiece(cfg);
    ensureFinite(active);
    assert(!active.path.empty() && active.info.bassPhraseTimbre);
    assert(active.info.bassCoreSubEventCount > 0 && active.info.bassPhraseTimbreFrames > 0);
    assert(std::accumulate(active.info.bassPhraseTimbreNotes.begin(),
        active.info.bassPhraseTimbreNotes.end(), std::uint64_t{0})
        == static_cast<std::uint64_t>(active.info.bassCoreSubEventCount));

    cfg.recordOutput = false; cfg.outDir = (root / "repeat-memory").string();
    auto repeat = renderIntegratedPiece(cfg);
    assert(repeat.samples == active.samples && !fs::exists(cfg.outDir));
    repeat.samples.clear(); repeat.samples.shrink_to_fit();

    cfg.recordOutput = true; cfg.bassPhraseTimbreEnabled = false;
    cfg.outDir = (root / "B-spectral-off").string();
    auto off = renderIntegratedPiece(cfg);
    ensureFinite(off);
    assert(!off.path.empty() && !off.info.bassPhraseTimbre);
    assert(off.info.bassCoreSubEventCount == active.info.bassCoreSubEventCount);
    assert(off.info.bassPhraseTimbreFrames == 0 && off.samples.size() == active.samples.size());
    double differenceEnergy = 0.0, activeEnergy = 0.0, offEnergy = 0.0, crossEnergy = 0.0;
    for (std::size_t i = 0; i < active.samples.size(); ++i) {
        const double difference = active.samples[i] - off.samples[i];
        differenceEnergy += difference * difference;
        activeEnergy += active.samples[i] * active.samples[i];
        offEnergy += off.samples[i] * off.samples[i];
        crossEnergy += active.samples[i] * off.samples[i];
    }
    const double differenceRms = std::sqrt(differenceEnergy / active.samples.size());
    const double activeRms = std::sqrt(activeEnergy / active.samples.size());
    assert(differenceRms > 1e-5 && activeRms > 1e-5);
    // Fit away any constant gain change: different master levels alone must
    // not qualify as evidence of a transformed final waveform.
    const double fittedGain = offEnergy > 0 ? crossEnergy / offEnergy : 0;
    double residualEnergy = 0;
    for (std::size_t i = 0; i < active.samples.size(); ++i) {
        const double residual = active.samples[i] - fittedGain * off.samples[i];
        residualEnergy += residual * residual;
    }
    const double residualRms = std::sqrt(residualEnergy / active.samples.size());
    assert(residualRms > 1e-5);
    std::ofstream report(root / "piece-report.json");
    report << "{\"seed\":2026,\"frames\":" << active.samples.size() / 2
        << ",\"coreSubNotes\":" << active.info.bassCoreSubEventCount
        << ",\"processedFrames\":" << active.info.bassPhraseTimbreFrames
        << ",\"differenceRms\":" << differenceRms << ",\"activeRms\":" << activeRms
        << ",\"relativeDifference\":" << differenceRms / activeRms
        << ",\"fittedGain\":" << fittedGain << ",\"gainMatchedResidualRms\":" << residualRms
        << ",\"relativeResidual\":" << residualRms / activeRms << "}\n";
    assert(report.good());
    std::cout << "PASS A/B: notes=" << active.info.bassCoreSubEventCount
        << " frames=" << active.info.bassPhraseTimbreFrames
        << " post-mix difference RMS=" << differenceRms
        << " relative=" << differenceRms / activeRms
        << " gain-matched residual=" << residualRms / activeRms << '\n';
}

static void checkBypass(const fs::path& root) {
    auto cfg = testPiece(root);
    cfg.targetDurationSeconds = 4;
    cfg.musicalDirectionEnabled = false;
    auto on = renderIntegratedPiece(cfg);
    cfg.bassPhraseTimbreEnabled = false;
    auto off = renderIntegratedPiece(cfg);
    assert(on.samples == off.samples && !on.info.bassPhraseTimbre);
    ensureFinite(on);
    on.samples.clear(); off.samples.clear();
    on.samples.shrink_to_fit(); off.samples.shrink_to_fit();
    cfg.musicalDirectionEnabled = true; cfg.expressiveActuationEnabled = false;
    cfg.bassPhraseTimbreEnabled = true;
    on = renderIntegratedPiece(cfg);
    cfg.bassPhraseTimbreEnabled = false;
    off = renderIntegratedPiece(cfg);
    assert(on.samples == off.samples && !on.info.bassPhraseTimbre);
    assert(!fs::exists(cfg.outDir));
    std::cout << "PASS exact bypass PCM without Direction and without Expressive\n";
}

static void checkAlbum(const fs::path& root) {
    AlbumConfig cfg;
    cfg.albumSeed = 2026; cfg.trackCount = 2; cfg.maxDurationSeconds = 16;
    cfg.options.targetDurationSeconds = 30;
    cfg.options.musicalDirectionEnabled = true; cfg.expressiveActuationEnabled = true;
    cfg.options.varyBpm = true;
    cfg.outDir = (root / "album").string();
    cfg.moduleSelection = testPiece(root).moduleSelection;
    std::array<std::uint64_t, 2> calibrationFrames{};
    auto results = renderAlbum(cfg, [&](int index, int, const PieceProgress& p, bool calibration) {
        if (p.phase != PieceRenderPhase::Complete) return;
        if (calibration) {
            assert(!fs::exists(cfg.outDir));
            calibrationFrames.at(index) = p.completedFrames;
        } else assert(calibrationFrames.at(index) == p.completedFrames);
    });
    assert(results.size() == 2);
    unsigned notes = 0;
    for (const auto& track : results) {
        ensureFinite(track.piece);
        assert(!track.piece.path.empty() && track.piece.info.bassPhraseTimbre);
        notes += track.piece.info.bassCoreSubEventCount;
    }
    assert(notes > 0);
    const auto folder = fs::path(results[0].piece.path).parent_path();
    assert(fs::exists(folder / "album_plan.json"));
    // Both calibration/final configs must carry the independent A/B switch.
    cfg.bassPhraseTimbreEnabled = false;
    auto pc = detail::makeTrackConfig(cfg, "unused", 2026, 0, {}, .5);
    assert(!pc.bassPhraseTimbreEnabled);
    std::cout << "PASS expressive album: calibration/final frames, core-sub notes=" << notes << '\n';
}

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    const std::string mode = argv[2];
    if (mode != "piece" && mode != "bypass" && mode != "album") return 2;
    const fs::path root(argv[1]);
    if (fs::exists(root) || !fs::create_directory(root)) return 2;
    if (mode == "piece") checkPiece(root);
    else if (mode == "bypass") checkBypass(root);
    else checkAlbum(root);
}
