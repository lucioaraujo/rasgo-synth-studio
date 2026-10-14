// Small native Windows/macOS/Linux regression; no synthesis or audio device.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../io/Recorder.hpp"
#include "../io/PublishedScore.hpp"
#include "../io/PublishedText.hpp"
#include "../engine/ProbabilityMarket.hpp"
#include "../../../rasgo-synth-app/src/PortableTextFormat.h"
#include <cassert>
#include <iostream>
#include <iterator>

namespace fs = std::filesystem;
static std::string bytes(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
int main(int argc, char** argv) {
    if (argc > 2) return 2;
    const auto root = argc == 2 ? fs::u8path(argv[1]) : fs::current_path()
        / ("native-portability-" + rasgo::RecordingPublication::uniqueToken());
    if (fs::exists(root) || !fs::create_directory(root)) return 2;
    const auto directory = root / fs::u8path(u8"gravações-é_écritures_日本語");
    rasgo::Recorder recorder(directory.u8string());
    rasgo::WavMetadata meta;
    meta.artist = u8"ação — création";
    meta.title = u8"música";
    meta.comment = "{\"seed\":7}";
    const std::vector<float> samples(128, .125f);
    const auto first = recorder.record("piece", samples, 2, 48000, meta);
    const auto second = recorder.record("piece", samples, 2, 48000, meta);
    assert(!first.empty() && first != second);
    assert(fs::is_regular_file(fs::u8path(first)));
    assert(fs::is_regular_file(fs::u8path(second)));
    assert(bytes(fs::u8path(first)).find(meta.artist) != std::string::npos);
    const auto index = bytes(directory / "rasgo_index.jsonl");
    assert(index.find(fs::u8path(first).filename().u8string()) != std::string::npos);
    assert(index.find(fs::u8path(second).filename().u8string()) != std::string::npos);
    assert(index.find(directory.filename().u8string()) != std::string::npos);
    rasgo::publishRecordingScore(fs::u8path(first), meta.artist);
    auto score = fs::u8path(first); score.replace_extension(".txt");
    assert(bytes(score) == meta.artist);
    assert(rasgo::publishTextNoReplace(directory / "album_plan.json", "{\"tracks\":2}"));
    const auto marketPath = (directory / ".rasgo_market.txt").u8string();
    rasgo::ProbabilityMarket market; market.price("utf8_test", .42); market.save(marketPath);
    rasgo::ProbabilityMarket loaded; loaded.load(marketPath);
    assert(loaded.price("utf8_test", .01) == .42);
    loaded.save(marketPath); // replacement must preserve the native path too
    const auto formatted = rasgo::ui::portableTextFormat("%s | %u | %.2f", meta.artist.c_str(), 7u, .5);
    assert(formatted == meta.artist + " | 7 | 0.50");
    assert(rasgo::ui::portableTextFormat("%s", std::string(9000, 'x').c_str()).empty());
    for (const auto& entry : fs::directory_iterator(directory))
        assert(entry.path().filename().u8string().find(".pending-") == std::string::npos);
    std::cout << "PASS: native UTF-8 WAV/index/score/manifest/market and portable text formatting\n";
    // Retain this tiny, owned fixture as evidence, outside source publication.
}
