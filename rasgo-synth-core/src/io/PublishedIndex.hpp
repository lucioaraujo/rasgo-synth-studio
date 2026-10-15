#pragma once
#include "RecordingPublication.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>
#include <new>
#include <thread>

namespace rasgo {

inline constexpr std::size_t maxIndexLineBytes = 1024 * 1024;
inline constexpr std::uintmax_t maxIndexBytes = 64 * 1024 * 1024;
enum class IndexAppendStatus {
    Complete, InvalidLine, InvalidTarget, Busy, BudgetExceeded, ReadFailed,
    IncompleteIndex, WriteFailed, ChangedTarget, PublishFailed
};
inline const char* indexAppendMessage(IndexAppendStatus status) noexcept {
    switch (status) {
        case IndexAppendStatus::Complete: return "complete";
        case IndexAppendStatus::InvalidLine: return "invalid or oversized single-line object";
        case IndexAppendStatus::InvalidTarget: return "invalid or unavailable index target";
        case IndexAppendStatus::Busy: return "busy or retained index lock; no automatic removal";
        case IndexAppendStatus::BudgetExceeded: return "index exceeds the 64 MiB budget";
        case IndexAppendStatus::ReadFailed: return "previous index could not be read completely";
        case IndexAppendStatus::IncompleteIndex: return "previous index has an incomplete last line";
        case IndexAppendStatus::WriteFailed: return "staged index write/flush/close failed";
        case IndexAppendStatus::ChangedTarget: return "index changed outside the writer contract";
        case IndexAppendStatus::PublishFailed: return "completed index could not be published";
    }
    return "unknown index failure";
}

namespace index_detail {
inline constexpr std::size_t copyBufferBytes = 64 * 1024;
// Cooperative writers, including separate processes, use the same directory.
// Only our own empty lock is removed. A crash retains it: bounded refusal,
// not unsafe lock stealing based on time/PID. Never used in an audio callback.
class Lease {
public:
    explicit Lease(const std::filesystem::path& target)
        : path_(target.parent_path() / std::filesystem::u8path("." + target.filename().u8string() + ".lock")) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        for (;;) {
            std::error_code error;
            if (std::filesystem::create_directory(path_, error)) {
                owned_ = true;
                status_ = IndexAppendStatus::Complete;
                return;
            }
            if (error && error != std::errc::file_exists) {
                status_ = IndexAppendStatus::InvalidTarget;
                return;
            }
            if (std::chrono::steady_clock::now() >= deadline) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    Lease(const Lease&) = delete;
    Lease& operator=(const Lease&) = delete;
    ~Lease() {
        if (!owned_) return;
        std::error_code ignored;
        std::filesystem::remove(path_, ignored); // not recursive; never sweep retained locks
    }
    IndexAppendStatus status() const noexcept { return status_; }
private:
    std::filesystem::path path_;
    bool owned_ = false;
    IndexAppendStatus status_ = IndexAppendStatus::Busy;
};

struct Snapshot {
    bool exists = false;
    std::uintmax_t size = 0;
    std::filesystem::file_time_type modified{};
    std::filesystem::perms permissions = std::filesystem::perms::unknown;
};
inline bool snapshot(const std::filesystem::path& target, Snapshot& value) {
    std::error_code error;
    const auto status = std::filesystem::symlink_status(target, error);
    if (error && error != std::errc::no_such_file_or_directory) return false;
    if (!std::filesystem::exists(status)) return true;
    if (!std::filesystem::is_regular_file(status)) return false; // no symlink/directory replacement
    value.exists = true;
    value.permissions = status.permissions();
    constexpr auto writable = std::filesystem::perms::owner_write
        | std::filesystem::perms::group_write | std::filesystem::perms::others_write;
    if (value.permissions != std::filesystem::perms::unknown
        && (value.permissions & writable) == std::filesystem::perms::none) return false;
    if (std::filesystem::hard_link_count(target, error) != 1 || error) return false;
    value.size = std::filesystem::file_size(target, error);
    if (error) return false;
    value.modified = std::filesystem::last_write_time(target, error);
    return !error;
}
inline bool same(const Snapshot& a, const Snapshot& b) noexcept {
    return a.exists == b.exists && a.size == b.size && a.modified == b.modified
        && a.permissions == b.permissions;
}
} // namespace index_detail

// Logically append one line, physically publish a completed same-filesystem
// snapshot. Existing bytes are copied verbatim with a 64 KiB buffer, never
// truncated in place or rolled back. Only cooperative writers are serialized;
// legacy/uncooperative/hostile writers are NOT covered by this contract.
// Final-newline check is not a JSON parser. No fsync, multi-file transaction,
// ACL/xattr cloning or automatic crash recovery is promised.
inline IndexAppendStatus appendIndexLine(const std::filesystem::path& target,
                                         const std::string& line) {
    if (line.empty() || line.size() > maxIndexLineBytes || line.front() != '{' || line.back() != '}'
        || line.find_first_of("\r\n") != std::string::npos || line.find('\0') != std::string::npos)
        return IndexAppendStatus::InvalidLine;
    index_detail::Lease lease(target);
    if (lease.status() != IndexAppendStatus::Complete) return lease.status();
    index_detail::Snapshot before;
    if (!index_detail::snapshot(target, before)) return IndexAppendStatus::InvalidTarget;
    if (before.size > maxIndexBytes - line.size() - 1) return IndexAppendStatus::BudgetExceeded;

    RecordingPublication publication(target);
    if (!publication.ready()) return IndexAppendStatus::WriteFailed;
    std::ofstream output(publication.temporaryPath(), std::ios::binary | std::ios::trunc);
    if (!output.is_open()) return IndexAppendStatus::WriteFailed;
    if (before.exists) {
        std::ifstream previous(target, std::ios::binary);
        if (!previous.is_open()) return IndexAppendStatus::ReadFailed;
        // Offline file publication, never the audio callback. This bounded
        // workspace must not extend the already-large renderer stack: the
        // Intel Mac worker hit ___chkstk_darwin here before any index write.
        std::unique_ptr<char[]> buffer(new (std::nothrow) char[index_detail::copyBufferBytes]);
        if (!buffer) return IndexAppendStatus::WriteFailed;
        auto remaining = before.size;
        char last = '\n';
        while (remaining) {
            const auto count = static_cast<std::streamsize>(std::min<std::uintmax_t>(remaining, index_detail::copyBufferBytes));
            previous.read(buffer.get(), count);
            if (previous.gcount() != count || previous.bad()) return IndexAppendStatus::ReadFailed;
            output.write(buffer.get(), count);
            if (!output.good()) return IndexAppendStatus::WriteFailed;
            last = buffer[static_cast<std::size_t>(count) - 1];
            remaining -= static_cast<std::uintmax_t>(count);
        }
        previous.close();
        if (previous.fail()) return IndexAppendStatus::ReadFailed;
        if (last != '\n') return IndexAppendStatus::IncompleteIndex;
    }
    output.write(line.data(), static_cast<std::streamsize>(line.size()));
    output.put('\n');
    output.flush();
    const bool written = output.good();
    output.close();
    if (!written || output.fail()) return IndexAppendStatus::WriteFailed;
    std::error_code error;
    if (before.exists && before.permissions != std::filesystem::perms::unknown) {
        std::filesystem::permissions(publication.temporaryPath(), before.permissions, error);
        if (error) return IndexAppendStatus::WriteFailed;
    }
    index_detail::Snapshot after;
    if (!index_detail::snapshot(target, after) || !index_detail::same(before, after))
        return IndexAppendStatus::ChangedTarget;
    if (!before.exists) return publication.publish() ? IndexAppendStatus::Complete : IndexAppendStatus::PublishFailed;
    std::filesystem::rename(publication.temporaryPath(), target, error);
    return error ? IndexAppendStatus::PublishFailed : IndexAppendStatus::Complete;
}
} // namespace rasgo
