#pragma once
#include <exception>
#include <memory>
#include <utility>

namespace rasgo::ui {
// Worker-side boundary, independent of JUCE. One result handle crosses the UI
// queue; exceptions are failures, never a fabricated successful/empty album.
// Callbacks must post bounded notifications; this is not an OOM guarantee.
template <class Result, class Work, class Completed, class Failed>
void runRenderTask(Work&& work, Completed&& completed, Failed&& failed) {
    try {
        auto result = std::make_shared<Result>(std::forward<Work>(work)());
        std::forward<Completed>(completed)(std::move(result));
    } catch (const std::exception& error) {
        std::forward<Failed>(failed)(error.what());
    } catch (...) {
        std::forward<Failed>(failed)(nullptr);
    }
}
} // namespace rasgo::ui
