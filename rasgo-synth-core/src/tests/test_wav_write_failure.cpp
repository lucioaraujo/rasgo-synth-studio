#include "../io/WavWriter.hpp"
#include "../io/Recorder.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#ifdef __unix__
#include <csignal>
#include <sys/resource.h>
#endif
int main(int argc, char** argv) {
    if (argc > 2) return 2;
    const auto root = argc == 2 ? std::filesystem::path(argv[1])
        : std::filesystem::current_path() / ("wav-write-validation-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    if (std::filesystem::exists(root) || !std::filesystem::create_directory(root)) return 2;
    const std::vector<float> samples(256, .125f);
    rasgo::WavMetadata meta;
    meta.title = "Local storage failure test";
    if (!rasgo::writeWavFile((root / "valid.wav").string(), samples, 2, 48000, meta)) return 1;
    if (rasgo::writeWavFile(root.string(), samples, 2, 48000, meta)) return 1;
#ifdef __unix__
    // Fail this process's file writes after256 bytes. This is not a full disk,
    // device write, permission change or change to another process's limits.
    rlimit previous{};
    if (::getrlimit(RLIMIT_FSIZE, &previous) != 0) return 2;
    auto limited = previous; limited.rlim_cur = 256;
    const auto handler = std::signal(SIGXFSZ, SIG_IGN);
    if (::setrlimit(RLIMIT_FSIZE, &limited) != 0) return 2;
    const bool claimedSuccess = rasgo::writeWavFile((root / "limited.wav").string(), samples, 2, 48000, meta);
    rasgo::Recorder recorder((root / "recorder-limited").string());
    const auto recordedPath = recorder.record("bounded", samples, 2, 48000, meta);
    const bool restored = ::setrlimit(RLIMIT_FSIZE, &previous) == 0;
    std::signal(SIGXFSZ, handler);
    if (!restored) return 2;
    if (claimedSuccess) {
        std::cerr << "FAIL: writer claimed success after a bounded storage failure\n";
        return 1;
    }
    if (!recordedPath.empty() || std::filesystem::exists(root / "recorder-limited/rasgo_index.jsonl")) return 1;
    // The recorder must not expose a partial final WAV or leak its owned
    // staging file when ordinary write failure returns through RAII cleanup.
    if (!std::filesystem::is_empty(root / "recorder-limited")) return 1;
#endif
    std::cout << "WAV writer: valid file, invalid output, bounded storage failure: PASS\n";
}
