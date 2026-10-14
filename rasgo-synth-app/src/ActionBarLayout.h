#pragma once

namespace rasgo::ui {
// Existing waveform actions need726 px; the new render settings need408 px.
// Wrap settings onto their own row instead of overlapping at the viewport minimum.
struct ActionBarLayout {
    static constexpr int waveformWidth = 726;
    static constexpr int settingsWidth = 408;
    static constexpr bool splitRows(int availableWidth) noexcept {
        return availableWidth < waveformWidth + settingsWidth;
    }
};
} // namespace rasgo::ui
