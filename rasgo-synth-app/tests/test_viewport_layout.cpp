#include "../src/ViewportLayout.h"
#include <iostream>
int main() {
    using rasgo::ui::ViewportLayout;
    for (int width : {900, 1100, 1117, 1118, 1366, 1920})
        for (int height : {600, 736, 750, 760, 778, 1006}) {
            const auto fit = ViewportLayout::fit(width, height, 18);
            const int availableWidth = width - (fit.vertical ? 18 : 0);
            const int availableHeight = height - (fit.horizontal ? 18 : 0);
            if (fit.horizontal != (fit.width > availableWidth)
                || fit.vertical != (fit.height > availableHeight)) return 1;
            if (fit.width < 1100 || fit.height < 760) return 1;
        }
    const auto small = ViewportLayout::fit(1366, 736, 18);
    if (small.horizontal || !small.vertical || small.width != 1348) return 1;
    std::cout << "Viewport:36 sizes, scrollbar fixed point, no unnecessary horizontal bar: PASS\n";
}
