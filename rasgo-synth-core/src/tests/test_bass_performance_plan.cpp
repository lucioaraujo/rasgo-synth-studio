#include <cstdlib>
#include <iostream>

#include "../engine/BassPerformancePlan.hpp"

namespace {
int failures = 0;

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

rasgo::BassPhrasePlan makePhrase(std::uint32_t seed = 179) {
    rasgo::BassFunctionRequest function;
    function.seed = seed;
    function.rootMidiNote = 36;
    function.nextRootMidiNote = 41;
    function.harmonicMotion = 1.0;
    function.mobility = 1.0;
    function.density = 0.75;
    function.risk = 0.45;
    function.mutation = 0.55;
    function.kickPattern = {true, false, false, false, true, false, false, false,
                            true, false, false, false, true, false, false, false};
    rasgo::BassPhraseRequest phrase;
    phrase.function = rasgo::BassFunctionPlanner::plan(function);
    phrase.phraseEnd = true;
    phrase.kickPattern = function.kickPattern;
    return rasgo::BassPhrasePlanner::plan(phrase);
}

bool samePlan(const rasgo::BassPerformancePlan &a,
              const rasgo::BassPerformancePlan &b) {
    if (a.sourceValid != b.sourceValid || a.role != b.role
        || a.stepCount != b.stepCount || a.activeStepCount != b.activeStepCount
        || a.grooveApplicationCount != b.grooveApplicationCount
        || a.ghostCount != b.ghostCount || a.deadCount != b.deadCount
        || a.slideCount != b.slideCount
        || a.imperfectionCount != b.imperfectionCount
        || a.averagePressure != b.averagePressure
        || a.averageWeight != b.averageWeight)
        return false;
    for (std::size_t i = 0; i < a.stepCount; ++i) {
        const auto &left = a.steps[i];
        const auto &right = b.steps[i];
        if (left.sourceIntent != right.sourceIntent
            || left.articulation != right.articulation
            || left.imperfection != right.imperfection
            || left.timingOffsetSteps != right.timingOffsetSteps
            || left.gate != right.gate
            || left.attackRenewal != right.attackRenewal
            || left.dynamicPeak != right.dynamicPeak
            || left.pressure != right.pressure
            || left.weight != right.weight
            || left.brightness != right.brightness
            || left.noise != right.noise || left.drive != right.drive)
            return false;
    }
    return true;
}

std::size_t firstActive(const rasgo::BassPerformancePlan &plan) {
    std::size_t index = 0;
    while (index < plan.stepCount && !plan.steps[index].active()) ++index;
    return index;
}
}

int main() {
    using namespace rasgo;

    BassPerformanceRequest request;
    request.phrase = makePhrase();
    const auto first = BassPerformancePlanner::plan(request);
    const auto repeat = BassPerformancePlanner::plan(request);
    check(first.valid(), "bass performance plan is valid");
    check(samePlan(first, repeat), "same phrase, groove and state are deterministic");
    check(first.realizationFeedbackRequired,
          "active performance explicitly requests realization feedback");
    check(first.grooveApplicationCount == 0,
          "neutral groove is an exact no-op");

    BassPerformanceRequest grooved = request;
    grooved.groove.enabled = true;
    grooved.groove.strength = 1.0;
    grooved.groove.swing = 0.62;
    grooved.groove.microtiming = 0.44;
    grooved.groove.humanization = 0.28;
    grooved.groove.accent = 0.52;
    grooved.groove.gate = 0.36;
    grooved.groove.memory = 0.70;
    for (std::size_t i = 0; i < grooved.phrase.stepCount; ++i) {
        grooved.groove.steps[i].timingOffsetSteps = i % 2u == 1u ? 0.08 : 0.0;
        grooved.groove.steps[i].accentDelta = i % 4u == 0u ? 0.5 : -0.1;
        grooved.groove.steps[i].gateScale = i % 2u == 0u ? 1.12 : 0.76;
        grooved.groove.steps[i].attackDelta = i % 4u == 0u ? 0.4 : 0.0;
    }
    const auto groovePlan = BassPerformancePlanner::plan(grooved);
    check(groovePlan.valid() && groovePlan.grooveApplicationCount > 0,
          "global groove imprint acts on active bass events");
    bool grooveChangedEvent = false;
    for (std::size_t i = 0; i < first.stepCount; ++i) {
        grooveChangedEvent = grooveChangedEvent
            || (first.steps[i].active()
                && (first.steps[i].timingOffsetSteps
                        != groovePlan.steps[i].timingOffsetSteps
                    || first.steps[i].gate != groovePlan.steps[i].gate
                    || first.steps[i].attackRenewal
                        != groovePlan.steps[i].attackRenewal));
    }
    check(grooveChangedEvent,
          "groove changes timing, gate or articulation rather than metadata only");

    BassPerformanceRequest low = request;
    low.performer.energy = 0.1;
    low.performer.effort = 0.1;
    low.performer.tension = 0.1;
    low.performer.risk = 0.1;
    BassPerformanceRequest high = request;
    high.performer.energy = 0.95;
    high.performer.effort = 0.95;
    high.performer.tension = 0.95;
    high.performer.risk = 0.80;
    const auto lowPlan = BassPerformancePlanner::plan(low);
    const auto highPlan = BassPerformancePlanner::plan(high);
    const std::size_t index = firstActive(lowPlan);
    check(index < lowPlan.stepCount && index < highPlan.stepCount,
          "comparison contains an active event");
    check(highPlan.steps[index].pressure > lowPlan.steps[index].pressure
          && highPlan.steps[index].brightness > lowPlan.steps[index].brightness
          && highPlan.steps[index].drive > lowPlan.steps[index].drive,
          "latent effort correlates pressure, brightness and drive");

    BassPerformanceRequest feedback = request;
    feedback.hasObservedFeedback = true;
    feedback.previousObservation.realizedIntegrity = 0.15;
    feedback.previousObservation.difficulty = 0.9;
    feedback.previousObservation.requestedEffort = 0.8;
    feedback.previousObservation.density = 0.8;
    feedback.previousObservation.noteDurationSeconds = 1.0;
    const auto feedbackPlan = BassPerformancePlanner::plan(feedback);
    check(feedbackPlan.valid(), "observed feedback produces a valid next plan");
    check(feedbackPlan.performerAtPlan.confidence
          < request.performer.confidence,
          "only observed weak realization lowers performer confidence");

    bool foundControlledImperfection = false;
    for (std::uint32_t seed = 1; seed <= 512 && !foundControlledImperfection; ++seed) {
        BassPerformanceRequest fragile;
        fragile.phrase = makePhrase(seed);
        fragile.controlledImperfection = 1.0;
        fragile.performer.confidence = 0.05;
        fragile.performer.precision = 0.05;
        fragile.performer.attention = 0.10;
        fragile.performer.fatigue = 0.95;
        fragile.performer.risk = 0.95;
        const auto plan = BassPerformancePlanner::plan(fragile);
        check(plan.valid(), "fragile performance remains bounded and valid");
        foundControlledImperfection = plan.imperfectionCount > 0;
    }
    check(foundControlledImperfection,
          "controlled imperfection and recovery requests are reachable");

    BassFunctionRequest sparseFunction;
    sparseFunction.seed = 4;
    sparseFunction.density = 0.0;
    sparseFunction.space = 1.0;
    sparseFunction.narrativeTension = 0.0;
    BassPhraseRequest sparsePhrase;
    sparsePhrase.function = BassFunctionPlanner::plan(sparseFunction);
    BassPerformanceRequest silent;
    silent.phrase = BassPhrasePlanner::plan(sparsePhrase);
    const auto silentPlan = BassPerformancePlanner::plan(silent);
    check(silentPlan.valid() && silentPlan.silent()
          && !silentPlan.realizationFeedbackRequired,
          "performance layer preserves silence without fake feedback");

    BassPerformanceRequest invalidGroove = request;
    invalidGroove.groove.steps[0].timingOffsetSteps = 0.1;
    const auto rejected = BassPerformancePlanner::plan(invalidGroove);
    check(!rejected.valid() && !rejected.sourceValid,
          "disabled non-neutral groove is rejected instead of acting secretly");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "BassPerformancePlan tests passed\n";
    return EXIT_SUCCESS;
}
