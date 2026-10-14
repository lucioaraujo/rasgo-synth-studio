#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <exception>
#include <mutex>
#include <optional>
#include <utility>

namespace rasgo::ui {
struct RenderCancelled final : std::exception {
    const char* what() const noexcept override { return "render cancelled"; }
};

// Worker/message-thread handoff, never used by the audio callback. One latest
// progress slot and one terminal result: a slow UI cannot grow a callback queue.
template <class Result, class Progress>
class RenderSession {
public:
    void requestCancel() noexcept { cancel_.store(true); }
    void checkpoint() const { if (cancel_.load()) throw RenderCancelled{}; }
    void publish(Progress progress) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (progress_) ++coalesced_;
        progress_ = std::move(progress);
    }
    std::optional<Progress> takeProgress() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto latest = std::move(progress_);
        progress_.reset();
        return latest;
    }
    std::uint64_t coalesced() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return coalesced_;
    }
    template <class Work>
    void run(Work&& work) noexcept {
        try {
            checkpoint();
            result_.emplace(std::forward<Work>(work)());
        } catch (const RenderCancelled&) {
            cancelled_ = true;
        } catch (const std::exception& error) {
            copyError(error.what());
        } catch (...) {
            copyError("unknown render failure");
        }
        done_.store(true, std::memory_order_release);
    }
    void launchFailed(const char* message) noexcept {
        copyError(message);
        done_.store(true, std::memory_order_release);
    }
    bool done() const noexcept { return done_.load(std::memory_order_acquire); }
    // Access terminal fields only after done() and after joining the worker.
    bool cancelled() const noexcept { return cancelled_; }
    const char* error() const noexcept { return error_.data(); }
    std::optional<Result>& result() noexcept { return result_; }
private:
    void copyError(const char* message) noexcept {
        if (!message) message = "unknown render failure";
        std::size_t index = 0;
        for (; index + 1 < error_.size() && message[index]; ++index) error_[index] = message[index];
        if (message[index]) {
            // Still bounded/no allocation on failure (including bad_alloc).
            // Leave an explicit marker and never split a UTF-8 scalar when
            // the incoming diagnostic is valid UTF-8.
            constexpr char marker[] = " [truncated]";
            index = error_.size() - sizeof(marker);
            while (index && (static_cast<unsigned char>(message[index]) & 0xc0) == 0x80) --index;
            for (std::size_t suffix = 0; suffix + 1 < sizeof(marker); ++suffix)
                error_[index++] = marker[suffix];
        }
        error_[index] = '\0';
    }
    std::atomic<bool> cancel_ {false}, done_ {false};
    mutable std::mutex mutex_;
    std::optional<Progress> progress_;
    std::uint64_t coalesced_ = 0;
    std::optional<Result> result_;
    // Covers a Linux-sized recovery path plus context without growing a queue
    // or allocating while handling an exception. Not an unlimited error log.
    std::array<char, 8192> error_ {};
    bool cancelled_ = false;
};
} // namespace rasgo::ui
