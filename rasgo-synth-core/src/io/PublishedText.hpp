#pragma once
#include "RecordingPublication.hpp"
#include <fstream>
#include <string>

namespace rasgo {

// Offline, single-file visibility only: no overwrite, no fsync or multi-file
// transaction. Reuses the recorder's owned staging/no-replace publication.
// Suitable for new manifests/sidecars, NOT append indexes or mutable markets.
inline bool publishTextNoReplace(const std::filesystem::path& target,
                                  const std::string& text) {
    constexpr std::size_t maxBytes = 4 * 1024 * 1024;
    if (text.size() > maxBytes) return false;
    RecordingPublication publication(target);
    if (!publication.ready()) return false;
    std::ofstream output(publication.temporaryPath(), std::ios::binary | std::ios::trunc);
    if (!output.is_open()) return false;
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
    output.flush();
    const bool written = output.good();
    output.close();
    return written && output.good() && publication.publish();
}
} // namespace rasgo
