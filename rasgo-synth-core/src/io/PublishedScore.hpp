#pragma once
#include "PublishedText.hpp"
#include <stdexcept>

namespace rasgo {

// Offline companion of an already-published recording. Publication failure is
// not a successful complete render: leave the WAV/index untouched and report
// their location. No rollback, mutable index handling or fsync is implied.
inline void publishRecordingScore(const std::filesystem::path& recording,
                                  const std::string& text) {
    std::error_code error;
    const auto status = std::filesystem::symlink_status(recording, error);
    if (error || !std::filesystem::is_regular_file(status) || recording.extension() != ".wav")
        throw std::invalid_argument("Score requires an already-published regular WAV");
    auto target = recording;
    target.replace_extension(".txt");
    if (!publishTextNoReplace(target, text))
        throw std::runtime_error("Score publication failed; completed WAV preserved at: "
            + recording.string() + "; recovery folder: " + recording.parent_path().string());
}
} // namespace rasgo
