#pragma once

#include <algorithm>
#include <cstdint>

namespace rasgo {

// Original Studio realization of bounded within-step repeats. A delayed head
// owns the remaining step window; no free-running clock or unbounded queue.
// The first attack remains unchanged. Subsequent attacks use the remembered
// head velocity, not a multiplication accumulated from earlier repeats.
class GrooveRepeat {
public:
    void arm(unsigned requested, int stepSamples, int headDelay,
             int minimumSpacing) noexcept {
        reset();
        if (stepSamples <= 0 || headDelay < 0 || headDelay >= stepSamples) return;
        window_ = stepSamples - headDelay;
        const int spacing = std::max(1, minimumSpacing);
        count_ = std::min(std::clamp(requested, 1u, 4u),
            static_cast<unsigned>(std::max(1, window_ / spacing)));
    }

    // Called only when the voice actually triggers. Returns true for an extra
    // attack, never for the original head. The voice's current output gain
    // must not change until that extra attack fires.
    bool beginTrigger(float& velocity) noexcept {
        if (index_ == 0) { headVelocity_ = velocity; return false; }
        velocity = headVelocity_ * (1.0f - .60f * static_cast<float>(index_)
            / static_cast<float>(count_));
        return true;
    }

    // Existing countdowns trigger at zero on the next loop iteration, hence
    // distance-1. Round absolute positions so thirds do not accumulate drift.
    int nextDelay() noexcept {
        if (index_ + 1u >= count_) { reset(); return -1; }
        const auto previous = position(index_);
        ++index_;
        return static_cast<int>(position(index_) - previous - 1);
    }
    void reset() noexcept { count_ = 1; index_ = 0; window_ = 0; headVelocity_ = 0; }
    unsigned count() const noexcept { return count_; }
private:
    std::int64_t position(unsigned index) const noexcept {
        return (static_cast<std::int64_t>(window_) * index + count_ / 2u) / count_;
    }
    unsigned count_ = 1, index_ = 0;
    int window_ = 0;
    float headVelocity_ = 0;
};
} // namespace rasgo
