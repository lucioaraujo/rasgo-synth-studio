#pragma once
#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

namespace rasgo {

// Offline recorder only, never the audio callback. A completed file is made
// visible with a no-replace hard link on the same filesystem. No fallback to
// overwrite/copy: a filesystem without hard links fails safely. This guarantees
// visibility, not power-loss durability (no fsync) or a multi-file transaction.
class RecordingPublication {
public:
    explicit RecordingPublication(std::filesystem::path target) : target_(std::move(target)) {
        for (unsigned attempt = 0; attempt < 64; ++attempt) {
            staging_ = target_.parent_path() / std::filesystem::u8path("." + target_.filename().u8string()
                + ".pending-" + uniqueToken());
            std::error_code error;
            if (std::filesystem::create_directory(staging_, error)) {
                owned_ = true;
                payload_ = staging_ / "payload.wav";
                return;
            }
            if (error) return;
        }
    }
    RecordingPublication(const RecordingPublication&) = delete;
    RecordingPublication& operator=(const RecordingPublication&) = delete;
    ~RecordingPublication() {
        if (!owned_) return;
        // Only remove our exact payload and the now-empty directory. Never
        // sweep old pending files, user recordings or unknown directory entries.
        std::error_code ignored;
        std::filesystem::remove(payload_, ignored);
        std::filesystem::remove(staging_, ignored);
    }
    bool ready() const noexcept { return owned_; }
    const std::filesystem::path& temporaryPath() const noexcept { return payload_; }
    bool publish() {
        if (!owned_) return false;
        std::error_code error;
        std::filesystem::create_hard_link(payload_, target_, error);
        return !error; // existing files, directories and symlinks are not replaced
    }
    static std::string uniqueToken() {
        static std::atomic<unsigned long long> sequence {0};
        return std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
            + "_" + std::to_string(sequence.fetch_add(1, std::memory_order_relaxed));
    }
private:
    std::filesystem::path target_, staging_, payload_;
    bool owned_ = false;
};
} // namespace rasgo
