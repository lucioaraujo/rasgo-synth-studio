#pragma once
#include <algorithm>
#include "ActionBarLayout.h"

namespace rasgo::ui {
// Keep the complete console scrollable instead of shrinking controls/fonts.
// These are the same vertical rows consumed in MainComponent::resized().
struct StudioLayoutBudget {
    static constexpr int waveformMinimum = 70;
    static constexpr int bottomMinimum = 142; // 22px heading + 120px visualizer
    static int minimumHeight(int usableWidth, int moduleHeight) noexcept {
        const int frameAndFooter = 36 + 18 + 4;
        const int header = 78 + 2 + 20 + 4;
        const int controls = 112 + 8;
        const int album = 62 + 8;
        const int transport = 72 + 10;
        const int actions = 26 + 6 + (ActionBarLayout::splitRows(usableWidth) ? 32 : 0);
        const int modules = std::clamp(moduleHeight, 56, 180) + 6;
        const int waveform = 18 + 3 + waveformMinimum + 8;
        const int meters = 96 + 8;
        return frameAndFooter + header + controls + album + transport + actions
             + modules + waveform + meters + bottomMinimum;
    }
};
} // namespace rasgo::ui
