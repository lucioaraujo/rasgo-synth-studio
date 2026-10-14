#include "../engine/ExpressiveEventLedger.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

rasgo::ExpressiveEventIntent drumIntent() {
    using namespace rasgo;
    ExpressiveEventIntent intent;
    intent.source = ExpressiveEventSource::Snare;
    intent.scale = ExpressiveScale::Step;
    intent.nominalSample = 1000;
    intent.step = 4;
    intent.bar = 2;
    intent.section = 1;
    intent.velocity = 0.72;
    intent.durationSeconds = 0.125;
    intent.density = 0.6;
    intent.accent = 0.8;
    intent.domains = expressiveBit(ExpressiveDomain::Timing)
        | expressiveBit(ExpressiveDomain::Energy)
        | expressiveBit(ExpressiveDomain::Material);
    intent.causes = expressiveBit(ExpressiveCause::Grid)
        | expressiveBit(ExpressiveCause::Humanization)
        | expressiveBit(ExpressiveCause::Swing)
        | expressiveBit(ExpressiveCause::MetricAccent);
    return intent;
}

rasgo::ExpressiveEventPlan drumPlan() {
    rasgo::ExpressiveEventPlan plan;
    plan.performedSample = 1019;
    plan.timing.humanizationSamples = 5;
    plan.timing.swingSamples = 12;
    plan.timing.phraseGestureSamples = 3;
    plan.timing.rasgarGestureSamples = 1;
    plan.timing.relationSamples = -2;
    plan.performedVelocity = 0.72;
    plan.performedDurationSeconds = 0.125;
    plan.materialAmount = 0.3;
    return plan;
}

} // namespace

int main() {
    using namespace rasgo;

    check(expressiveTemporalSchemaVersion == 1,
          "compact metadata schema version is explicit and stable");

    ExpressiveEventLedger disabled(false);
    disabled.markAvailable(ExpressiveDomain::Timing);
    disabled.observe(drumIntent(), drumPlan());
    const auto disabledSummary = disabled.summary();
    check(!disabledSummary.enabled && disabledSummary.eventCount == 0,
          "disabled ledger is exactly observationally empty");
    check(disabledSummary.availableDomains == 0,
          "disabled ledger does not report availability");

    ExpressiveEventLedger ledger;
    ledger.markAvailable(ExpressiveDomain::Timing);
    ledger.markAvailable(ExpressiveDomain::Energy);
    ledger.markAvailable(ExpressiveDomain::Material);
    ledger.markAvailable(ExpressiveDomain::Space);
    ledger.observe(drumIntent(), drumPlan());

    auto earlyIntent = drumIntent();
    earlyIntent.source = ExpressiveEventSource::Hihat;
    earlyIntent.scale = ExpressiveScale::Microevent;
    earlyIntent.ghost = true;
    earlyIntent.ratchet = true;
    auto earlyPlan = drumPlan();
    earlyPlan.timing = {};
    earlyPlan.timing.rasgarGestureSamples = -4;
    earlyPlan.performedVelocity = 0.24;
    ledger.observe(earlyIntent, earlyPlan);

    auto invalidIntent = drumIntent();
    invalidIntent.source = ExpressiveEventSource::FmFlute;
    invalidIntent.scale = ExpressiveScale::Form;
    invalidIntent.domains |= expressiveBit(ExpressiveDomain::Pitch);
    invalidIntent.pitchMidi = 73.0;
    auto invalidPlan = drumPlan();
    invalidPlan.timing = {};
    invalidPlan.performedPitchMidi = std::numeric_limits<double>::quiet_NaN();
    invalidPlan.performedVelocity = 0.91;
    invalidPlan.clamped = true;
    ledger.observe(invalidIntent, invalidPlan);

    const auto summary = ledger.summary();
    check(summary.enabled && summary.eventCount == 3,
          "enabled ledger counts every observed plan");
    check(summary.lateEventCount == 1 && summary.earlyEventCount == 1
              && summary.onGridEventCount == 1,
          "signed timing is classified without losing the grid event");
    check(summary.minimumTimingSamples == -4 && summary.maximumTimingSamples == 19,
          "timing extrema preserve contributor sum");
    check(std::fabs(summary.meanTimingSamples - 5.0) < 1.0e-12,
          "mean signed timing is exact");
    check(std::fabs(summary.meanAbsoluteTimingSamples - (23.0 / 3.0)) < 1.0e-12,
          "mean absolute timing is exact");
    check(summary.ghostEventCount == 1 && summary.ratchetEventCount == 1,
          "microevent identities remain independent");
    check(summary.clampedEventCount == 1 && summary.invalidValueCount == 1,
          "clamps and non-finite repairs are auditable");
    check(summary.minimumVelocity == 0.24 && summary.maximumVelocity == 0.91,
          "performed velocity range is retained");
    check(summary.minimumPitchMidi == 73.0 && summary.maximumPitchMidi == 73.0,
          "invalid performed pitch falls back to valid intent pitch");
    check(summary.sourceCounts[static_cast<std::size_t>(ExpressiveEventSource::Snare)] == 1
              && summary.sourceCounts[static_cast<std::size_t>(ExpressiveEventSource::Hihat)] == 1
              && summary.sourceCounts[static_cast<std::size_t>(ExpressiveEventSource::FmFlute)] == 1,
          "source provenance remains distinct");
    check(summary.domainCounts[static_cast<std::size_t>(ExpressiveDomain::Timing)] == 3,
          "domain counts report actual event reach");
    check(summary.causeCounts[static_cast<std::size_t>(ExpressiveCause::Swing)] == 3,
          "cause counts preserve the declared contributor");
    check((summary.unusedDomains & expressiveBit(ExpressiveDomain::Space)) != 0,
          "available but unused domains remain visible");
    check((summary.unusedDomains & expressiveBit(ExpressiveDomain::Timing)) == 0,
          "manifested domains are not reported as unused");

    // The compact expressiveTemporal JSON uses these arrays in enum order.
    // Keep the order observable here so metadata consumers cannot silently
    // read a source count as a different source after a vocabulary extension.
    check(summary.sourceCounts.size() == static_cast<std::size_t>(ExpressiveEventSource::Count)
              && summary.scaleCounts.size() == static_cast<std::size_t>(ExpressiveScale::Count)
              && summary.domainCounts.size() == static_cast<std::size_t>(ExpressiveDomain::Count)
              && summary.causeCounts.size() == static_cast<std::size_t>(ExpressiveCause::Count),
          "compact metadata vectors match the expressive vocabulary sizes");
    check(summary.sourceCounts[static_cast<std::size_t>(ExpressiveEventSource::Snare)] == 1
              && summary.sourceCounts[static_cast<std::size_t>(ExpressiveEventSource::Hihat)] == 1
              && summary.sourceCounts[static_cast<std::size_t>(ExpressiveEventSource::FmFlute)] == 1,
          "compact source vector preserves enum-indexed provenance");
    check(summary.domainCounts[static_cast<std::size_t>(ExpressiveDomain::Material)] == 3
              && summary.causeCounts[static_cast<std::size_t>(ExpressiveCause::MetricAccent)] == 3,
          "compact domain and cause vectors preserve declared counts");

    auto structuralIntent = drumIntent();
    structuralIntent.source = ExpressiveEventSource::Structural;
    structuralIntent.scale = ExpressiveScale::Form;
    structuralIntent.domains = expressiveBit(ExpressiveDomain::Structure)
        | expressiveBit(ExpressiveDomain::Timing);
    structuralIntent.causes = expressiveBit(ExpressiveCause::Rupture);
    ledger.observe(structuralIntent, drumPlan());
    const auto structuralSummary = ledger.summary();
    check(structuralSummary.eventCount == 4
              && structuralSummary.sourceCounts[static_cast<std::size_t>(ExpressiveEventSource::Structural)] == 1
              && structuralSummary.causeCounts[static_cast<std::size_t>(ExpressiveCause::Rupture)] == 1,
          "structural glitch/rupture provenance reaches compact summary");

    check(std::string(expressiveEventSourceName(ExpressiveEventSource::Cowbell)) == "cowbell",
          "source names are stable metadata vocabulary");
    check(std::string(expressiveEventSourceName(ExpressiveEventSource::Noise)) == "noise"
              && std::string(expressiveEventSourceName(ExpressiveEventSource::Space)) == "space",
          "noise and space sources are stable metadata vocabulary");
    check(std::string(expressiveScaleName(ExpressiveScale::History)) == "history",
          "scale names are stable metadata vocabulary");
    check(std::string(expressiveDomainName(ExpressiveDomain::Gesture)) == "gesture",
          "domain names are stable metadata vocabulary");
    check(std::string(expressiveCauseName(ExpressiveCause::Rupture)) == "rupture",
          "cause names are stable metadata vocabulary");

    std::cout << "ExpressiveEventLedger tests passed\n";
    return 0;
}
