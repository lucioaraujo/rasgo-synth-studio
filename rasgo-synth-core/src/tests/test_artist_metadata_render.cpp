// Bounded, opt-in real piece/album regression. No device or playback.
#include "../engine/AlbumRenderer.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace fs = std::filesystem;
static void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
static std::string bytes(const fs::path& path) {
    require(fs::is_regular_file(path) && fs::file_size(path) < 16 * 1024 * 1024,
            "missing or oversized metadata fixture");
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
static uint32_t u32(const std::string& raw, size_t at) {
    require(at + 4 <= raw.size(), "truncated chunk size");
    uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i)
        value |= static_cast<uint32_t>(static_cast<unsigned char>(raw[at + i])) << (8 * i);
    return value;
}
static void check(const rasgo::PieceResult& result, const std::string& artist) {
    require(!result.path.empty(), "render did not publish a WAV");
    const auto path = fs::u8path(result.path);
    const auto raw = bytes(path);
    require(raw.substr(0, 4) == "RIFF" && raw.substr(8, 4) == "WAVE", "invalid WAV");
    unsigned found = 0;
    for (size_t at = 12; at + 8 <= raw.size();) {
        const auto size = u32(raw, at + 4);
        const auto end = at + 8 + size;
        require(end <= raw.size(), "truncated RIFF chunk");
        if (raw.substr(at, 4) == "LIST" && size >= 4 && raw.substr(at + 8, 4) == "INFO") {
            for (size_t sub = at + 12; sub + 8 <= end;) {
                const auto length = u32(raw, sub + 4);
                require(sub + 8 + length <= end, "truncated INFO field");
                if (raw.substr(sub, 4) == "IART") {
                    auto value = raw.substr(sub + 8, length);
                    while (!value.empty() && value.back() == '\0') value.pop_back();
                    require(value == artist, "WAV artist differs from user input");
                    ++found;
                }
                sub += 8 + length + (length & 1u);
            }
        }
        at = end + (size & 1u);
    }
    require(found == 1, "expected one IART field");
    auto score = path;
    score.replace_extension(".txt");
    require(bytes(score).find("Artista:     " + artist + "\n") != std::string::npos,
            "companion score artist differs from user input");
}
int main() {
    try {
        const auto root = fs::current_path() / ("artist-metadata-" + rasgo::RecordingPublication::uniqueToken());
        require(fs::create_directory(root), "test directory must be fresh");
        unsigned index = 0;
        for (const auto& artist : {std::string{}, std::string("Artista de teste"),
                                  std::string(u8"Música — création 日本語")}) {
            const auto folder = root / std::to_string(index++);
            rasgo::PieceConfig piece;
            piece.seed = 2026;
            piece.maxDurationSeconds = .01;
            piece.targetDurationSeconds = .05;
            piece.outDir = (folder / "piece").u8string();
            piece.artistOverride = artist;
            check(rasgo::renderIntegratedPiece(piece), artist);
            rasgo::AlbumConfig album;
            album.albumSeed = 2026;
            album.trackCount = 2;
            album.maxDurationSeconds = .01;
            album.options.targetDurationSeconds = 30;
            album.outDir = (folder / "album").u8string();
            album.artistOverride = artist;
            const auto tracks = rasgo::renderAlbum(album);
            require(tracks.size() == 2, "album must publish two tracks");
            for (const auto& track : tracks) check(track.piece, artist);
        }
        std::cout << "PASS: nine real WAVs/scores; empty, supplied and Unicode artists, piece and album\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
