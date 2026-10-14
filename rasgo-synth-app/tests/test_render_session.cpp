#include "../src/RenderSession.h"
#include <cstring>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
int main() {
    using Session = rasgo::ui::RenderSession<std::vector<int>, int>;
    Session success;
    success.run([&] { for (int i = 0; i < 100000; ++i) success.publish(i); return std::vector<int>{1, 2}; });
    if (!success.done() || !success.result() || success.result()->at(1) != 2
        || success.coalesced() != 99999 || success.takeProgress() != 99999 || success.takeProgress()) return 1;
    Session cancelled;
    cancelled.requestCancel();
    bool called = false;
    cancelled.run([&] { called = true; return std::vector<int>{}; });
    if (called || !cancelled.done() || !cancelled.cancelled() || cancelled.result()) return 2;
    Session during;
    during.run([&]() -> std::vector<int> { during.requestCancel(); during.checkpoint(); return {3}; });
    if (!during.cancelled() || during.result()) return 3;
    Session failure;
    failure.run([]() -> std::vector<int> { throw std::runtime_error("failed"); });
    if (std::strcmp(failure.error(), "failed") || failure.result()) return 4;
    Session unknown;
    unknown.run([]() -> std::vector<int> { throw 7; });
    if (!unknown.done() || !*unknown.error()) return 5;
    Session launch;
    launch.launchFailed("thread creation failed");
    if (!launch.done() || launch.result() || !*launch.error()) return 6;
    // Recovery folder may be long/UTF-8. This must survive worker -> UI -> log.
    const auto recovery = std::string("Album manifest publication failed; completed tracks preserved in: /")
        + std::string(4096, 'x') + u8"/École — coração";
    Session longFailure;
    longFailure.run([&]() -> std::vector<int> { throw std::runtime_error(recovery); });
    if (longFailure.result() || std::strcmp(longFailure.error(), recovery.c_str())) {
        std::cerr << "FAIL: render failure truncated recoverable path\n";
        return 9;
    }
    const auto oversized = std::string(8178, 'x') + u8"é" + std::string(2048, 'x');
    Session boundedFailure;
    boundedFailure.launchFailed(oversized.c_str());
    const auto expected = std::string(8178, 'x') + " [truncated]";
    if (std::strcmp(boundedFailure.error(), expected.c_str())) return 10;
    Session allocationFailure;
    allocationFailure.run([]() -> std::vector<int> { throw std::bad_alloc(); });
    if (allocationFailure.result() || !*allocationFailure.error()) return 11;
    for (int round = 0; round < 200; ++round) {
        Session concurrent;
        std::thread worker([&] {
            concurrent.run([&] { for (int i = 0; i < 1000; ++i) concurrent.publish(i); return std::vector<int>{round}; });
        });
        int latest = -1;
        bool monotone = true;
        while (!concurrent.done()) if (auto progress = concurrent.takeProgress()) {
            if (*progress < latest) monotone = false;
            latest = *progress;
        }
        worker.join();
        if (!monotone) return 7;
        if (concurrent.result()->at(0) != round) return 8;
    }
    std::cout << "Render session: one-slot progress, cancellation, failures, long recovery path, "
                 "bounded UTF-8 truncation, bad_alloc, launch failure, 200 concurrent runs: PASS\n";
}
