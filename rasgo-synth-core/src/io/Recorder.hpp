#pragma once
#include <string>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <mutex>
#include "WavWriter.hpp"
#include "RecordingPublication.hpp"
#include "PublishedIndex.hpp"
#include <stdexcept>

namespace rasgo {

// Wraps WavWriter with the "the synth records its own experiments into a
// directory" requirement: given an output directory and a short base name,
// it creates the directory if needed, builds a timestamped filename so
// nothing gets overwritten between runs, and writes the WAV with metadata.
class Recorder {
public:
    explicit Recorder(std::string outputDirectory) : outputDirectory_(std::move(outputDirectory)) {
        ensureDirectoryExists(outputDirectory_);
    }

    // Returns the full path written, or empty if WAV publication fails.
    // If the completed WAV cannot be indexed, throws with its recovery path;
    // never delete a published WAV or silently announce a complete recording.
    std::string record(const std::string &baseName,
                        const std::vector<float> &interleavedSamples,
                        int numChannels,
                        int sampleRate,
                        WavMetadata meta) {
        // Reject path components rather than permitting a base name to escape
        // the caller's output directory. Existing callers use ASCII identifiers.
        if (numChannels <= 0 || numChannels > 21845 || sampleRate <= 0
            || sampleRate > 2147483647 / (numChannels * 3)
            || interleavedSamples.empty()
            || interleavedSamples.size() % static_cast<std::size_t>(numChannels) != 0
            || baseName.empty() || baseName.size() > 96
            || baseName.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")
                != std::string::npos) return {};
        std::string timestamp = currentTimestamp();
        if (meta.date.empty()) meta.date = currentDate();

        std::string path = outputDirectory_ + "/" + baseName + "_" + timestamp
            + "_" + RecordingPublication::uniqueToken() + ".wav";
        RecordingPublication publication(std::filesystem::u8path(path));
        if (!publication.ready()) return {};
        bool ok = writeWavFile(publication.temporaryPath().u8string(),
            interleavedSamples, numChannels, sampleRate, meta) && publication.publish();
        if (ok) appendIndexEntry(path, meta);
        return ok ? path : std::string();
    }

private:
    // Every recording's seed/config already lives embedded in its own WAV
    // (meta.comment, a JSON object -- see PieceRenderer.hpp), so a session
    // can always recover "what made this file" from the file itself. What
    // was missing was a way to answer "what have I made so far" without
    // opening each WAV's binary metadata one at a time -- an append-only
    // index, one JSON line per recording, accumulating across every
    // session that has ever written into this output directory (not reset
    // per run, unlike the app's own session log). Reuses meta.comment
    // rather than re-deriving the same fields a second time -- this stays
    // correct automatically if the caller's JSON grows new fields later.
    void appendIndexEntry(const std::string &path, const WavMetadata &meta) {
        if (meta.comment.size() > maxIndexLineBytes || meta.date.size() > maxIndexLineBytes)
            indexFailure(path, IndexAppendStatus::InvalidLine);
        std::string line = meta.comment;
        if (!line.empty() && line.front() == '{') {
            line.insert(1, "\"path\":\"" + jsonEscape(path) + "\",\"date\":\"" + jsonEscape(meta.date) + "\",");
        } else {
            // Empty/plain-text comments must still yield an identifiable JSON
            // row. Existing object metadata retains its exact previous bytes.
            line = "{\"path\":\"" + jsonEscape(path) + "\",\"date\":\"" + jsonEscape(meta.date)
                + "\",\"comment\":\"" + jsonEscape(meta.comment) + "\"}";
        }
        const auto status = appendIndexLine(std::filesystem::u8path(outputDirectory_) / "rasgo_index.jsonl", line);
        if (status != IndexAppendStatus::Complete) indexFailure(path, status);
    }

    [[noreturn]] void indexFailure(const std::string& path, IndexAppendStatus status) const {
        throw std::runtime_error(std::string("Recording index publication failed (") + indexAppendMessage(status)
            + "); completed WAV preserved at: " + path + "; recovery folder: " + outputDirectory_);
    }

    static void ensureDirectoryExists(const std::string &dir) {
        std::error_code ignored;
        std::filesystem::create_directories(std::filesystem::u8path(dir), ignored);
    }

    static std::string jsonEscape(const std::string& value) {
        std::string escaped;
        for (const unsigned char c : value) {
            if (c == '"' || c == '\\') { escaped += '\\'; escaped += static_cast<char>(c); }
            else if (c < 32) {
                const char* hex = "0123456789abcdef";
                escaped += "\\u00"; escaped += hex[c >> 4]; escaped += hex[c & 15];
            } else escaped += static_cast<char>(c);
        }
        return escaped;
    }

    static std::string currentTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::ostringstream os;
        const auto calendar = localCalendar(tt);
        os << std::put_time(&calendar, "%Y%m%d_%H%M%S");
        return os.str();
    }

    static std::string currentDate() {
        auto now = std::chrono::system_clock::now();
        std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::ostringstream os;
        const auto calendar = localCalendar(tt);
        os << std::put_time(&calendar, "%Y-%m-%d");
        return os.str();
    }

    static std::tm localCalendar(std::time_t time) {
        std::tm calendar{};
#if defined(_WIN32)
        ::localtime_s(&calendar, &time);
        return calendar;
#elif defined(__unix__) || defined(__APPLE__)
        ::localtime_r(&time, &calendar);
        return calendar;
#else
        // Serializes recorder callers on platforms without the reentrant API.
        static std::mutex calendarMutex;
        const std::lock_guard<std::mutex> lock(calendarMutex);
        const auto* value = std::localtime(&time);
        return value ? *value : std::tm{};
#endif
    }

    std::string outputDirectory_;
};

} // namespace rasgo
