#pragma once
#include <algorithm>
namespace rasgo::ui {
struct ViewportLayout {
    int width, height;
    bool horizontal, vertical;
    static constexpr int minimumWidth = 1100, minimumHeight = 760;
    static ViewportLayout fit(int width, int height, int scrollbar,
                              int requiredHeight = minimumHeight) noexcept {
        width = std::max(0, width); height = std::max(0, height);
        scrollbar = std::max(0, scrollbar);
        requiredHeight = std::max(minimumHeight, requiredHeight);
        bool horizontal = false, vertical = false;
        // Monotone fixed point: either scrollbar can require the other.
        for (int pass = 0; pass < 3; ++pass) {
            horizontal = horizontal || width - (vertical ? scrollbar : 0) < minimumWidth;
            vertical = vertical || height - (horizontal ? scrollbar : 0) < requiredHeight;
        }
        return {std::max(minimumWidth, width - (vertical ? scrollbar : 0)),
                std::max(requiredHeight, height - (horizontal ? scrollbar : 0)),
                horizontal, vertical};
    }
    template <class MinimumHeight>
    static ViewportLayout fitContent(int width, int height, int scrollbar,
                                     MinimumHeight minimum) {
        auto result = fit(width, height, scrollbar);
        // Width-dependent rows may wrap after the vertical bar appears.
        for (int pass = 0; pass < 3; ++pass)
            result = fit(width, height, scrollbar, minimum(result.width));
        return result;
    }
};
} // namespace rasgo::ui
