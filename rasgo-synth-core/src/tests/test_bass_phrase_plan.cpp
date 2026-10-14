#include <cstdlib>
#include <iostream>

#include "../engine/BassPhrasePlan.hpp"

namespace {
int failures = 0;

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

bool samePlan(const rasgo::BassPhrasePlan &a,
              const rasgo::BassPhrasePlan &b) {
    if (a.sourceValid != b.sourceValid || a.role != b.role
        || a.contour != b.contour || a.boundary != b.boundary
        || a.stepCount != b.stepCount || a.activeStepCount != b.activeStepCount
        || a.sourceSignature != b.sourceSignature
        || a.realizedSignature != b.realizedSignature)
        return false;
    for (std::size_t i = 0; i < a.stepCount; ++i) {
        const auto &left = a.steps[i];
        const auto &right = b.steps[i];
        if (left.intent != right.intent
            || left.semitoneOffset != right.semitoneOffset
            || left.registerIntent != right.registerIntent
            || left.kickRelation != right.kickRelation
            || left.timingOffsetSteps != right.timingOffsetSteps
            || left.weight != right.weight || left.gate != right.gate)
            return false;
    }
    return true;
}

rasgo::BassFunctionPlan makePath(double space, double memory,
                                 std::uint32_t seed = 178) {
    rasgo::BassFunctionRequest request;
    request.seed = seed;
    request.rootMidiNote = 36;
    request.nextRootMidiNote = 41;
    request.harmonicMotion = 1.0;
    request.mobility = 1.0;
    request.risk = 0.0;
    request.density = 0.72;
    request.space = space;
    request.memory = memory;
    request.mutation = 0.8;
    request.kickPattern = {true, false, false, false, true, false, false, false,
                           true, false, false, false, true, false, false, false};
    return rasgo::BassFunctionPlanner::plan(request);
}
}

int main() {
    using namespace rasgo;

    BassPhraseRequest request;
    request.function = makePath(0.2, 0.5);
    request.phraseEnd = true;
    request.kickPattern = {true, false, false, false, true, false, false, false,
                           true, false, false, false, true, false, false, false};
    const auto first = BassPhrasePlanner::plan(request);
    const auto repeat = BassPhrasePlanner::plan(request);
    check(first.valid(), "phrase plan is valid");
    check(samePlan(first, repeat), "same function and history remain deterministic");
    check(first.role == BassFunctionRole::Path
          && first.contour == BassContour::Ascending,
          "positive harmonic path receives an ascending contour");
    check(first.boundary == BassBoundaryGesture::AnticipateNextRoot,
          "connected path anticipates the next root");
    check(first.steps[first.stepCount - 1u].semitoneOffset == 5
          && first.steps[first.stepCount - 1u].timingOffsetSteps < 0.0,
          "anticipated destination preserves relative harmonic target");

    for (std::size_t i = 0; i < first.stepCount; ++i) {
        if (!first.steps[i].active()) continue;
        check(first.steps[i].timingOffsetSteps >= -0.25
              && first.steps[i].timingOffsetSteps <= 0.25,
              "relational microtiming remains bounded");
    }

    BassPhraseRequest delayed;
    delayed.function = makePath(1.0, 1.0, 179);
    delayed.phraseEnd = true;
    delayed.kickPattern = request.kickPattern;
    const auto delayedPlan = BassPhrasePlanner::plan(delayed);
    check(delayedPlan.valid(), "delayed boundary plan is valid");
    check(delayedPlan.boundary == BassBoundaryGesture::DelayedResolution,
          "spacious remembered path can retain resolution");
    check(delayedPlan.nextHistory.pendingResolution
          && delayedPlan.nextHistory.pendingTargetMidiNote == 41,
          "delayed target is carried in fixed phrase memory");

    BassFunctionRequest arrivalFunction;
    arrivalFunction.seed = 180;
    arrivalFunction.rootMidiNote = 41;
    arrivalFunction.nextRootMidiNote = 41;
    arrivalFunction.density = 0.6;
    arrivalFunction.allowSilence = false;
    BassPhraseRequest arrival;
    arrival.function = BassFunctionPlanner::plan(arrivalFunction);
    arrival.history = delayedPlan.nextHistory;
    arrival.phraseStart = true;
    arrival.kickPattern = request.kickPattern;
    const auto arrivalPlan = BassPhrasePlanner::plan(arrival);
    check(arrivalPlan.valid(), "carried resolution produces a valid next phrase");
    std::size_t firstActive = 0;
    while (firstActive < arrivalPlan.stepCount
           && !arrivalPlan.steps[firstActive].active())
        ++firstActive;
    check(firstActive < arrivalPlan.stepCount
          && arrivalPlan.steps[firstActive].intent == BassStepIntent::Root
          && arrivalPlan.steps[firstActive].timingOffsetSteps > 0.0,
          "next phrase realizes delayed root behind its grid point");

    BassPhraseRequest evolved = request;
    evolved.phraseEnd = false;
    const auto original = BassPhrasePlanner::plan(evolved);
    evolved.history = original.nextHistory;
    const auto variation = BassPhrasePlanner::plan(evolved);
    check(variation.valid(), "repetition evolution remains valid");
    check(variation.nextHistory.repeatedSections == 1,
          "phrase memory counts repeated source material");
    check(original.sourceSignature == variation.sourceSignature
          && original.realizedSignature != variation.realizedSignature,
          "repetition evolves realization without losing source identity");

    BassFunctionRequest silenceFunction;
    silenceFunction.seed = 4;
    silenceFunction.density = 0.0;
    silenceFunction.space = 1.0;
    silenceFunction.narrativeTension = 0.0;
    BassPhraseRequest silence;
    silence.function = BassFunctionPlanner::plan(silenceFunction);
    const auto silencePlan = BassPhrasePlanner::plan(silence);
    check(silence.function.role == BassFunctionRole::Silence,
          "fixture reaches legitimate silent function");
    check(silencePlan.valid() && silencePlan.silent()
          && silencePlan.boundary == BassBoundaryGesture::None,
          "phrase layer preserves legitimate silence");

    BassPhraseRequest invalid;
    const auto invalidPlan = BassPhrasePlanner::plan(invalid);
    check(!invalidPlan.valid() && !invalidPlan.sourceValid,
          "invalid upstream function is rejected without fabrication");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "BassPhrasePlan tests passed\n";
    return EXIT_SUCCESS;
}
