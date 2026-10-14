#pragma once
#include <vector>
#include <algorithm>

namespace rasgo {

// Bjorklund's algorithm: distributes `pulses` onsets as evenly as possible
// across `steps` slots. This is the standard "Euclidean rhythm" generator
// (the same logic behind genres from Steve Reich to countless drum
// machines); a nice complement to the original scale-based random pattern
// generator, since it gives evenly-distributed rhythmic structure that the
// melodic randomizer can then paint notes on top of.
inline std::vector<bool> euclideanRhythm(int steps, int pulses) {
    if (steps <= 0) return {};
    if (pulses <= 0) return std::vector<bool>(static_cast<size_t>(steps), false);
    if (pulses >= steps) return std::vector<bool>(static_cast<size_t>(steps), true);

    std::vector<std::vector<bool>> a(static_cast<size_t>(pulses), std::vector<bool>{true});
    std::vector<std::vector<bool>> b(static_cast<size_t>(steps - pulses), std::vector<bool>{false});

    while (b.size() > 1) {
        size_t n = std::min(a.size(), b.size());
        std::vector<std::vector<bool>> newA;
        newA.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            std::vector<bool> combined = a[i];
            combined.insert(combined.end(), b[i].begin(), b[i].end());
            newA.push_back(std::move(combined));
        }
        std::vector<std::vector<bool>> newB;
        if (a.size() > n) {
            newB.assign(a.begin() + static_cast<long>(n), a.end());
        } else {
            newB.assign(b.begin() + static_cast<long>(n), b.end());
        }
        a = std::move(newA);
        b = std::move(newB);
    }

    std::vector<bool> result;
    result.reserve(static_cast<size_t>(steps));
    for (auto &group : a) result.insert(result.end(), group.begin(), group.end());
    for (auto &group : b) result.insert(result.end(), group.begin(), group.end());
    return result;
}

} // namespace rasgo
