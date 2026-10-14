#include "../src/RenderTask.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
int main() {
    using rasgo::ui::runRenderTask;
    int completed = 0, failed = 0;
    std::shared_ptr<std::vector<int>> retained;
    std::string message;
    auto success = [&](std::shared_ptr<std::vector<int>> result) {
        ++completed; retained = std::move(result);
    };
    auto failure = [&](const char* detail) {
        ++failed; message = detail ? detail : "unknown";
    };
    runRenderTask<std::vector<int>>([] { return std::vector<int>{3, 5, 8}; }, success, failure);
    if (completed != 1 || failed || !retained || retained->at(2) != 8 || retained.use_count() != 1)
        return EXIT_FAILURE;
    runRenderTask<std::vector<int>>([]() -> std::vector<int> { throw std::runtime_error("render failed"); }, success, failure);
    if (completed != 1 || failed != 1 || message != "render failed") return EXIT_FAILURE;
    runRenderTask<std::vector<int>>([]() -> std::vector<int> { throw 7; }, success, failure);
    if (completed != 1 || failed != 2 || message != "unknown") return EXIT_FAILURE;
    runRenderTask<std::vector<int>>([]() -> std::vector<int> { throw std::bad_alloc(); }, success, failure);
    if (completed != 1 || failed != 3 || message.empty()) return EXIT_FAILURE;
    std::cout << "Render task: shared result, standard/unknown/allocation exceptions: PASS (not GUI or OOM acceptance)\n";
}
