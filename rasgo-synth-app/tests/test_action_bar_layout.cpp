#include "../src/ActionBarLayout.h"
#include <cstdlib>
#include <iostream>
int main() {
    using rasgo::ui::ActionBarLayout;
    for (int width : {900, 1000, 1040, 1080, 1100, 1133})
        if (!ActionBarLayout::splitRows(width)
            || ActionBarLayout::waveformWidth > width
            || ActionBarLayout::settingsWidth > width) return EXIT_FAILURE;
    for (int width : {1134, 1366, 1920})
        if (ActionBarLayout::splitRows(width)) return EXIT_FAILURE;
    std::cout << "Action row width budgets and wrap threshold: PASS (not visual acceptance)\n";
}
