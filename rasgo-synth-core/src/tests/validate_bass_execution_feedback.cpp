// Opt-in, serial renderer A/B. A fresh caller-owned directory contains only
// local evidence. Compile with RASGO_FEEDBACK_BASELINE before integration to
// preserve the preceding renderer's PCM; the regular build must match bypass.
#include "../engine/AlbumRenderer.hpp"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif

using namespace rasgo;
namespace fs = std::filesystem;

static PieceConfig config() {
    PieceConfig cfg;
    cfg.seed = 1466468054u;
    cfg.bpm = 120;
    cfg.targetDurationSeconds = 12;
    cfg.maxDurationSeconds = 16;
    cfg.recordOutput = false;
    cfg.probabilityMarketOverride = true;
    cfg.expressiveActuationEnabled = true;
    cfg.musicalDirectionEnabled = true;
    // Exercise the previously silent-family case, without a shadow mixer.
    cfg.organismStemAuditEnabled = false;
    // Isolate this observer from the subsequent kick/bass interaction.
    cfg.bassKickInteractionEnabled = false;
    for (auto module : {StudioModule::Acid, StudioModule::Tuba,
             StudioModule::Theremin, StudioModule::FmFlute,
             StudioModule::Galo, StudioModule::Footnote})
        cfg.moduleSelection.set(module, ModuleChoice::Exclude);
    cfg.moduleSelection.set(StudioModule::Core, ModuleChoice::Include);
    return cfg;
}

static void finite(const PieceResult& piece) {
    assert(!piece.samples.empty() && piece.path.empty());
    for (float sample : piece.samples) assert(std::isfinite(sample));
}

#ifndef RASGO_FEEDBACK_BASELINE
// Separate bounded mode: two real album passes, no reference PCM required.
// Completion frames are observed in both passes. Feedback counters below
// belong to final tracks, not unexposed calibration traces.
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
    album.moduleSelection = config().moduleSelection;
    album.outDir = (root / "feedback-album").string();
    const auto reportPath = root / "album-feedback-report.json";
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
        << ",\"shadowAudit\":false,\"tracks\":[";
    for (std::size_t index = 0; index < results.size(); ++index) {
        const auto& track = results[index];
        const auto& piece = track.piece;
        assert(calibrationCompletions.at(index) == 1 && finalCompletions.at(index) == 1);
        assert(piece.samples.size() / 2 == finalFrames.at(index));
        assert(!piece.path.empty() && fs::is_regular_file(piece.path));
        for (float sample : piece.samples) assert(std::isfinite(sample));
        assert(piece.info.bassExecutionFeedback && piece.info.bassFeedbackUpdates > 0);
        assert(piece.info.bassObservedTriggers == piece.info.bassCoreSubEventCount);
        assert(piece.info.bassObservedActiveFrames > 0
            && piece.info.bassObservedActiveFrames <= finalFrames.at(index));
        if (index) report << ',';
        report << "{\"seed\":" << track.trackSeed
            << ",\"calibrationFrames\":" << calibrationFrames.at(index)
            << ",\"finalFrames\":" << finalFrames.at(index)
            << ",\"updates\":" << piece.info.bassFeedbackUpdates
            << ",\"triggers\":" << piece.info.bassObservedTriggers << '}';
    }
    const auto folder = fs::path(results.front().piece.path).parent_path();
    assert(fs::is_regular_file(folder / "album_plan.json"));
    report << "]}\n";
    assert(report.good());
    std::cout << "PASS two-track feedback album: calibration/final frames match; "
        << "calibration creates no output, final counters active, finite PCM\n";
}
#endif

int main(int argc, char** argv) {
    assert(argc == 2 || argc == 3);
    const fs::path root(argv[1]);
    assert(fs::is_directory(root));
#ifndef RASGO_FEEDBACK_BASELINE
    if (argc == 3) {
        assert(std::string(argv[2]) == "--album");
        checkAlbum(root);
        return 0;
    }
#else
    assert(argc == 2);
#endif
    const auto reference = root / "baseline.pcm";
    auto cfg = config();
#ifdef RASGO_FEEDBACK_BASELINE
    assert(!fs::exists(reference));
    auto baseline = renderIntegratedPiece(cfg);
    finite(baseline);
    std::ofstream file(reference, std::ios::binary);
    file.write(reinterpret_cast<const char*>(baseline.samples.data()),
        static_cast<std::streamsize>(baseline.samples.size() * sizeof(float)));
    assert(file.good());
    std::cout << "BASELINE frames=" << baseline.samples.size() / 2
        << " bassEvents=" << baseline.info.bassCoreSubEventCount << '\n';
#else
    cfg.bassExecutionFeedbackEnabled = false;
    auto off = renderIntegratedPiece(cfg);
    finite(off);
    assert(!off.info.bassExecutionFeedback && off.info.bassFeedbackUpdates == 0);
    assert(fs::file_size(reference) == off.samples.size() * sizeof(float));
    std::vector<float> preceding(off.samples.size());
    std::ifstream file(reference, std::ios::binary);
    file.read(reinterpret_cast<char*>(preceding.data()),
        static_cast<std::streamsize>(preceding.size() * sizeof(float)));
    assert(file.good() && preceding == off.samples);
    preceding.clear(); preceding.shrink_to_fit();

    cfg.bassExecutionFeedbackEnabled = true;
    auto on = renderIntegratedPiece(cfg);
    finite(on);
    assert(on.info.bassExecutionFeedback && on.info.bassFeedbackUpdates > 0);
    assert(on.info.bassObservedTriggers > 0 && on.info.bassObservedActiveFrames > 0);
    assert(on.samples.size() == off.samples.size());
    auto repeat = renderIntegratedPiece(cfg);
    assert(on.samples == repeat.samples);
    assert(on.info.bassFeedbackUpdates == repeat.info.bassFeedbackUpdates);
    repeat.samples.clear(); repeat.samples.shrink_to_fit();
    double aa = 0, bb = 0, ab = 0;
    for (std::size_t i = 0; i < on.samples.size(); ++i) {
        aa += static_cast<double>(on.samples[i]) * on.samples[i];
        bb += static_cast<double>(off.samples[i]) * off.samples[i];
        ab += static_cast<double>(on.samples[i]) * off.samples[i];
    }
    const double fittedGain = bb > 0 ? ab / bb : 0;
    double residual = 0;
    for (std::size_t i = 0; i < on.samples.size(); ++i) {
        const double value = on.samples[i] - fittedGain * off.samples[i];
        residual += value * value;
    }
    const double relativeResidual = std::sqrt(residual / std::max(aa, 1e-30));
    assert(relativeResidual > 1e-4); // Not merely a constant master gain.
    std::ofstream report(root / "feedback-report.json");
    report << "{\"seed\":" << cfg.seed << ",\"shadowAudit\":false"
        << ",\"frames\":" << on.samples.size() / 2
        << ",\"updates\":" << on.info.bassFeedbackUpdates
        << ",\"triggers\":" << on.info.bassObservedTriggers
        << ",\"activeFrames\":" << on.info.bassObservedActiveFrames
        << ",\"relativeGainMatchedResidual\":" << relativeResidual << "}\n";
    assert(report.good());
    std::cout << "PASS legacy-exact bypass/repeat/A-B residual=" << relativeResidual
        << " updates=" << on.info.bassFeedbackUpdates << std::endl;
    on.samples.clear(); off.samples.clear();
    on.samples.shrink_to_fit(); off.samples.shrink_to_fit();

    cfg.targetDurationSeconds = 4;
    for (bool expressive : {true, false}) {
        cfg.expressiveActuationEnabled = expressive;
        cfg.musicalDirectionEnabled = !expressive;
        cfg.bassExecutionFeedbackEnabled = true;
        auto bypassOn = renderIntegratedPiece(cfg);
        cfg.bassExecutionFeedbackEnabled = false;
        auto bypassOff = renderIntegratedPiece(cfg);
        assert(!bypassOn.info.bassExecutionFeedback);
        assert(bypassOn.info.bassFeedbackUpdates == 0);
        assert(bypassOn.samples == bypassOff.samples);
    }
    AlbumConfig album;
    album.expressiveActuationEnabled = true;
    album.options.musicalDirectionEnabled = true;
    album.bassExecutionFeedbackEnabled = false;
    const detail::GameWeightPoint game{.25, .25, .25, .25};
    auto track = detail::makeTrackConfig(album, root.string(), 42, 0, game, .7);
    assert(!track.bassExecutionFeedbackEnabled);
    album.bassExecutionFeedbackEnabled = true;
    track = detail::makeTrackConfig(album, root.string(), 42, 0, game, .7);
    assert(track.bassExecutionFeedbackEnabled);
    std::cout << "PASS exact Direction/Expressive bypass and album option propagation\n";
#endif
}
