#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "BassPerformancePlan.hpp"

namespace rasgo {

enum class BassBodyKind : std::uint8_t {
    None,
    Tuba,
    Acid,
    CoreSub,
    Count
};

struct BassBodyAvailability {
    bool tuba = false;
    bool acid = false;
    bool coreSub = true;
};

// Body-specific destinations remain normalized intentions. The renderer maps
// them into each voice's safe local range; no universal timbre is imposed.
struct BassBodyDirective {
    bool active = false;
    BassBodyKind body = BassBodyKind::None;
    int semitoneOffset = 0;
    BassRegisterIntent registerIntent = BassRegisterIntent::Low;
    double timingOffsetSteps = 0.0;
    double gate = 0.0;
    double presence = 0.0;
    double attack = 0.0;
    double release = 0.0;
    double breath = 0.0;
    double brightness = 0.0;
    double noise = 0.0;
    double pressure = 0.0;
    double drive = 0.0;
    double harmonicBody = 0.0;
    double kickDuck = 0.0;
    bool accent = false;
    bool slide = false;
    bool dead = false;

    bool valid() const noexcept {
        return body < BassBodyKind::Count
            && registerIntent < BassRegisterIntent::Count
            && semitoneOffset >= -24 && semitoneOffset <= 24
            && finiteRange(timingOffsetSteps, -0.25, 0.25)
            && unit(gate) && unit(presence) && unit(attack) && unit(release)
            && unit(breath) && unit(brightness) && unit(noise)
            && unit(pressure) && unit(drive) && unit(harmonicBody)
            && unit(kickDuck)
            && (active ? body != BassBodyKind::None : neutral());
    }

private:
    bool neutral() const noexcept {
        return body == BassBodyKind::None && gate == 0.0 && presence == 0.0
            && attack == 0.0 && release == 0.0 && breath == 0.0
            && brightness == 0.0 && noise == 0.0 && pressure == 0.0
            && drive == 0.0 && harmonicBody == 0.0 && kickDuck == 0.0
            && !accent && !slide && !dead;
    }
    static bool finiteRange(double value, double minimum, double maximum) noexcept {
        return std::isfinite(value) && value >= minimum && value <= maximum;
    }
    static bool unit(double value) noexcept {
        return finiteRange(value, 0.0, 1.0);
    }
};

struct BassBodyPlan {
    static constexpr std::size_t kMaxSteps = BassPerformancePlan::kMaxSteps;

    bool sourceValid = false;
    BassBodyKind primaryBody = BassBodyKind::None;
    std::array<BassBodyDirective, kMaxSteps> steps{};
    std::size_t stepCount = 0;
    std::size_t activeStepCount = 0;
    int tubaEventCount = 0;
    int acidEventCount = 0;
    int coreSubEventCount = 0;

    bool valid() const noexcept {
        if (!sourceValid || primaryBody >= BassBodyKind::Count
            || stepCount == 0 || stepCount > kMaxSteps
            || activeStepCount > stepCount || tubaEventCount < 0
            || acidEventCount < 0 || coreSubEventCount < 0)
            return false;
        std::size_t counted = 0;
        for (std::size_t i = 0; i < stepCount; ++i) {
            if (!steps[i].valid()) return false;
            counted += steps[i].active ? 1u : 0u;
        }
        if (counted != activeStepCount) return false;
        return activeStepCount == 0
            ? primaryBody == BassBodyKind::None
            : primaryBody != BassBodyKind::None;
    }
};

class BassBodyAdapterPlanner {
public:
    static BassBodyPlan plan(const BassPerformancePlan &performance,
                             BassBodyAvailability availability) noexcept {
        BassBodyPlan out;
        if (!performance.valid()) return out;
        out.sourceValid = true;
        out.stepCount = performance.stepCount;
        if (performance.silent()) return out;

        out.primaryBody = chooseBody(performance.role, availability);
        if (out.primaryBody == BassBodyKind::None) {
            out.sourceValid = false;
            return out;
        }

        for (std::size_t i = 0; i < out.stepCount; ++i) {
            const auto &source = performance.steps[i];
            if (!source.active()) continue;
            auto &target = out.steps[i];
            target.active = true;
            target.body = out.primaryBody;
            target.semitoneOffset = source.semitoneOffset;
            target.registerIntent = source.registerIntent;
            target.timingOffsetSteps = source.timingOffsetSteps;
            target.gate = source.gate;
            target.presence = clamp01(source.dynamicPeak * 0.42
                + source.weight * 0.34 + source.pressure * 0.24);
            target.pressure = source.pressure;
            target.brightness = source.brightness;
            target.noise = source.noise;
            target.drive = source.drive;
            target.dead = source.articulation == BassArticulationIntent::Dead;

            switch (target.body) {
                case BassBodyKind::Tuba:
                    adaptTuba(source, target);
                    ++out.tubaEventCount;
                    break;
                case BassBodyKind::Acid:
                    adaptAcid(source, target);
                    ++out.acidEventCount;
                    break;
                case BassBodyKind::CoreSub:
                    adaptCoreSub(source, target);
                    ++out.coreSubEventCount;
                    break;
                case BassBodyKind::None:
                case BassBodyKind::Count:
                    break;
            }
            ++out.activeStepCount;
        }
        return out;
    }

private:
    static BassBodyKind chooseBody(BassFunctionRole role,
                                   BassBodyAvailability available) noexcept {
        const bool acidRole = role == BassFunctionRole::Ostinato
            || role == BassFunctionRole::Energy
            || role == BassFunctionRole::Interlock;
        const bool breathRole = role == BassFunctionRole::Foundation
            || role == BassFunctionRole::Pedal || role == BassFunctionRole::Path;
        if (acidRole && available.acid) return BassBodyKind::Acid;
        if (breathRole && available.tuba) return BassBodyKind::Tuba;
        if (role == BassFunctionRole::Counterpoint && available.acid)
            return BassBodyKind::Acid;
        if (available.tuba) return BassBodyKind::Tuba;
        if (available.acid) return BassBodyKind::Acid;
        if (available.coreSub) return BassBodyKind::CoreSub;
        return BassBodyKind::None;
    }

    static void adaptTuba(const BassPerformanceStep &source,
                          BassBodyDirective &target) noexcept {
        // Tuba keeps wind causality: pressure becomes breath and spectral
        // opening, while rapid attacks are softened instead of copied from
        // a plucked/electronic bass articulation.
        target.attack = clamp01(0.18 + (1.0 - source.attackRenewal) * 0.48
                                + source.noise * 0.10);
        target.release = clamp01(0.24 + source.gate * 0.52);
        target.breath = clamp01(source.pressure * 0.48
                                + source.weight * 0.30
                                + source.dynamicPeak * 0.22);
        target.brightness = clamp01(source.brightness * 0.58
                                    + target.breath * 0.24
                                    + source.noise * 0.18);
        target.noise = clamp01(source.noise * 0.72 + target.dead * 0.18);
        target.harmonicBody = clamp01(source.weight * 0.62
                                      + (1.0 - source.brightness) * 0.20
                                      + source.gate * 0.18);
        target.drive = clamp01(source.drive * 0.28);
        target.slide = source.articulation == BassArticulationIntent::Slide
            || source.articulation == BassArticulationIntent::Connected;
        target.accent = source.articulation == BassArticulationIntent::Accent;
    }

    static void adaptAcid(const BassPerformanceStep &source,
                          BassBodyDirective &target) noexcept {
        // Acid retains its own 303 identity: renewal becomes envelope
        // retrigger, pressure opens the filter, and slide remains a tie.
        target.attack = clamp01(0.78 + source.attackRenewal * 0.22);
        target.release = clamp01(0.16 + source.gate * 0.38);
        target.breath = 0.0;
        target.brightness = clamp01(source.brightness * 0.44
                                    + source.pressure * 0.34
                                    + source.drive * 0.22);
        target.noise = clamp01(source.noise * 0.18);
        target.drive = clamp01(source.drive * 0.62 + source.pressure * 0.28);
        target.harmonicBody = clamp01(source.weight * 0.44
                                      + target.drive * 0.36
                                      + source.dynamicPeak * 0.20);
        target.slide = source.articulation == BassArticulationIntent::Slide
            || source.articulation == BassArticulationIntent::Connected
            || source.attackRenewal < 0.34;
        target.accent = source.articulation == BassArticulationIntent::Accent
            || source.dynamicPeak > 0.72;
    }

    static void adaptCoreSub(const BassPerformanceStep &source,
                             BassBodyDirective &target) noexcept {
        // Fallback is restrained: stable fundamental plus a bounded second
        // harmonic for articulation, with explicit kick ducking. It is not a
        // copy of Aquorbium's BiomaBass research.
        target.attack = clamp01(0.10 + (1.0 - source.attackRenewal) * 0.30);
        target.release = clamp01(0.18 + source.gate * 0.42);
        target.breath = 0.0;
        target.brightness = clamp01(source.brightness * 0.24);
        target.noise = clamp01(source.noise * 0.08);
        target.drive = clamp01(source.drive * 0.20);
        target.harmonicBody = clamp01(0.18 + source.brightness * 0.24
                                      + source.weight * 0.22);
        target.kickDuck = source.kickRelation == BassKickRelation::Coincident
            ? clamp01(0.30 + source.weight * 0.28) : 0.08;
        target.slide = source.articulation == BassArticulationIntent::Slide
            || source.articulation == BassArticulationIntent::Connected;
        target.accent = source.articulation == BassArticulationIntent::Accent;
    }

    static double clamp01(double value) noexcept {
        return std::clamp(value, 0.0, 1.0);
    }
};

inline const char *bassBodyName(BassBodyKind body) noexcept {
    switch (body) {
        case BassBodyKind::None: return "none";
        case BassBodyKind::Tuba: return "tuba";
        case BassBodyKind::Acid: return "acid";
        case BassBodyKind::CoreSub: return "core-sub";
        case BassBodyKind::Count: break;
    }
    return "unknown";
}

} // namespace rasgo
