#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../engine/PerformanceActorRegistry.hpp"

namespace {
int failures = 0;

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

rasgo::PerformanceActor actor(
    std::uint16_t moduleIndex,
    rasgo::PerformanceGestureFamily family,
    rasgo::PerformanceCapability capabilities,
    double audibility = 0.5,
    double salience = 0.5,
    double budget = 0.5) {
    rasgo::PerformanceActor result;
    result.moduleIndex = moduleIndex;
    result.gestureFamily = family;
    result.capabilities = capabilities;
    result.active = true;
    result.observedAudibility = audibility;
    result.salience = salience;
    result.gestureBudget = budget;
    return result;
}
}

int main() {
    using namespace rasgo;

    PerformanceActorRegistry registry;
    check(registry.add(actor(7, PerformanceGestureFamily::Hit,
                             PerformanceCapability::Impact
                                 | PerformanceCapability::DynamicTrajectory,
                             0.78, 0.72, 0.84)),
          "active hit organism enters registry");
    check(registry.add(actor(42, PerformanceGestureFamily::Tone,
                             PerformanceCapability::PitchTrajectory
                                 | PerformanceCapability::DynamicTrajectory
                                 | PerformanceCapability::ReleaseShape,
                             0.45, 0.68, 0.70)),
          "active tonal organism enters registry");
    check(registry.add(actor(58, PerformanceGestureFamily::Texture,
                             PerformanceCapability::SpectralTrajectory
                                 | PerformanceCapability::DynamicTrajectory,
                             0.63, 0.60, 0.76)),
          "active texture organism enters registry");

    auto inactive = actor(61, PerformanceGestureFamily::Rupture,
                          PerformanceCapability::FailureRecovery);
    inactive.active = false;
    check(!registry.add(inactive), "absent seed module is never forced into registry");
    auto infrastructure = actor(75, PerformanceGestureFamily::Relation,
                                PerformanceCapability::DynamicTrajectory);
    infrastructure.infrastructure = true;
    check(!registry.add(infrastructure), "mixer or infrastructure cannot become performer");
    check(!registry.add(actor(7, PerformanceGestureFamily::Pattern,
                              PerformanceCapability::PatternTiming)),
          "observable module index remains unique");
    check(registry.size() == 3, "registry contains only eligible active organisms");

    PerformanceSelectionRequest hitRequest;
    hitRequest.constrainFamily = true;
    hitRequest.family = PerformanceGestureFamily::Hit;
    const auto hit = registry.select(9182, 3, hitRequest);
    check(hit.size == 1 && hit.actors[0].moduleIndex == 7,
          "family selection does not privilege tonal actor");

    PerformanceSelectionRequest spectralRequest;
    spectralRequest.requiredCapabilities = PerformanceCapability::SpectralTrajectory;
    const auto spectral = registry.select(9182, 3, spectralRequest);
    check(spectral.size == 1 && spectral.actors[0].moduleIndex == 58,
          "capability selection reaches texture actor");

    PerformanceSelectionRequest pluralRequest;
    pluralRequest.requiredCapabilities = PerformanceCapability::DynamicTrajectory;
    pluralRequest.maximumActors = 3;
    const auto pluralA = registry.select(9182, 3, pluralRequest);
    const auto pluralB = registry.select(9182, 3, pluralRequest);
    check(pluralA.size == 3, "several active families can share a bounded scene");
    check(pluralA.size == pluralB.size
              && pluralA.actors[0].moduleIndex == pluralB.actors[0].moduleIndex
              && pluralA.actors[1].moduleIndex == pluralB.actors[1].moduleIndex
              && pluralA.actors[2].moduleIndex == pluralB.actors[2].moduleIndex,
          "selection is deterministic without musical RNG consumption");

    check(registry.spend(7, 0.84), "gesture budget can be consumed after an event");
    const auto exhaustedHit = registry.select(9182, 4, hitRequest);
    check(exhaustedHit.empty(), "exhausted actor yields instead of filling every moment");

    PerformanceAuditTarget ruptureTarget;
    ruptureTarget.kind = PerformanceAuditTargetKind::Family;
    ruptureTarget.family = PerformanceGestureFamily::Rupture;
    const auto audit = PerformanceAuditSelector::findNext(
        100, 20, ruptureTarget, [](unsigned seed) {
            PerformanceActorRegistry candidate;
            candidate.add(actor(7, PerformanceGestureFamily::Hit,
                                PerformanceCapability::Impact));
            if (seed % 7 == 0) {
                candidate.add(actor(61, PerformanceGestureFamily::Rupture,
                                    PerformanceCapability::FailureRecovery));
            }
            return candidate;
        });
    check(audit.found && audit.seed == 105 && audit.attempts == 6,
          "generic audit locates a seed for requested family");

    PerformanceAuditTarget moduleTarget;
    moduleTarget.kind = PerformanceAuditTargetKind::Module;
    moduleTarget.moduleIndex = 61;
    const auto moduleAudit = PerformanceAuditSelector::findNext(
        105, 1, moduleTarget, [](unsigned seed) {
            PerformanceActorRegistry candidate;
            if (seed == 105) {
                candidate.add(actor(61, PerformanceGestureFamily::Rupture,
                                    PerformanceCapability::FailureRecovery));
            }
            return candidate;
        });
    check(moduleAudit.found && moduleAudit.seed == 105,
          "audit can target any registered module, not a hardcoded flute");

    PerformanceActorRegistry population;
    for (std::uint16_t index = 0; index < 90; ++index) {
        const auto family = static_cast<PerformanceGestureFamily>(
            index % static_cast<std::uint16_t>(PerformanceGestureFamily::Count));
        check(population.add(actor(index, family,
                                   PerformanceCapability::DynamicTrajectory)),
              "registry accepts current observable population scale");
    }
    check(population.size() == 90, "registry is sized for the complete current panel");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "PerformanceActorRegistry tests passed\n";
    return EXIT_SUCCESS;
}
