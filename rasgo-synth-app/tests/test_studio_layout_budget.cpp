#include "../src/StudioLayoutBudget.h"
#include "../src/ViewportLayout.h"
#include <iostream>

int main() {
    using namespace rasgo::ui;
    int cases = 0;
    for (int width : {900, 1100, 1169, 1170, 1366, 1920})
        for (int height : {600, 736, 768, 1006, 1080})
            for (int cells : {90, 100}) {
                const auto minimum = [cells](int contentWidth) {
                    const int usable = contentWidth - 36;
                    const int columns = std::max(1, usable / 92);
                    return StudioLayoutBudget::minimumHeight(usable, ((cells + columns - 1) / columns) * 18);
                };
                const auto fit = ViewportLayout::fitContent(width, height, 18, minimum);
                if (fit.height < minimum(fit.width)) return 1;
                if (fit.horizontal != (fit.width > width - (fit.vertical ? 18 : 0))) return 2;
                if (fit.vertical != (fit.height > height - (fit.horizontal ? 18 : 0))) return 3;
                if (width >= 1366 && fit.horizontal) return 4;
                ++cases;
            }
    if (StudioLayoutBudget::minimumHeight(1330, 126) != 943) return 5;
    std::cout << "Studio layout: " << cases << " width/height/grid budgets; complete bottom band: PASS\n";
}
