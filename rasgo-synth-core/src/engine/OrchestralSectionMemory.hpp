#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

#include "OrchestralRole.hpp"

namespace rasgo {

struct OrchestralSectionSignal {
    double rms = 0.0;
    double lowRms = 0.0;
    double midRms = 0.0;
    double highRms = 0.0;
    double audibleRatio = 0.0;
    double registerMidi = 60.0;
    bool active = false;
};

struct OrchestralSectionReport {
    OrganismStem strongest = OrganismStem::Other;
    double strongestRms = 0.0;
    double highBandShare = 0.0;
    double activeShare = 0.0;
    int activeCount = 0;
    int highConflictCount = 0;
    bool repeatedLeader = false;
};

// Bar/section-rate memory. It does not change sound and does not choose a
// gain. Its purpose is to turn observed ensemble behaviour into causal
// context: a layer that repeatedly dominates can later yield, answer, or
// change register without a global EQ rule.
class OrchestralSectionMemory {
public:
    static constexpr std::size_t kCount =
        static_cast<std::size_t>(OrganismStem::Count);

    void beginSection() noexcept {
        current_.fill(OrchestralSectionSignal{});
        observed_.fill(false);
    }

    void observe(OrganismStem stem, OrchestralSectionSignal signal) noexcept {
        const auto i = index(stem);
        if (i >= kCount) return;
        signal.rms = finiteNonNegative(signal.rms);
        signal.lowRms = finiteNonNegative(signal.lowRms);
        signal.midRms = finiteNonNegative(signal.midRms);
        signal.highRms = finiteNonNegative(signal.highRms);
        signal.audibleRatio = std::clamp(
            std::isfinite(signal.audibleRatio) ? signal.audibleRatio : 0.0,
            0.0, 1.0);
        signal.registerMidi = std::isfinite(signal.registerMidi)
            ? signal.registerMidi : 60.0;
        current_[i] = signal;
        observed_[i] = true;
    }

    OrchestralSectionReport finishSection() noexcept {
        OrchestralSectionReport report;
        double totalRms = 0.0;
        double totalHigh = 0.0;
        int observedCount = 0;
        for (std::size_t i = 0; i < kCount; ++i) {
            if (!observed_[i]) continue;
            ++observedCount;
            const auto &signal = current_[i];
            totalRms += signal.rms;
            totalHigh += signal.highRms;
            if (signal.active && signal.audibleRatio > 0.004) {
                ++report.activeCount;
                if (signal.rms > report.strongestRms) {
                    report.strongestRms = signal.rms;
                    report.strongest = static_cast<OrganismStem>(i);
                }
            }
        }
        report.activeShare = observedCount == 0
            ? 0.0 : static_cast<double>(report.activeCount) / observedCount;
        report.highBandShare = totalRms > 1.0e-12
            ? std::clamp(totalHigh / totalRms, 0.0, 1.0) : 0.0;

        for (std::size_t i = 0; i < kCount; ++i) {
            if (!observed_[i] || !current_[i].active) continue;
            const double ownHigh = current_[i].highRms;
            const double ownTotal = current_[i].lowRms + current_[i].midRms + ownHigh;
            if (ownTotal <= 1.0e-12 || ownHigh / ownTotal < 0.42) continue;
            for (std::size_t j = i + 1; j < kCount; ++j) {
                if (!observed_[j] || !current_[j].active) continue;
                const double otherHigh = current_[j].highRms;
                const double otherTotal = current_[j].lowRms
                    + current_[j].midRms + otherHigh;
                if (otherTotal > 1.0e-12 && otherHigh / otherTotal >= 0.42)
                    ++report.highConflictCount;
            }
        }

        report.repeatedLeader = report.strongest == lastLeader_
            && report.strongest != OrganismStem::Other;
        if (report.strongest != OrganismStem::Other) lastLeader_ = report.strongest;
        lastReport_ = report;
        ++sectionCount_;
        return report;
    }

    const OrchestralSectionReport &lastReport() const noexcept { return lastReport_; }
    int sectionCount() const noexcept { return sectionCount_; }
    OrganismStem lastLeader() const noexcept { return lastLeader_; }

private:
    static constexpr std::size_t index(OrganismStem stem) noexcept {
        return static_cast<std::size_t>(stem);
    }

    static double finiteNonNegative(double value) noexcept {
        return std::isfinite(value) ? std::max(0.0, value) : 0.0;
    }

    std::array<OrchestralSectionSignal, kCount> current_{};
    std::array<bool, kCount> observed_{};
    OrchestralSectionReport lastReport_{};
    OrganismStem lastLeader_ = OrganismStem::Other;
    int sectionCount_ = 0;
};

} // namespace rasgo
