#include "../io/PublishedIndex.hpp"
#include "../io/Recorder.hpp"
#include <cassert>
#include <array>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <set>
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
using Status = rasgo::IndexAppendStatus;
static std::string bytes(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
static void write(const fs::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary); file << text; file.close(); assert(file.good());
}
static void noPending(const fs::path& root) {
    for (const auto& entry : fs::directory_iterator(root)) {
        const auto name = entry.path().filename().string();
        assert(name.find(".pending-") == std::string::npos && name.find(".lock") == std::string::npos);
    }
}
int main(int argc, char** argv) {
    if (argc > 2) return 2;
    const auto root = argc == 2 ? fs::path(argv[1]) : fs::current_path()
        / ("index-publication-" + rasgo::RecordingPublication::uniqueToken());
    if (fs::exists(root) || !fs::create_directory(root)) return 2;
    const auto normal = root / u8"École — coração.jsonl";
    const std::string old = "{\"legacy\":true}\n";
    const std::string added = u8"{\"título\":\"École — coração\"}";
    write(normal, old);
    fs::permissions(normal, fs::perms::owner_read | fs::perms::owner_write);
    const auto permissions = fs::status(normal).permissions();
    assert(rasgo::appendIndexLine(normal, added) == Status::Complete);
    assert(bytes(normal) == old + added + "\n" && fs::status(normal).permissions() == permissions);
    const auto fresh = root / "fresh.jsonl";
    assert(rasgo::appendIndexLine(fresh, "{}") == Status::Complete && bytes(fresh) == "{}\n");
    const auto empty = root / "empty.jsonl";
    write(empty, "");
    assert(rasgo::appendIndexLine(empty, "{}") == Status::Complete && bytes(empty) == "{}\n");
    const auto saved = bytes(normal);
    for (const std::string line : {"", "plain", "{}\n{}", "{}\r"})
        assert(rasgo::appendIndexLine(normal, line) == Status::InvalidLine && bytes(normal) == saved);
    assert(rasgo::appendIndexLine(normal, std::string("{\0}", 3)) == Status::InvalidLine);
    assert(rasgo::appendIndexLine(normal, "{\"p\":\"" + std::string(rasgo::maxIndexLineBytes, 'x') + "\"}") == Status::InvalidLine);
    const auto partial = root / "partial.jsonl";
    write(partial, old + "{\"unfinished\":");
    const auto partialBefore = bytes(partial);
    assert(rasgo::appendIndexLine(partial, added) == Status::IncompleteIndex && bytes(partial) == partialBefore);
    const auto occupied = root / "occupied.jsonl";
    assert(fs::create_directory(occupied));
    assert(rasgo::appendIndexLine(occupied, added) == Status::InvalidTarget && fs::is_directory(occupied));
    assert(rasgo::appendIndexLine(root / "absent/index.jsonl", added) == Status::InvalidTarget);
    const auto readonly = root / "readonly.jsonl";
    write(readonly, old); fs::permissions(readonly, fs::perms::owner_read);
    assert(rasgo::appendIndexLine(readonly, added) == Status::InvalidTarget && bytes(readonly) == old);
    const auto budget = root / "budget.jsonl";
    { std::ofstream file(budget, std::ios::binary); file.seekp(rasgo::maxIndexBytes); file.put('\n'); file.close(); assert(file.good()); }
    const auto budgetSize = fs::file_size(budget);
    const auto budgetTime = fs::last_write_time(budget);
    assert(rasgo::appendIndexLine(budget, added) == Status::BudgetExceeded);
    assert(fs::file_size(budget) == budgetSize && fs::last_write_time(budget) == budgetTime);
    noPending(root);

    // Large real-sized rows, exact lines rather than a count that could miss
    // merged writes. At most16 rows/1.2 MiB, eight cooperative threads.
    const auto concurrent = root / "concurrent.jsonl";
    write(concurrent, old);
    std::array<bool, 8> passed{};
    std::vector<std::thread> workers;
    std::set<std::string> expected{"{\"legacy\":true}"};
    for (unsigned i = 0; i < passed.size(); ++i) {
        for (unsigned j = 0; j < 2; ++j)
            expected.insert("{\"id\":\"" + std::to_string(i) + "-" + std::to_string(j)
                + "\",\"p\":\"" + std::string(70 * 1024, 'a' + i) + "\"}");
        workers.emplace_back([&, i] {
            passed[i] = true;
            for (unsigned j = 0; j < 2; ++j) {
                const auto line = "{\"id\":\"" + std::to_string(i) + "-" + std::to_string(j)
                    + "\",\"p\":\"" + std::string(70 * 1024, 'a' + i) + "\"}";
                if (rasgo::appendIndexLine(concurrent, line) != Status::Complete) passed[i] = false;
            }
        });
    }
    for (auto& worker : workers) worker.join();
    for (const auto ok : passed) assert(ok);
    std::ifstream lines(concurrent);
    std::string line;
    while (std::getline(lines, line)) assert(expected.erase(line) == 1);
    assert(lines.eof() && expected.empty()); lines.close();
    noPending(root);

    // Actual Recorder integration: a published WAV must not hide index failure.
    const auto recordingFolder = root / "recordings";
    fs::create_directory(recordingFolder);
    fs::create_directory(recordingFolder / "rasgo_index.jsonl");
    rasgo::Recorder recorder(recordingFolder.string());
    rasgo::WavMetadata meta; meta.date = "2026-10-11"; meta.comment = "{\"seed\":2026}";
    const std::vector<float> samples(256, .125f);
    const auto reference = root / "reference.wav";
    assert(rasgo::writeWavFile(reference.string(), samples, 2, 48000, meta));
    std::string diagnostic;
    try { recorder.record("index_refused", samples, 2, 48000, meta); }
    catch (const std::runtime_error& error) { diagnostic = error.what(); }
    assert(diagnostic.find("Recording index publication failed") != std::string::npos);
    unsigned wavs = 0;
    for (const auto& entry : fs::directory_iterator(recordingFolder)) {
        if (entry.path().extension() != ".wav") continue;
        ++wavs;
        assert(diagnostic.find(entry.path().string()) != std::string::npos);
        assert(bytes(entry.path()) == bytes(reference));
    }
    assert(wavs == 1 && fs::is_directory(recordingFolder / "rasgo_index.jsonl"));
    noPending(recordingFolder);
    const auto plainFolder = root / "plain";
    rasgo::Recorder plain(plainFolder.string());
    meta.comment = u8"Acento \"coração\"\nsegunda linha";
    assert(!plain.record("plain", samples, 2, 48000, meta).empty());
    const auto plainIndex = bytes(plainFolder / "rasgo_index.jsonl");
    assert(plainIndex.find("\\\"coração\\\"\\u000a") != std::string::npos && plainIndex.back() == '\n');

#ifdef __unix__
    fs::create_symlink(fs::absolute(normal), root / "linked.jsonl");
    assert(rasgo::appendIndexLine(root / "linked.jsonl", added) == Status::InvalidTarget);
    assert(fs::is_symlink(root / "linked.jsonl") && bytes(normal) == saved);
    fs::create_hard_link(normal, root / "alias.jsonl");
    assert(rasgo::appendIndexLine(normal, added) == Status::InvalidTarget && bytes(normal) == saved);
    // Each process has its own lease instance, not a process-local mutex.
    const auto processes = root / "processes.jsonl";
    write(processes, old);
    std::array<pid_t, 4> children{};
    for (unsigned i = 0; i < children.size(); ++i) {
        children[i] = ::fork(); assert(children[i] >= 0);
        if (children[i] == 0) {
            const auto row = "{\"child\":" + std::to_string(i) + "}";
            ::_exit(rasgo::appendIndexLine(processes, row) == Status::Complete ? 0 : 2);
        }
    }
    for (const auto child : children) {
        int status = 0;
        assert(::waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }
    std::set<std::string> processRows{"{\"legacy\":true}"};
    for (unsigned i = 0; i < children.size(); ++i) processRows.insert("{\"child\":" + std::to_string(i) + "}");
    { std::ifstream file(processes); while (std::getline(file, line)) assert(processRows.erase(line) == 1); }
    assert(processRows.empty());

    rlimit previous{}; assert(::getrlimit(RLIMIT_FSIZE, &previous) == 0);
    auto limited = previous; limited.rlim_cur = 16;
    const auto handler = std::signal(SIGXFSZ, SIG_IGN);
    assert(::setrlimit(RLIMIT_FSIZE, &limited) == 0);
    const auto failed = rasgo::appendIndexLine(processes, added);
    assert(::setrlimit(RLIMIT_FSIZE, &previous) == 0); std::signal(SIGXFSZ, handler);
    assert(failed == Status::WriteFailed);
    { std::ifstream file(processes); unsigned count = 0; while (std::getline(file, line)) ++count; assert(count == 5); }
    noPending(root);

    // Abrupt owned child leaves the previous final index AND private staging.
    const auto interrupted = root / "interrupted.jsonl";
    write(interrupted, old);
    const auto child = ::fork(); assert(child >= 0);
    if (child == 0) {
        rasgo::index_detail::Lease lease(interrupted);
        if (lease.status() != Status::Complete) ::_exit(2);
        rasgo::RecordingPublication pending(interrupted);
        if (!pending.ready()) ::_exit(2);
        write(pending.temporaryPath(), old + added + "\n");
        ::_exit(0);
    }
    int status = 0;
    assert(::waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    assert(bytes(interrupted) == old);
    const auto start = std::chrono::steady_clock::now();
    assert(rasgo::appendIndexLine(interrupted, added) == Status::Busy);
    assert(std::chrono::steady_clock::now() - start < std::chrono::seconds(5));
    assert(bytes(interrupted) == old && fs::is_directory(root / ".interrupted.jsonl.lock"));
    unsigned retained = 0;
    for (const auto& entry : fs::directory_iterator(root)) {
        if (entry.path().filename().string().find(".interrupted.jsonl.pending-") == 0) {
            ++retained; assert(bytes(entry.path() / "payload.wav") == old + added + "\n");
        }
    }
    assert(retained == 1);
#endif
    std::cout << "PASS index publication: exact prefix/UTF-8/permissions, bounded snapshots, refused partial/links, "
                 "8 threads +4 processes, write failure, preserved WAV with explicit error, retained crash evidence\n";
}
