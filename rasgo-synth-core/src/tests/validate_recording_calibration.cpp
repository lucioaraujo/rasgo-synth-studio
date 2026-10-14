// Opt-in, bounded integration check. Only writes below a new caller-owned root.
// Not a default CTest render, not listening or a complete release regression.
#include "../engine/AlbumRenderer.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
static void checkInterruptedAlbum(const fs::path& root) {
    rasgo::AlbumConfig album;
    album.albumSeed = 2026; album.trackCount = 2; album.maxDurationSeconds = .01;
    album.options.targetDurationSeconds = 30.0;
    album.options.varyBpm = true; album.options.musicalDirectionEnabled = true;
    album.outDir = (root / "interrupted-album").string();
    bool interrupted = false;
    try {
        rasgo::renderAlbum(album, [](int index, int, const rasgo::PieceProgress&, bool calibration) {
            if (!calibration && index == 1) throw std::runtime_error("owned checkpoint cancellation");
        });
    } catch (const std::runtime_error& error) {
        interrupted = std::string(error.what()) == "owned checkpoint cancellation";
    }
    assert(interrupted);
    fs::path partial;
    for (const auto& entry : fs::directory_iterator(album.outDir)) {
        assert(partial.empty() && entry.is_directory()); partial = entry.path();
    }
    assert(!partial.empty() && !fs::exists(partial / "album_plan.json"));
    unsigned partialWavs = 0;
    for (const auto& entry : fs::directory_iterator(partial))
        if (entry.path().extension() == ".wav") { ++partialWavs; assert(fs::file_size(entry.path()) > 44); }
    assert(partialWavs == 1);
    std::ifstream partialIndex(partial / "rasgo_index.jsonl"); std::string line;
    assert(std::getline(partialIndex, line) && !std::getline(partialIndex, line));
    std::cout << "PASS: second-track checkpoint cancellation preserves first recording/index, no complete manifest\n";
}
int main(int argc, char** argv) {
    if (argc != 2 && (argc != 3 || std::string(argv[2]) != "partial")) return 2;
    const fs::path root(argv[1]);
    if (fs::exists(root) || !fs::create_directory(root)) return 2;
    if (argc == 3) { checkInterruptedAlbum(root); return 0; }
    rasgo::PieceConfig piece;
    piece.seed = 2026; piece.maxDurationSeconds = .01; piece.targetDurationSeconds = 4.0;
    piece.outDir = (root / "memory").string(); piece.recordOutput = false;
    auto probe = rasgo::renderIntegratedPiece(piece);
    assert(probe.path.empty() && !probe.samples.empty() && !fs::exists(piece.outDir));
    piece.recordOutput = true;
    auto recorded = rasgo::renderIntegratedPiece(piece);
    assert(!recorded.path.empty() && probe.samples == recorded.samples);
    assert(fs::exists(fs::path(recorded.path).replace_extension(".txt")));
    assert(fs::exists(fs::path(piece.outDir) / "rasgo_index.jsonl"));
    assert(fs::exists(fs::path(piece.outDir) / ".rasgo_market.txt"));
    probe.samples.clear(); probe.samples.shrink_to_fit();
    recorded.samples.clear(); recorded.samples.shrink_to_fit();

    // Next-track history is frozen for probe/final instead of advancing it
    // twice or carrying transient fired flags through the continuation.
    piece.outDir = (root / "frozen-market").string();
    piece.probabilityMarketOverride = true;
    piece.probabilityMarketInput = probe.nextProbabilityMarket;
    piece.recordOutput = false;
    auto nextProbe = rasgo::renderIntegratedPiece(piece);
    assert(!fs::exists(piece.outDir));
    piece.recordOutput = true;
    auto nextFinal = rasgo::renderIntegratedPiece(piece);
    assert(!nextFinal.path.empty() && nextProbe.samples == nextFinal.samples);
    nextProbe.samples.clear(); nextProbe.samples.shrink_to_fit();
    nextFinal.samples.clear(); nextFinal.samples.shrink_to_fit();

    // A refused destination cannot persist market state or advertise a recording.
    std::ofstream(root / "blocked") << "owned test sentinel";
    piece.outDir = (root / "blocked").string();
    auto refused = rasgo::renderIntegratedPiece(piece);
    assert(refused.path.empty());
    std::ifstream sentinel(root / "blocked"); std::string content;
    std::getline(sentinel, content); assert(content == "owned test sentinel");
    refused.samples.clear(); refused.samples.shrink_to_fit();

    rasgo::AlbumConfig album;
    album.albumSeed = 2026; album.trackCount = 2; album.maxDurationSeconds = .01;
    album.options.targetDurationSeconds = 30.0;
    album.options.varyBpm = true; album.options.musicalDirectionEnabled = true;
    album.outDir = (root / "album").string();
    unsigned calibrationDone = 0;
    auto tracks = rasgo::renderAlbum(album, [&](int, int, const rasgo::PieceProgress& progress, bool calibration) {
        if (calibration && progress.phase == rasgo::PieceRenderPhase::Complete) {
            ++calibrationDone;
            assert(!fs::exists(album.outDir)); // no WAV/index/score/market directory during probes
        }
    });
    assert(calibrationDone == 2 && tracks.size() == 2);
    const auto folder = fs::path(tracks[0].piece.path).parent_path();
    assert(!tracks[0].piece.path.empty() && !tracks[1].piece.path.empty());
    assert(tracks[0].piece.path != tracks[1].piece.path);
    unsigned wavs = 0, scores = 0;
    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.path().extension() == ".wav") ++wavs;
        if (entry.path().extension() == ".txt" && entry.path().filename() != ".rasgo_market.txt") ++scores;
        assert(entry.path().filename().string().find(".pending-") == std::string::npos);
    }
    assert(wavs == 2 && scores == 2 && fs::exists(folder / "album_plan.json"));
    std::ifstream index(folder / "rasgo_index.jsonl");
    std::string line; unsigned entries = 0;
    while (std::getline(index, line)) {
        assert(entries < tracks.size());
        assert(line.find("\"outputTruePeakMethod\":\"" + std::string(rasgo::TruePeakDetector::methodId()) + "\"")
               != std::string::npos);
        assert(line.find("\"deliveryTruePeakTargetDbtp\":-1.8") != std::string::npos);
        assert(line.find("\"publicTruePeakCeilingDbtp\":-1.0") != std::string::npos);
        assert(line.find(tracks[entries].piece.path) != std::string::npos);
        assert(fs::exists(tracks[entries].piece.path));
        ++entries;
    }
    assert(entries == tracks.size());
    for (const auto& track : tracks)
        for (const auto sample : track.piece.samples) assert(std::isfinite(sample));

    std::cout << "PASS: in-memory calibration leaves no artifacts, piece PCM unchanged, "
                 "frozen next-track PCM, refused output preserves sentinel, exactly two album WAVs/scores/index entries\n";
}
