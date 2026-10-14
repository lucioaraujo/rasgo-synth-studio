// Opt-in offline regression: only a fresh, caller-owned evidence directory.
#include "../engine/AlbumRenderer.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace fs = std::filesystem;
static std::string bytes(const fs::path& file) {
    std::ifstream input(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
int main(int argc, char** argv) {
    if (argc != 3) return 2;
    const std::string mode(argv[2]);
    if (mode != "normal" && mode != "collision" && mode != "directory"
        && mode != "index-partial" && mode != "score-failure") return 2;
    const fs::path root(argv[1]);
    if (fs::exists(root) || !fs::create_directory(root)) return 2;
    rasgo::AlbumConfig album;
    album.albumSeed = 2026;
    album.trackCount = 2;
    album.maxDurationSeconds = .01;
    album.options.targetDurationSeconds = 30;
    album.outDir = (root / "album").string();
    const std::string sentinel = "owned pre-existing manifest sentinel\n";
    fs::path folder;
    bool injected = false, refused = false;
    std::string error;
    std::string partialIndex, marketBefore;
    unsigned completed = 0;
    std::vector<rasgo::AlbumTrackResult> tracks;
    try {
        tracks = rasgo::renderAlbum(album, [&](int index, int,
                const rasgo::PieceProgress& progress, bool calibration) {
            if (calibration) return;
            if (progress.phase == rasgo::PieceRenderPhase::Complete) ++completed;
            if (index == 0 && progress.phase == rasgo::PieceRenderPhase::Complete) {
                for (const auto& entry : fs::directory_iterator(album.outDir)) {
                    if (!folder.empty() || !entry.is_directory()) throw std::runtime_error("unexpected test folder");
                    folder = entry.path();
                }
                if (folder.empty()) throw std::runtime_error("missing test folder");
                marketBefore = bytes(folder / ".rasgo_market.txt");
                if (mode == "index-partial") {
                    const auto target = folder / "rasgo_index.jsonl";
                    if (fs::file_size(target) > 2 * 1024 * 1024) throw std::runtime_error("test index budget");
                    partialIndex = bytes(target) + "owned incomplete tail";
                    std::ofstream file(target, std::ios::binary | std::ios::app);
                    file << "owned incomplete tail"; file.close();
                    if (!file.good()) throw std::runtime_error("partial-index fixture failed");
                    injected = true;
                }
            }
            if (mode == "score-failure" && index == 1 && !injected && !folder.empty()
                && progress.phase == rasgo::PieceRenderPhase::Finalizing) {
                unsigned wavs = 0;
                fs::path missingScore;
                for (const auto& entry : fs::directory_iterator(folder)) {
                    if (entry.path().extension() != ".wav") continue;
                    ++wavs;
                    auto score = entry.path(); score.replace_extension(".txt");
                    if (!fs::exists(score)) missingScore = score;
                }
                if (wavs == 2 && !missingScore.empty()) {
                    injected = fs::create_directory(missingScore);
                    if (!injected) throw std::runtime_error("score sentinel directory failed");
                }
            }
            if (index != 1 || progress.phase != rasgo::PieceRenderPhase::Complete) return;
            if (mode == "collision") {
                std::ofstream file(folder / "album_plan.json", std::ios::binary);
                file << sentinel;
                file.close();
                if (!file.good()) throw std::runtime_error("sentinel write failed");
                injected = true;
            } else if (mode == "directory") {
                injected = fs::create_directory(folder / "album_plan.json");
                if (!injected) throw std::runtime_error("sentinel directory failed");
            }
        });
    } catch (const std::runtime_error& failure) {
        refused = true;
        error = failure.what();
    }
    if (folder.empty()) return 1;
    unsigned wavs = 0, scores = 0, entries = 0;
    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.path().extension() == ".wav") {
            ++wavs;
            if (fs::file_size(entry.path()) <= 44) return 1;
        }
        if (entry.is_regular_file() && entry.path().extension() == ".txt"
            && entry.path().filename() != ".rasgo_market.txt") ++scores;
        const auto name = entry.path().filename().string();
        if (name.find(".pending-") != std::string::npos || name.find(".lock") != std::string::npos) return 1;
    }
    std::ifstream index(folder / "rasgo_index.jsonl");
    std::string line;
    while (std::getline(index, line)) ++entries;
    const bool auxiliaryFailure = mode == "index-partial" || mode == "score-failure";
    if (wavs != 2 || scores != (auxiliaryFailure ? 1u : 2u) || entries != 2) return 1;
    if (auxiliaryFailure) {
        const auto expected = mode == "index-partial" ? "previous index has an incomplete last line" : "Score publication failed";
        if (!injected || !refused || completed != 1 || error.find(expected) == std::string::npos
            || error.find(folder.string()) == std::string::npos || fs::exists(folder / "album_plan.json")
            || bytes(folder / ".rasgo_market.txt") != marketBefore) return 1;
        if (mode == "index-partial" && bytes(folder / "rasgo_index.jsonl") != partialIndex) return 1;
        if (mode == "score-failure") {
            unsigned directories = 0;
            for (const auto& entry : fs::directory_iterator(folder))
                if (entry.is_directory() && entry.path().extension() == ".txt") ++directories;
            if (directories != 1) return 1;
        }
        std::cout << "PASS album " << mode << ": completed WAVs retained, first score/market preserved, "
                     "no second Complete/album manifest, explicit recovery error\n";
        return 0;
    }
    if (mode == "normal") {
        if (refused || tracks.size() != 2 || !fs::is_regular_file(folder / "album_plan.json")) return 1;
        const auto manifest = bytes(folder / "album_plan.json");
        if (manifest.find("\"schemaVersion\":1") == std::string::npos) return 1;
        for (const auto& track : tracks)
            if (manifest.find(fs::path(track.piece.path).filename().string()) == std::string::npos) return 1;
    } else {
        const bool preserved = mode == "collision" ? bytes(folder / "album_plan.json") == sentinel
            : fs::is_directory(folder / "album_plan.json");
        if (!injected || !refused || !preserved || error.find(folder.string()) == std::string::npos) {
            std::cerr << "FAIL: manifest destination not preserved or failure silently accepted\n";
            return 1;
        }
    }
    std::cout << "PASS album manifest " << mode << ": two WAVs/scores/index entries preserved; "
        << (mode == "normal" ? "completed manifest published" : "failure reported with recovery folder") << '\n';
}
