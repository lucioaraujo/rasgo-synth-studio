#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace rasgo {

// Phase 167 is deliberately observational. These types describe what the
// existing renderer intended and planned, but own no oscillator, scheduler,
// gain or random source. Keeping the contract pure makes an enabled ledger
// sample-identical to an enabled=false counterfactual.
enum class ExpressiveEventSource : std::uint8_t {
    CoreSequence,
    Kick,
    Snare,
    Hihat,
    Cowbell,
    Clap,
    Tom,
    Cymbal,
    Maracas,
    Acid,
    FmFlute,
    Scat,
    Noise,
    Space,
    Structural,
    Count
};

inline constexpr const char *expressiveEventSourceName(
    ExpressiveEventSource source) noexcept {
    switch (source) {
        case ExpressiveEventSource::CoreSequence: return "core-sequence";
        case ExpressiveEventSource::Kick: return "kick";
        case ExpressiveEventSource::Snare: return "snare";
        case ExpressiveEventSource::Hihat: return "hihat";
        case ExpressiveEventSource::Cowbell: return "cowbell";
        case ExpressiveEventSource::Clap: return "clap";
        case ExpressiveEventSource::Tom: return "tom";
        case ExpressiveEventSource::Cymbal: return "cymbal";
        case ExpressiveEventSource::Maracas: return "maracas";
        case ExpressiveEventSource::Acid: return "acid";
        case ExpressiveEventSource::FmFlute: return "fm-flute";
        case ExpressiveEventSource::Scat: return "scat";
        case ExpressiveEventSource::Noise: return "noise";
        case ExpressiveEventSource::Space: return "space";
        case ExpressiveEventSource::Structural: return "structural";
        case ExpressiveEventSource::Count: break;
    }
    return "unknown";
}

enum class ExpressiveScale : std::uint8_t {
    Microevent,
    Step,
    PatternCycle,
    Form,
    History,
    Count
};

inline constexpr const char *expressiveScaleName(ExpressiveScale scale) noexcept {
    switch (scale) {
        case ExpressiveScale::Microevent: return "microevent";
        case ExpressiveScale::Step: return "step";
        case ExpressiveScale::PatternCycle: return "pattern-cycle";
        case ExpressiveScale::Form: return "form";
        case ExpressiveScale::History: return "history";
        case ExpressiveScale::Count: break;
    }
    return "unknown";
}

enum class ExpressiveDomain : std::uint8_t {
    Timing,
    Energy,
    Duration,
    Gesture,
    Pitch,
    Material,
    Space,
    Structure,
    Count
};

inline constexpr const char *expressiveDomainName(ExpressiveDomain domain) noexcept {
    switch (domain) {
        case ExpressiveDomain::Timing: return "timing";
        case ExpressiveDomain::Energy: return "energy";
        case ExpressiveDomain::Duration: return "duration";
        case ExpressiveDomain::Gesture: return "gesture";
        case ExpressiveDomain::Pitch: return "pitch";
        case ExpressiveDomain::Material: return "material";
        case ExpressiveDomain::Space: return "space";
        case ExpressiveDomain::Structure: return "structure";
        case ExpressiveDomain::Count: break;
    }
    return "unknown";
}

enum class ExpressiveCause : std::uint8_t {
    Grid,
    Humanization,
    Swing,
    PhraseGesture,
    RasgarGesture,
    Relation,
    MetricAccent,
    Density,
    Phrase,
    TimbreVariation,
    NoiseField,
    Narrative,
    Glitch,
    Rupture,
    Count
};

inline constexpr const char *expressiveCauseName(ExpressiveCause cause) noexcept {
    switch (cause) {
        case ExpressiveCause::Grid: return "grid";
        case ExpressiveCause::Humanization: return "humanization";
        case ExpressiveCause::Swing: return "swing";
        case ExpressiveCause::PhraseGesture: return "phrase-gesture";
        case ExpressiveCause::RasgarGesture: return "rasgar-gesture";
        case ExpressiveCause::Relation: return "relation";
        case ExpressiveCause::MetricAccent: return "metric-accent";
        case ExpressiveCause::Density: return "density";
        case ExpressiveCause::Phrase: return "phrase";
        case ExpressiveCause::TimbreVariation: return "timbre-variation";
        case ExpressiveCause::NoiseField: return "noise-field";
        case ExpressiveCause::Narrative: return "narrative";
        case ExpressiveCause::Glitch: return "glitch";
        case ExpressiveCause::Rupture: return "rupture";
        case ExpressiveCause::Count: break;
    }
    return "unknown";
}

using ExpressiveMask = std::uint32_t;

// Version of the compact expressiveTemporal metadata contract. Increment
// when vector order or scalar meaning changes; adding a new enum member also
// requires a corresponding contract review before changing this value.
inline constexpr int expressiveTemporalSchemaVersion = 1;

template <typename Enum>
inline constexpr ExpressiveMask expressiveBit(Enum value) noexcept {
    return ExpressiveMask{1u} << static_cast<unsigned>(value);
}

struct ExpressiveTimingPlan {
    int humanizationSamples = 0;
    int swingSamples = 0;
    int phraseGestureSamples = 0;
    int rasgarGestureSamples = 0;
    int relationSamples = 0;

    int totalSamples() const noexcept {
        const long long total = static_cast<long long>(humanizationSamples)
            + swingSamples + phraseGestureSamples + rasgarGestureSamples
            + relationSamples;
        return static_cast<int>(std::clamp(
            total,
            static_cast<long long>(std::numeric_limits<int>::min()),
            static_cast<long long>(std::numeric_limits<int>::max())));
    }
};

struct ExpressiveEventIntent {
    ExpressiveEventSource source = ExpressiveEventSource::CoreSequence;
    ExpressiveScale scale = ExpressiveScale::Step;
    std::uint64_t nominalSample = 0;
    int step = -1;
    int bar = -1;
    int section = -1;
    double pitchMidi = 0.0;
    double velocity = 0.0;
    double durationSeconds = 0.0;
    double density = 0.0;
    double accent = 0.0;
    double tension = 0.0;
    double entropy = 0.0;
    ExpressiveMask domains = 0;
    ExpressiveMask causes = 0;
    bool ghost = false;
    bool ratchet = false;
};

struct ExpressiveEventPlan {
    std::uint64_t performedSample = 0;
    ExpressiveTimingPlan timing{};
    double performedPitchMidi = 0.0;
    double performedVelocity = 0.0;
    double performedDurationSeconds = 0.0;
    double materialAmount = 0.0;
    double spatialAmount = 0.0;
    bool clamped = false;
};

struct ExpressiveEventSummary {
    bool enabled = false;
    std::uint64_t eventCount = 0;
    std::uint64_t earlyEventCount = 0;
    std::uint64_t onGridEventCount = 0;
    std::uint64_t lateEventCount = 0;
    std::uint64_t ghostEventCount = 0;
    std::uint64_t ratchetEventCount = 0;
    std::uint64_t clampedEventCount = 0;
    std::uint64_t invalidValueCount = 0;
    int minimumTimingSamples = 0;
    int maximumTimingSamples = 0;
    double meanTimingSamples = 0.0;
    double meanAbsoluteTimingSamples = 0.0;
    double minimumVelocity = 0.0;
    double maximumVelocity = 0.0;
    double minimumPitchMidi = 0.0;
    double maximumPitchMidi = 0.0;
    ExpressiveMask availableDomains = 0;
    ExpressiveMask manifestedDomains = 0;
    ExpressiveMask unusedDomains = 0;
    std::array<std::uint64_t,
        static_cast<std::size_t>(ExpressiveEventSource::Count)> sourceCounts{};
    std::array<std::uint64_t,
        static_cast<std::size_t>(ExpressiveScale::Count)> scaleCounts{};
    std::array<std::uint64_t,
        static_cast<std::size_t>(ExpressiveDomain::Count)> domainCounts{};
    std::array<std::uint64_t,
        static_cast<std::size_t>(ExpressiveCause::Count)> causeCounts{};
};

class ExpressiveEventLedger {
public:
    explicit ExpressiveEventLedger(bool enabled = true) noexcept
        : enabled_(enabled) {}

    bool enabled() const noexcept { return enabled_; }

    void markAvailable(ExpressiveDomain domain, bool available = true) noexcept {
        if (!enabled_ || !available || domain == ExpressiveDomain::Count) return;
        availableDomains_ |= expressiveBit(domain);
    }

    void observe(const ExpressiveEventIntent &rawIntent,
                 const ExpressiveEventPlan &rawPlan) noexcept {
        if (!enabled_) return;

        ExpressiveEventIntent intent = rawIntent;
        ExpressiveEventPlan plan = rawPlan;
        sanitize(intent, plan);

        const auto sourceIndex = static_cast<std::size_t>(intent.source);
        const auto scaleIndex = static_cast<std::size_t>(intent.scale);
        if (sourceIndex >= sourceCounts_.size() || scaleIndex >= scaleCounts_.size()) {
            ++invalidValueCount_;
            return;
        }

        ++eventCount_;
        ++sourceCounts_[sourceIndex];
        ++scaleCounts_[scaleIndex];
        if (intent.ghost) ++ghostEventCount_;
        if (intent.ratchet) ++ratchetEventCount_;
        if (plan.clamped) ++clampedEventCount_;

        for (std::size_t i = 0; i < domainCounts_.size(); ++i) {
            const ExpressiveMask bit = ExpressiveMask{1u} << i;
            if ((intent.domains & bit) != 0) {
                ++domainCounts_[i];
                manifestedDomains_ |= bit;
            }
        }
        for (std::size_t i = 0; i < causeCounts_.size(); ++i) {
            if ((intent.causes & (ExpressiveMask{1u} << i)) != 0) {
                ++causeCounts_[i];
            }
        }

        const int timing = plan.timing.totalSamples();
        if (timing < 0) ++earlyEventCount_;
        else if (timing > 0) ++lateEventCount_;
        else ++onGridEventCount_;
        timingSum_ += static_cast<long double>(timing);
        absoluteTimingSum_ += std::fabs(static_cast<long double>(timing));

        if (!hasTiming_) {
            minimumTimingSamples_ = maximumTimingSamples_ = timing;
            hasTiming_ = true;
        } else {
            minimumTimingSamples_ = std::min(minimumTimingSamples_, timing);
            maximumTimingSamples_ = std::max(maximumTimingSamples_, timing);
        }
        updateRange(plan.performedVelocity, hasVelocity_, minimumVelocity_, maximumVelocity_);
        if ((intent.domains & expressiveBit(ExpressiveDomain::Pitch)) != 0) {
            updateRange(plan.performedPitchMidi, hasPitch_, minimumPitchMidi_, maximumPitchMidi_);
        }
    }

    ExpressiveEventSummary summary() const noexcept {
        ExpressiveEventSummary result;
        result.enabled = enabled_;
        result.eventCount = eventCount_;
        result.earlyEventCount = earlyEventCount_;
        result.onGridEventCount = onGridEventCount_;
        result.lateEventCount = lateEventCount_;
        result.ghostEventCount = ghostEventCount_;
        result.ratchetEventCount = ratchetEventCount_;
        result.clampedEventCount = clampedEventCount_;
        result.invalidValueCount = invalidValueCount_;
        result.minimumTimingSamples = hasTiming_ ? minimumTimingSamples_ : 0;
        result.maximumTimingSamples = hasTiming_ ? maximumTimingSamples_ : 0;
        if (eventCount_ > 0) {
            const long double divisor = static_cast<long double>(eventCount_);
            result.meanTimingSamples = static_cast<double>(timingSum_ / divisor);
            result.meanAbsoluteTimingSamples = static_cast<double>(absoluteTimingSum_ / divisor);
        }
        result.minimumVelocity = hasVelocity_ ? minimumVelocity_ : 0.0;
        result.maximumVelocity = hasVelocity_ ? maximumVelocity_ : 0.0;
        result.minimumPitchMidi = hasPitch_ ? minimumPitchMidi_ : 0.0;
        result.maximumPitchMidi = hasPitch_ ? maximumPitchMidi_ : 0.0;
        result.availableDomains = availableDomains_;
        result.manifestedDomains = manifestedDomains_;
        result.unusedDomains = availableDomains_ & ~manifestedDomains_;
        result.sourceCounts = sourceCounts_;
        result.scaleCounts = scaleCounts_;
        result.domainCounts = domainCounts_;
        result.causeCounts = causeCounts_;
        return result;
    }

private:
    static double finiteOr(double value, double fallback,
                           std::uint64_t &invalidCounter) noexcept {
        if (std::isfinite(value)) return value;
        ++invalidCounter;
        return fallback;
    }

    void sanitize(ExpressiveEventIntent &intent,
                  ExpressiveEventPlan &plan) noexcept {
        intent.pitchMidi = finiteOr(intent.pitchMidi, 0.0, invalidValueCount_);
        intent.velocity = finiteOr(intent.velocity, 0.0, invalidValueCount_);
        intent.durationSeconds = finiteOr(intent.durationSeconds, 0.0, invalidValueCount_);
        intent.density = finiteOr(intent.density, 0.0, invalidValueCount_);
        intent.accent = finiteOr(intent.accent, 0.0, invalidValueCount_);
        intent.tension = finiteOr(intent.tension, 0.0, invalidValueCount_);
        intent.entropy = finiteOr(intent.entropy, 0.0, invalidValueCount_);
        plan.performedPitchMidi = finiteOr(
            plan.performedPitchMidi, intent.pitchMidi, invalidValueCount_);
        plan.performedVelocity = finiteOr(
            plan.performedVelocity, intent.velocity, invalidValueCount_);
        plan.performedDurationSeconds = finiteOr(
            plan.performedDurationSeconds, intent.durationSeconds, invalidValueCount_);
        plan.materialAmount = finiteOr(plan.materialAmount, 0.0, invalidValueCount_);
        plan.spatialAmount = finiteOr(plan.spatialAmount, 0.0, invalidValueCount_);
    }

    static void updateRange(double value, bool &hasValue,
                            double &minimum, double &maximum) noexcept {
        if (!hasValue) {
            minimum = maximum = value;
            hasValue = true;
        } else {
            minimum = std::min(minimum, value);
            maximum = std::max(maximum, value);
        }
    }

    bool enabled_ = true;
    ExpressiveMask availableDomains_ = 0;
    ExpressiveMask manifestedDomains_ = 0;
    std::uint64_t eventCount_ = 0;
    std::uint64_t earlyEventCount_ = 0;
    std::uint64_t onGridEventCount_ = 0;
    std::uint64_t lateEventCount_ = 0;
    std::uint64_t ghostEventCount_ = 0;
    std::uint64_t ratchetEventCount_ = 0;
    std::uint64_t clampedEventCount_ = 0;
    std::uint64_t invalidValueCount_ = 0;
    long double timingSum_ = 0.0;
    long double absoluteTimingSum_ = 0.0;
    bool hasTiming_ = false;
    int minimumTimingSamples_ = 0;
    int maximumTimingSamples_ = 0;
    bool hasVelocity_ = false;
    double minimumVelocity_ = 0.0;
    double maximumVelocity_ = 0.0;
    bool hasPitch_ = false;
    double minimumPitchMidi_ = 0.0;
    double maximumPitchMidi_ = 0.0;
    std::array<std::uint64_t,
        static_cast<std::size_t>(ExpressiveEventSource::Count)> sourceCounts_{};
    std::array<std::uint64_t,
        static_cast<std::size_t>(ExpressiveScale::Count)> scaleCounts_{};
    std::array<std::uint64_t,
        static_cast<std::size_t>(ExpressiveDomain::Count)> domainCounts_{};
    std::array<std::uint64_t,
        static_cast<std::size_t>(ExpressiveCause::Count)> causeCounts_{};
};

} // namespace rasgo
