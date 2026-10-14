#include "../io/PublishedScore.hpp"
#include "../io/Recorder.hpp"
#include <array>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <thread>
#include <vector>
#ifdef __unix__
#include <csignal>
#include <sys/resource.h>
#endif

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif

namespace fs = std::filesystem;
static std::string bytes(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
static fs::path scorePath(fs::path recording) { return recording.replace_extension(".txt"); }
static void noPending(const fs::path& root) {
    for (const auto& entry : fs::directory_iterator(root))
        assert(entry.path().filename().string().find(".pending-") == std::string::npos);
}
static void failurePreservesRecording(const fs::path& recording, const std::string& text) {
    const auto before = bytes(recording);
    const auto index = recording.parent_path() / "rasgo_index.jsonl";
    const auto indexBefore = bytes(index);
    bool refused = false;
    try { rasgo::publishRecordingScore(recording, text); }
    catch (const std::runtime_error& error) {
        const std::string message(error.what());
        refused = message.find("Score publication failed") != std::string::npos
            && message.find(recording.string()) != std::string::npos
            && message.find(recording.parent_path().string()) != std::string::npos;
    }
    assert(refused && bytes(recording) == before && bytes(index) == indexBefore);
    noPending(recording.parent_path());
}
int main(int argc, char** argv) {
    if (argc > 2) return 2;
    const auto root = argc == 2 ? fs::path(argv[1]) : fs::current_path()
        / ("score-publication-" + rasgo::RecordingPublication::uniqueToken());
    if (fs::exists(root) || !fs::create_directory(root)) return 2;
    const auto recordings = root / u8"École — coração";
    rasgo::Recorder recorder(recordings.string());
    rasgo::WavMetadata meta; meta.date = "2026-10-11"; meta.comment = "{\"seed\":2026}";
    const std::vector<float> samples(256, .125f);
    auto record = [&](const char* name) {
        const auto path = recorder.record(name, samples, 2, 48000, meta);
        assert(!path.empty());
        return fs::path(path);
    };
    const std::string text = u8"RASGO SYNTH -- partitura gerativa\nTítulo: École — coração\n";
    const auto normal = record("normal");
    const auto normalWav = bytes(normal);
    const auto indexBefore = bytes(recordings / "rasgo_index.jsonl");
    rasgo::publishRecordingScore(normal, text);
    assert(bytes(scorePath(normal)) == text && bytes(normal) == normalWav);
    assert(bytes(recordings / "rasgo_index.jsonl") == indexBefore);
    failurePreservesRecording(normal, "replacement");
    assert(bytes(scorePath(normal)) == text);

    const auto occupied = record("occupied");
    assert(fs::create_directory(scorePath(occupied)));
    failurePreservesRecording(occupied, text);
    assert(fs::is_directory(scorePath(occupied)));

    const auto oversized = record("oversized");
    failurePreservesRecording(oversized, std::string(4 * 1024 * 1024 + 1, 'x'));
    assert(!fs::exists(scorePath(oversized)));

    bool invalid = false;
    try { rasgo::publishRecordingScore(recordings / "missing.wav", text); }
    catch (const std::invalid_argument&) { invalid = true; }
    assert(invalid && !fs::exists(recordings / "missing.txt"));

    const auto race = record("race");
    std::array<bool, 8> winners{};
    std::array<std::string, 8> diagnostics{};
    std::vector<std::thread> workers;
    for (std::size_t i = 0; i < winners.size(); ++i) workers.emplace_back([&, i] {
        try { rasgo::publishRecordingScore(race, std::to_string(i)); winners[i] = true; }
        catch (const std::runtime_error& error) { diagnostics[i] = error.what(); }
    });
    for (auto& worker : workers) worker.join();
    unsigned count = 0;
    for (std::size_t i = 0; i < winners.size(); ++i) {
        if (winners[i]) { ++count; assert(bytes(scorePath(race)) == std::to_string(i)); }
        else assert(diagnostics[i].find(race.string()) != std::string::npos);
    }
    assert(count == 1 && bytes(race) == normalWav);
    noPending(recordings);
#ifdef __unix__
    const auto linked = record("linked");
    fs::create_symlink(fs::absolute(scorePath(normal)), scorePath(linked));
    failurePreservesRecording(linked, "replacement");
    assert(fs::is_symlink(scorePath(linked)) && bytes(scorePath(normal)) == text);

    const auto writeFailure = record("write_failure");
    rlimit previous{};
    assert(::getrlimit(RLIMIT_FSIZE, &previous) == 0);
    auto limited = previous;
    limited.rlim_cur = 16;
    const auto handler = std::signal(SIGXFSZ, SIG_IGN);
    assert(::setrlimit(RLIMIT_FSIZE, &limited) == 0);
    failurePreservesRecording(writeFailure, std::string(256, 'x'));
    assert(::setrlimit(RLIMIT_FSIZE, &previous) == 0);
    std::signal(SIGXFSZ, handler);
    assert(!fs::exists(scorePath(writeFailure)));
#endif
    std::cout << "PASS score publication: exact UTF-8, preserved WAV/index, explicit recovery error, "
                 "no-replace file/directory/symlink, bounded size, 8 concurrent callers, write failure\n";
}
