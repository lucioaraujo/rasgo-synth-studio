#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "MaterialAgency.hpp"

namespace rasgo {

// Performance is broader than pitched notes. Each active sound-producing
// organism declares the kind of gesture it can realize; infrastructure such
// as mixers, meters and limiters never enters this registry.
enum class PerformanceGestureFamily {
    Tone,
    Hit,
    Texture,
    Pattern,
    Rupture,
    Relation,
    Count
};

inline const char *performanceGestureFamilyName(PerformanceGestureFamily family) {
    static const char *names[] = {
        "tone", "hit", "texture", "pattern", "rupture", "relation"
    };
    const auto index = static_cast<std::size_t>(family);
    return index < static_cast<std::size_t>(PerformanceGestureFamily::Count)
        ? names[index] : "unknown";
}

enum class PerformanceCapability : std::uint32_t {
    None = 0,
    PitchTrajectory = 1u << 0,
    DynamicTrajectory = 1u << 1,
    SpectralTrajectory = 1u << 2,
    OnsetShape = 1u << 3,
    ReleaseShape = 1u << 4,
    Transition = 1u << 5,
    Impact = 1u << 6,
    PatternTiming = 1u << 7,
    Density = 1u << 8,
    FailureRecovery = 1u << 9,
    SpatialTrajectory = 1u << 10
};

constexpr PerformanceCapability operator|(PerformanceCapability left,
                                           PerformanceCapability right) noexcept {
    return static_cast<PerformanceCapability>(
        static_cast<std::uint32_t>(left) | static_cast<std::uint32_t>(right));
}

constexpr PerformanceCapability operator&(PerformanceCapability left,
                                           PerformanceCapability right) noexcept {
    return static_cast<PerformanceCapability>(
        static_cast<std::uint32_t>(left) & static_cast<std::uint32_t>(right));
}

constexpr bool hasAllCapabilities(PerformanceCapability available,
                                  PerformanceCapability required) noexcept {
    const auto availableBits = static_cast<std::uint32_t>(available);
    const auto requiredBits = static_cast<std::uint32_t>(required);
    return (availableBits & requiredBits) == requiredBits;
}

struct PerformanceActor {
    std::uint16_t moduleIndex = 0;
    PerformanceGestureFamily gestureFamily = PerformanceGestureFamily::Texture;
    AgencyFamily agencyFamily = AgencyFamily::Core;
    PerformanceCapability capabilities = PerformanceCapability::None;
    bool active = false;
    bool infrastructure = false;
    double salience = 0.5;
    double observedAudibility = 0.0;
    double gestureBudget = 0.5;
};

struct PerformanceSelectionRequest {
    bool constrainFamily = false;
    PerformanceGestureFamily family = PerformanceGestureFamily::Tone;
    PerformanceCapability requiredCapabilities = PerformanceCapability::None;
    bool constrainModule = false;
    std::uint16_t moduleIndex = 0;
    double minimumAudibility = 0.0;
    std::size_t maximumActors = 1;
};

struct PerformanceActorSelection {
    static constexpr std::size_t capacity = 4;
    std::array<PerformanceActor, capacity> actors{};
    std::size_t size = 0;

    bool empty() const noexcept { return size == 0; }
};

// Fixed-capacity and allocation-free after construction. It can be populated
// once the seed has decided which organisms exist, then safely consulted by
// section/event planning without consuming the composition's musical RNG.
class PerformanceActorRegistry {
public:
    static constexpr std::size_t capacity = 96;

    bool add(const PerformanceActor &candidate) noexcept {
        if (!candidate.active || candidate.infrastructure
            || candidate.capabilities == PerformanceCapability::None
            || candidate.moduleIndex >= capacity || size_ >= capacity
            || find(candidate.moduleIndex) != nullptr) {
            return false;
        }
        actors_[size_] = sanitize(candidate);
        ++size_;
        return true;
    }

    std::size_t size() const noexcept { return size_; }
    bool empty() const noexcept { return size_ == 0; }

    const PerformanceActor *find(std::uint16_t moduleIndex) const noexcept {
        for (std::size_t i = 0; i < size_; ++i) {
            if (actors_[i].moduleIndex == moduleIndex) return &actors_[i];
        }
        return nullptr;
    }

    // Read-only traversal for a control-side export. The registry remains the
    // sole authority on which Studio organisms actually manifested.
    const PerformanceActor *at(std::size_t index) const noexcept {
        return index < size_ ? &actors_[index] : nullptr;
    }

    std::size_t count(PerformanceGestureFamily family) const noexcept {
        std::size_t result = 0;
        for (std::size_t i = 0; i < size_; ++i) {
            if (actors_[i].gestureFamily == family) ++result;
        }
        return result;
    }

    bool spend(std::uint16_t moduleIndex, double amount) noexcept {
        for (std::size_t i = 0; i < size_; ++i) {
            if (actors_[i].moduleIndex != moduleIndex) continue;
            actors_[i].gestureBudget = clamp01(
                actors_[i].gestureBudget - std::max(0.0, finite(amount, 0.0)));
            return true;
        }
        return false;
    }

    PerformanceActorSelection select(unsigned seed, std::uint32_t sectionIndex,
                                     const PerformanceSelectionRequest &request) const noexcept {
        PerformanceActorSelection result;
        const std::size_t wanted = std::min(
            request.maximumActors, PerformanceActorSelection::capacity);
        std::array<bool, capacity> selected{};

        while (result.size < wanted) {
            std::size_t best = capacity;
            double bestScore = -1.0;
            for (std::size_t i = 0; i < size_; ++i) {
                if (selected[i] || !matches(actors_[i], request)) continue;
                const double tieBreak = deterministicUnit(
                    seed, sectionIndex, actors_[i].moduleIndex);
                const double score = actors_[i].salience * 0.44
                    + actors_[i].observedAudibility * 0.30
                    + actors_[i].gestureBudget * 0.24
                    + tieBreak * 0.02;
                if (score > bestScore) {
                    bestScore = score;
                    best = i;
                }
            }
            if (best == capacity) break;
            selected[best] = true;
            result.actors[result.size++] = actors_[best];
        }
        return result;
    }

private:
    std::array<PerformanceActor, capacity> actors_{};
    std::size_t size_ = 0;

    static bool matches(const PerformanceActor &actor,
                        const PerformanceSelectionRequest &request) noexcept {
        if (request.constrainFamily && actor.gestureFamily != request.family)
            return false;
        if (request.constrainModule && actor.moduleIndex != request.moduleIndex)
            return false;
        if (!hasAllCapabilities(actor.capabilities, request.requiredCapabilities))
            return false;
        return actor.observedAudibility >= clamp01(request.minimumAudibility)
            && actor.gestureBudget > 0.0;
    }

    static PerformanceActor sanitize(const PerformanceActor &actor) noexcept {
        PerformanceActor out = actor;
        out.salience = clamp01(out.salience);
        out.observedAudibility = clamp01(out.observedAudibility);
        out.gestureBudget = clamp01(out.gestureBudget);
        return out;
    }

    static double finite(double value, double fallback) noexcept {
        return std::isfinite(value) ? value : fallback;
    }

    static double clamp01(double value) noexcept {
        return std::clamp(finite(value, 0.0), 0.0, 1.0);
    }

    static double deterministicUnit(unsigned seed, std::uint32_t sectionIndex,
                                    std::uint16_t moduleIndex) noexcept {
        std::uint64_t value = static_cast<std::uint64_t>(seed)
            ^ (static_cast<std::uint64_t>(sectionIndex) << 32)
            ^ (static_cast<std::uint64_t>(moduleIndex) * 0x9e3779b97f4a7c15ULL);
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        value ^= value >> 31;
        return static_cast<double>(value >> 11)
            * (1.0 / 9007199254740992.0);
    }
};

enum class PerformanceAuditTargetKind {
    Module,
    Family,
    Capability
};

struct PerformanceAuditTarget {
    PerformanceAuditTargetKind kind = PerformanceAuditTargetKind::Family;
    std::uint16_t moduleIndex = 0;
    PerformanceGestureFamily family = PerformanceGestureFamily::Tone;
    PerformanceCapability capability = PerformanceCapability::None;
};

struct PerformanceAuditSeed {
    bool found = false;
    unsigned seed = 0;
    std::size_t attempts = 0;
};

class PerformanceAuditSelector {
public:
    static bool matches(const PerformanceActorRegistry &registry,
                        const PerformanceAuditTarget &target) noexcept {
        if (target.kind == PerformanceAuditTargetKind::Module)
            return registry.find(target.moduleIndex) != nullptr;
        PerformanceSelectionRequest request;
        request.minimumAudibility = 0.0;
        request.maximumActors = 1;
        if (target.kind == PerformanceAuditTargetKind::Family) {
            request.constrainFamily = true;
            request.family = target.family;
        } else {
            request.requiredCapabilities = target.capability;
        }
        return !registry.select(0, 0, request).empty();
    }

    // Probe must return the actor registry that the candidate seed would
    // activate. This makes the audit generic: flute, drums, textures, patterns
    // and ruptures use the same search protocol without being forced on in
    // ordinary compositions.
    template <typename RegistryProbe>
    static PerformanceAuditSeed findNext(unsigned firstSeed,
                                         std::size_t maximumAttempts,
                                         const PerformanceAuditTarget &target,
                                         RegistryProbe probe) {
        PerformanceAuditSeed result;
        for (std::size_t offset = 0; offset < maximumAttempts; ++offset) {
            const std::uint64_t candidate = static_cast<std::uint64_t>(firstSeed)
                + static_cast<std::uint64_t>(offset);
            if (candidate > std::numeric_limits<unsigned>::max()) break;
            ++result.attempts;
            const unsigned seed = static_cast<unsigned>(candidate);
            if (!matches(probe(seed), target)) continue;
            result.found = true;
            result.seed = seed;
            return result;
        }
        return result;
    }
};

} // namespace rasgo
