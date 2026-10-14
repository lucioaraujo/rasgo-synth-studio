#include "../io/PublishedText.hpp"
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
#include <sys/wait.h>
#include <unistd.h>
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
static void noPending(const fs::path& root) {
    for (const auto& entry : fs::directory_iterator(root))
        assert(entry.path().filename().string().find(".pending-") == std::string::npos);
}
int main(int argc, char** argv) {
    if (argc > 2) return 2;
    const fs::path root = argc == 2 ? fs::path(argv[1]) : fs::current_path()
        / ("text-publication-" + rasgo::RecordingPublication::uniqueToken());
    if (fs::exists(root) || !fs::create_directory(root)) return 2;
    const std::string text = u8"{\"título\":\"École — coração\"}\n";
    assert(rasgo::publishTextNoReplace(root / "manifest.json", text));
    assert(bytes(root / "manifest.json") == text);
    assert(!rasgo::publishTextNoReplace(root / "manifest.json", "replacement"));
    assert(bytes(root / "manifest.json") == text);
    assert(rasgo::publishTextNoReplace(root / "empty.txt", ""));
    assert(fs::file_size(root / "empty.txt") == 0);
    fs::create_directory(root / "occupied.json");
    assert(!rasgo::publishTextNoReplace(root / "occupied.json", text));
    assert(fs::is_directory(root / "occupied.json"));
    assert(!rasgo::publishTextNoReplace(root / "missing-parent/manifest.json", text));
    assert(!fs::exists(root / "missing-parent"));
    const std::string oversized(4 * 1024 * 1024 + 1, 'x');
    assert(!rasgo::publishTextNoReplace(root / "oversized.json", oversized));
    assert(!fs::exists(root / "oversized.json"));
    // Fixed-size cohort: exactly one complete publication wins, no replacement.
    std::array<bool, 8> winners{};
    std::vector<std::thread> workers;
    for (std::size_t index = 0; index < winners.size(); ++index)
        workers.emplace_back([&, index] {
            winners[index] = rasgo::publishTextNoReplace(root / "race.json", std::to_string(index));
        });
    for (auto& worker : workers) worker.join();
    unsigned count = 0;
    for (std::size_t index = 0; index < winners.size(); ++index)
        if (winners[index]) { ++count; assert(bytes(root / "race.json") == std::to_string(index)); }
    assert(count == 1);
    noPending(root);
#ifdef __unix__
    fs::create_symlink(fs::absolute(root / "manifest.json"), root / "linked.json");
    assert(!rasgo::publishTextNoReplace(root / "linked.json", "replacement"));
    assert(fs::is_symlink(root / "linked.json") && bytes(root / "manifest.json") == text);
    rlimit previous{};
    assert(::getrlimit(RLIMIT_FSIZE, &previous) == 0);
    auto limited = previous;
    limited.rlim_cur = 16;
    const auto handler = std::signal(SIGXFSZ, SIG_IGN);
    assert(::setrlimit(RLIMIT_FSIZE, &limited) == 0);
    const bool success = rasgo::publishTextNoReplace(root / "failed.json", std::string(256, 'x'));
    assert(::setrlimit(RLIMIT_FSIZE, &previous) == 0);
    std::signal(SIGXFSZ, handler);
    assert(!success && !fs::exists(root / "failed.json"));
    assert(bytes(root / "manifest.json") == text);
    noPending(root);
    // Abrupt loss retains owned staging as evidence, never an incomplete final.
    const auto child = ::fork();
    assert(child >= 0);
    if (child == 0) {
        rasgo::RecordingPublication pending(root / "interrupted.json");
        if (!pending.ready()) ::_exit(2);
        std::ofstream file(pending.temporaryPath(), std::ios::binary);
        file << text;
        file.close();
        ::_exit(file.good() ? 0 : 2);
    }
    int status = 0;
    assert(::waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(!fs::exists(root / "interrupted.json"));
    unsigned retained = 0;
    for (const auto& entry : fs::directory_iterator(root)) {
        if (entry.path().filename().string().find(".interrupted.json.pending-") == 0) {
            ++retained;
            assert(bytes(entry.path() / "payload.wav") == text);
        }
    }
    assert(retained == 1);
#endif
    std::cout << "PASS text publication: UTF-8 bytes, no-replace, bounded size, single concurrent winner, "
                 "failed-write cleanup and owned-child interruption\n";
}
