#pragma once

#include <algorithm>
#include <cstdint>

namespace rasgo {

// A small phrase-level memory for melodic pitch selection. It does not pick
// notes, scales or chords; it only gives a bounded preference to a direction
// while a phrase unfolds, then turns that direction around near an apex. The
// caller remains free to ignore it, so this is a contour intention rather
// than a hidden second sequencer.
class MelodicContourMemory {
public:
    void configure(std::uint32_t seed, int phraseBars = 4) noexcept {
        state_ = seed == 0 ? 1u : seed;
        phraseBars_ = std::max(2, phraseBars);
        phraseBar_ = phraseBars_;
        direction_ = 0;
        apexBar_ = phraseBars_ / 2;
    }

    void beginPhrase() noexcept {
        direction_ = nextSigned();
        if (direction_ == 0) direction_ = 1;
        const int lower = std::max(1, phraseBars_ / 3);
        const int upper = std::max(lower, phraseBars_ - 1);
        apexBar_ = lower + static_cast<int>(nextUnit() * (upper - lower + 1));
        phraseBar_ = 0;
    }

    void beginBar() noexcept {
        if (phraseBar_ >= phraseBars_) beginPhrase();
    }

    // Returns a bounded multiplier for a candidate pitch offset (semitones
    // from the current root). Positive contour prefers pitches above the
    // previous pitch before the apex and below it afterwards; negative
    // contour does the inverse. Using offsets rather than scale indices keeps
    // the preference musically coherent when primary/secondary collections
    // have different numbers of degrees.
    double pitchWeight(int candidateOffset, int previousOffset, int bar) const noexcept {
        if (direction_ == 0 || candidateOffset == previousOffset) return 1.0;
        const bool beforeApex = bar < apexBar_;
        const int desired = (beforeApex ? direction_ : -direction_);
        const int delta = candidateOffset - previousOffset;
        const double distanceFromApex = std::abs(bar - apexBar_);
        const double strength = std::clamp(0.34 * (distanceFromApex + 1.0), 0.0, 0.34);
        return delta * desired > 0 ? 1.0 + strength : 1.0 - strength * 0.55;
    }

    void finishBar() noexcept { ++phraseBar_; }
    int direction() const noexcept { return direction_; }
    int apexBar() const noexcept { return apexBar_; }
    int phraseBar() const noexcept { return phraseBar_; }

private:
    double nextUnit() noexcept {
        state_ = state_ * 1664525u + 1013904223u;
        return static_cast<double>(state_) / 4294967296.0;
    }

    int nextSigned() noexcept {
        const double value = nextUnit();
        return value < 0.3333333333 ? -1 : value > 0.6666666666 ? 1 : 0;
    }

    std::uint32_t state_ = 1u;
    int phraseBars_ = 4;
    int phraseBar_ = 4;
    int direction_ = 0;
    int apexBar_ = 2;
};

} // namespace rasgo
