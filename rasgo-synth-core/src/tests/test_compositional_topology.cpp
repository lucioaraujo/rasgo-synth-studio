#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "engine/CompositionalTopology.hpp"

using namespace rasgo;

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif

int main() {
    OpenModuleArchive archive;
    CompositionalTopology topology(archive);

    const RegionId rhythm = topology.addRegion(RegionRole::Rhythm, 0.25, 0.7, 0.55);
    const RegionId harmony = topology.addRegion(RegionRole::Harmony, -0.1, 0.4, 0.35);
    const std::uint32_t reading = topology.connect(
        rhythm, harmony, ModuleDomain::Rhythm, ModuleDomain::Harmony,
        PassageMode::Accumulate, 1.1, 0.2, false);
    assert(reading != 0);

    TraversalResult first = topology.traverse(reading, rhythm, 0.8, 1);
    assert(first.crossed);
    assert(first.inputDomain == ModuleDomain::Rhythm);
    assert(first.outputDomain == ModuleDomain::Harmony);
    assert(std::isfinite(first.output));
    assert(topology.traversalCount() == 1);
    assert(archive.count(ArchiveAction::Traverse) == 1);
    assert(archive.count(ArchiveAction::Reinterpret) == 1);

    const double harmonyBeforeSplit = topology.region(harmony)->state;
    const CismResult cism = topology.split(harmony, 0.82, ModuleDomain::Harmony, 2);
    assert(cism.original == harmony);
    assert(cism.interior != 0 && cism.exterior != 0 && cism.opening != 0);
    assert(!topology.region(harmony)->active);
    assert(topology.region(cism.interior)->active);
    assert(topology.region(cism.exterior)->active);
    assert(topology.region(cism.interior)->parent == harmony);
    assert(topology.region(cism.exterior)->parent == harmony);
    assert(topology.region(cism.interior)->state != topology.region(cism.exterior)->state);
    assert(topology.region(cism.interior)->state != harmonyBeforeSplit);
    assert(topology.splitCount() == 1);
    assert(archive.count(ArchiveAction::Split) == 1);

    const TopologyPassage *opening = topology.passage(cism.opening);
    assert(opening && opening->bidirectional && opening->open);
    const double resistanceBefore = opening->resistance;
    TraversalResult crossed = topology.traverse(cism.opening, cism.interior, 0.9, 3);
    assert(crossed.crossed && crossed.to == cism.exterior);
    assert(std::isfinite(crossed.output));
    assert(topology.passage(cism.opening)->resistance <= resistanceBefore);
    assert(topology.passage(cism.opening)->traversalCount == 1);

    FeedbackResult feedback = topology.feedback(cism.opening, cism.interior,
                                                0.6, -0.75, 4);
    assert(feedback.outward.crossed);
    assert(feedback.returning.crossed);
    assert(feedback.returning.to == cism.interior);
    assert(topology.passage(cism.opening)->traversalCount == 3);

    auto latestHarmony = archive.latest(ModuleDomain::Harmony);
    assert(latestHarmony.has_value());
    auto reread = archive.readLatest(cism.interior, ModuleDomain::Harmony, 5, true);
    assert(reread.has_value());
    assert(archive.count(ArchiveAction::Reread) == 1);

    // Same construction is deterministic and different force changes the
    // descendant states: cism is a state transformation, not a copy.
    OpenModuleArchive archive2;
    CompositionalTopology topology2(archive2);
    const RegionId same = topology2.addRegion(RegionRole::Harmony,
                                               harmonyBeforeSplit, 0.4, 0.35);
    const CismResult weaker = topology2.split(same, 0.2, ModuleDomain::Harmony, 2);
    assert(topology2.region(weaker.interior)->state
           != topology.region(cism.interior)->state);

    std::cout << "compositional topology tests passed\n";
    return 0;
}
