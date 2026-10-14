// Opt-in integration check; not in default CTest. Writes bounded local audio.
#include "../engine/AlbumRenderer.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <set>

int main(int argc, char** argv) {
    if (argc != 2) return 2; // caller supplies an isolated local output root
    const std::string root = argv[1];
    rasgo::PieceConfig config;
    config.seed = 2026u;
    config.maxDurationSeconds = 0.01; // one opening section + coda, not a long batch
    config.outDir = root + "/without-callback";
    auto baseline = rasgo::renderIntegratedPiece(config);
    config.outDir = root + "/with-callback";
    int callbacks = 0;
    std::uint64_t finalFrames = 0, plannedFrames = 0;
    auto observed = rasgo::renderIntegratedPiece(config, [&](const rasgo::PieceProgress& progress) {
        ++callbacks;
        assert(progress.t >= 0.0 && progress.t <= 1.0);
        if (progress.phase == rasgo::PieceRenderPhase::Complete) {
            assert(progress.t == 1.0);
            finalFrames = progress.completedFrames; plannedFrames = progress.totalFrames;
        }
    });
    assert(!baseline.path.empty() && !observed.path.empty());
    assert(callbacks > 2 && finalFrames == plannedFrames && finalFrames == observed.samples.size() / 2);
    assert(baseline.samples == observed.samples); // options off: counted progress has no PCM consequence
    baseline.samples.clear(); baseline.samples.shrink_to_fit();
    observed.samples.clear(); observed.samples.shrink_to_fit();

    rasgo::AlbumConfig album;
    album.albumSeed = 2026u; album.trackCount = 2;
    album.outDir = root + "/album";
    album.options.varyBpm = true;
    album.options.tempoContour = rasgo::AlbumTempoContour::Contrasting;
    album.options.targetDurationSeconds = 50.0;
    album.options.durationDistribution = rasgo::AlbumDurationDistribution::Contrasting;
    const auto plan = rasgo::makeAlbumPlan(album.albumSeed, album.trackCount, album.bpm, album.options);
    std::vector<std::uint64_t> calibrationFrames(2), finalTrackFrames(2);
    const auto results = rasgo::renderAlbum(album, [&](int index, int, const rasgo::PieceProgress& progress, bool calibration) {
        if (progress.phase == rasgo::PieceRenderPhase::Complete) {
            assert(progress.completedFrames == progress.totalFrames);
            (calibration ? calibrationFrames : finalTrackFrames)[static_cast<std::size_t>(index)] = progress.completedFrames;
        }
    });
    assert(results.size() == 2);
    std::set<std::string> paths;
    double sum = 0.0;
    for (std::size_t index = 0; index < results.size(); ++index) {
        const auto& piece = results[index].piece;
        assert(!piece.path.empty());
        assert(paths.insert(piece.path).second); // short tracks cannot collide by second
        assert(!piece.tempoTimeline.empty() && piece.tempoTimeline.front().bpm == plan.tracks[index].bpm);
        const double seconds = piece.samples.size() / 2.0 / piece.sampleRate;
        assert(std::abs(seconds - plan.tracks[index].targetDurationSeconds) <= 0.25);
        assert(std::abs(calibrationFrames[index] / 48000.0 - plan.tracks[index].targetDurationSeconds) <= 0.25);
        assert(finalTrackFrames[index] == piece.samples.size() / 2);
        for (float sample : piece.samples) assert(std::isfinite(sample) && std::abs(sample) <= 1.0f);
        sum += seconds;
        std::ifstream file(piece.path, std::ios::binary); assert(file.good());
        std::cout << "Track " << index + 1 << ": " << piece.tempoTimeline.front().bpm << " BPM, " << seconds << " seconds\n";
    }
    assert(std::abs(sum - plan.effectiveDurationSeconds) <= 0.5);
    const auto folder = results.front().piece.path.substr(0, results.front().piece.path.find_last_of('/'));
    std::ifstream manifest(folder + "/album_plan.json"); assert(manifest.good());
    std::cout << "Callback PCM invariance; real album BPM, duration, finite levels, distinct files, manifest: PASS\n";
}
