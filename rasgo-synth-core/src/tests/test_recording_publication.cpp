#include "../io/Recorder.hpp"
#include <cassert>
#include <array>
#include <iostream>
#include <iterator>
#include <set>
#include <thread>
#ifdef __unix__
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;
static std::string bytes(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
static void noPending(const fs::path& root) {
    for (const auto& entry : fs::directory_iterator(root))
        assert(entry.path().filename().string().find(".pending-") == std::string::npos);
}
int main(int argc, char** argv) {
    if (argc > 2) return 2;
    const auto root = argc == 2 ? fs::path(argv[1]) : fs::current_path()
        / ("recording-publication-" + rasgo::RecordingPublication::uniqueToken());
    if (fs::exists(root) || !fs::create_directory(root)) return 2;
    const std::vector<float> samples(256, .125f);
    rasgo::WavMetadata meta; meta.date = "2026-10-08"; meta.comment = "{\"seed\":2026}";
    assert(rasgo::writeWavFile((root / "reference.wav").string(), samples, 2, 48000, meta));
    {
        rasgo::RecordingPublication staged(root / "complete.wav");
        assert(staged.ready() && !fs::exists(root / "complete.wav"));
        assert(rasgo::writeWavFile(staged.temporaryPath().string(), samples, 2, 48000, meta));
        assert(!fs::exists(root / "complete.wav")); // never visible before close+publish
        assert(staged.publish());
        assert(bytes(root / "complete.wav") == bytes(root / "reference.wav"));
        assert(!staged.publish()); // no replacement even on repeated commit
    }
    {
        rasgo::RecordingPublication collision(root / "complete.wav");
        assert(collision.ready());
        assert(rasgo::writeWavFile(collision.temporaryPath().string(), samples, 2, 44100, meta));
        assert(!collision.publish());
        assert(bytes(root / "complete.wav") == bytes(root / "reference.wav"));
    }
    fs::create_directory(root / "occupied.wav");
    {
        rasgo::RecordingPublication collision(root / "occupied.wav");
        assert(collision.ready());
        assert(rasgo::writeWavFile(collision.temporaryPath().string(), samples, 2, 48000, meta));
        assert(!collision.publish() && fs::is_directory(root / "occupied.wav"));
    }
    {
        rasgo::RecordingPublication cancelled(root / "cancelled.wav");
        assert(cancelled.ready());
        assert(rasgo::writeWavFile(cancelled.temporaryPath().string(), samples, 2, 48000, meta));
    }
    assert(!fs::exists(root / "cancelled.wav"));
    noPending(root);
    // Unique names during a tight same-name batch and simultaneous workers.
    const auto recorded = root / "recorded";
    rasgo::Recorder recorder(recorded.string());
    std::set<std::string> paths;
    for (unsigned i = 0; i < 32; ++i) {
        auto path = recorder.record("same", samples, 2, 48000, meta);
        assert(!path.empty() && paths.insert(path).second);
        assert(bytes(path) == bytes(root / "reference.wav"));
    }
    std::array<std::string, 8> concurrentPaths;
    std::vector<std::thread> threads;
    for (std::size_t i = 0; i < concurrentPaths.size(); ++i)
        threads.emplace_back([&, i] {
            rasgo::Recorder worker(recorded.string());
            concurrentPaths[i] = worker.record("same", samples, 2, 48000, meta);
        });
    for (auto& thread : threads) thread.join();
    for (const auto& path : concurrentPaths) {
        assert(!path.empty() && paths.insert(path).second);
        assert(bytes(path) == bytes(root / "reference.wav"));
    }
    assert(recorder.record("../escape", samples, 2, 48000, meta).empty());
    assert(recorder.record("invalid", samples, 0, 48000, meta).empty());
    assert(recorder.record("invalid", samples, 2, 0, meta).empty());
    assert(recorder.record("invalid", {0.1f}, 2, 48000, meta).empty());
    noPending(recorded);
    const auto quoted = root / "quoted\"folder";
    rasgo::Recorder escaping(quoted.string());
    assert(!escaping.record("same", samples, 2, 48000, meta).empty());
    assert(bytes(quoted / "rasgo_index.jsonl").find("quoted\\\"folder") != std::string::npos);
#ifdef __unix__
    // This owned child simulates abrupt loss before publication. Its private
    // pending file is retained as evidence; no incomplete final WAV appears.
    const auto child = ::fork();
    if (child < 0) return 2;
    if (child == 0) {
        rasgo::RecordingPublication interrupted(root / "interrupted.wav");
        if (!interrupted.ready() || !rasgo::writeWavFile(
            interrupted.temporaryPath().string(), samples, 2, 48000, meta)) ::_exit(2);
        ::_exit(0);
    }
    int status = 0;
    if (::waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0) return 2;
    assert(!fs::exists(root / "interrupted.wav"));
    // An existing symlink also stays untouched.
    fs::create_symlink(root / "reference.wav", root / "linked.wav");
    {
        rasgo::RecordingPublication collision(root / "linked.wav");
        assert(collision.ready());
        assert(rasgo::writeWavFile(collision.temporaryPath().string(), samples, 2, 44100, meta));
        assert(!collision.publish() && fs::is_symlink(root / "linked.wav"));
    }
#endif
    std::cout << "PASS: atomic visibility, unchanged PCM bytes, no-replace file/directory/symlink, "
                 "cancel cleanup, 32 rapid +8 concurrent names, validation, JSON escaping, owned-child interruption\n";
}
