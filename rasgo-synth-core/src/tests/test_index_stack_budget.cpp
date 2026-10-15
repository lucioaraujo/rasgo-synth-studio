#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../io/PublishedIndex.hpp"
#include <cassert>
#include <iostream>
#include <iterator>
#include <string>
#if defined(__unix__) || defined(__APPLE__)
#include <pthread.h>
#endif

namespace fs = std::filesystem;

#if defined(__unix__) || defined(__APPLE__)
struct AppendWork {
    fs::path target;
    std::string line;
    rasgo::IndexAppendStatus status = rasgo::IndexAppendStatus::WriteFailed;
};
static void* appendOnSmallStack(void* raw) noexcept {
    auto& work = *static_cast<AppendWork*>(raw);
    try { work.status = rasgo::appendIndexLine(work.target, work.line); }
    catch (...) { work.status = rasgo::IndexAppendStatus::WriteFailed; }
    return nullptr;
}
#endif

int main() {
#if defined(__unix__) || defined(__APPLE__)
    const auto root = fs::current_path() / ("index-small-stack-" + rasgo::RecordingPublication::uniqueToken());
    assert(!fs::exists(root) && fs::create_directory(root));
    const std::string old = "{\"old\":\"" + std::string(70 * 1024, 'a') + "\"}\n";
    const std::string next = "{\"small_stack\":true}";
    pthread_attr_t attributes;
    assert(pthread_attr_init(&attributes) == 0);
    // Much smaller than the usual 512 KiB Mac worker. The old 64 KiB local
    // array alone exhausted this stack; publication now owns a heap buffer.
    assert(pthread_attr_setstacksize(&attributes, 64 * 1024) == 0);
    for (const bool existing : {false, true}) {
        AppendWork work{root / (existing ? "existing.jsonl" : "fresh.jsonl"), next};
        if (existing) {
            std::ofstream previous(work.target, std::ios::binary);
            previous << old;
            previous.close();
            assert(previous.good());
        }
        pthread_t thread;
        assert(pthread_create(&thread, &attributes, appendOnSmallStack, &work) == 0);
        assert(pthread_join(thread, nullptr) == 0);
        assert(work.status == rasgo::IndexAppendStatus::Complete);
        std::ifstream input(work.target, std::ios::binary);
        const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        assert(text == (existing ? old : "") + next + "\n");
    }
    assert(pthread_attr_destroy(&attributes) == 0);
    for (const auto& entry : fs::directory_iterator(root))
        assert(entry.path().extension() == ".jsonl");
    std::cout << "PASS fresh/existing index on 64 KiB worker stack, exact prefix and no pending files\n";
#else
    std::cout << "SKIP POSIX stack-size fixture; native Unicode recorder test covers Windows publication\n";
#endif
}
