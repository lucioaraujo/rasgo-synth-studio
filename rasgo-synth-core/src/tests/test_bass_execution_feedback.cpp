#include "../engine/BassExecutionFeedback.hpp"
#include "../engine/BassBodyAdapters.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

namespace {
void check(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL " << message << '\n'; std::exit(1); }
}
void frames(rasgo::BassExecutionFeedback& feedback, double signal, int count) {
    for (int i = 0; i < count; ++i) {
        feedback.beginFrame();
        feedback.observeVoice(signal);
        feedback.finishFrame();
    }
}
rasgo::BassPerformanceRequest request(unsigned seed) {
    rasgo::BassFunctionRequest function;
    function.seed = seed;
    function.rootMidiNote = 36;
    function.nextRootMidiNote = 41;
    function.harmonicMotion = function.mobility = 1.0;
    function.density = .75; function.risk = .45;
    function.kickPattern[0] = function.kickPattern[4] = true;
    rasgo::BassPhraseRequest phrase;
    phrase.function = rasgo::BassFunctionPlanner::plan(function);
    phrase.phraseEnd = true;
    phrase.kickPattern = function.kickPattern;
    rasgo::BassPerformanceRequest out;
    out.phrase = rasgo::BassPhrasePlanner::plan(phrase);
    return out;
}
}

int main() {
    using namespace rasgo;
    PerformerObservation observation;
    BassExecutionFeedback disabled(false, 8000);
    disabled.observeTrigger(1, 36);
    frames(disabled, .1, 1000);
    check(!disabled.takeObservation(true, observation), "disabled stays neutral");
    check(disabled.triggers() == 0 && disabled.activeFrames() == 0, "disabled counters zero");
    BassExecutionFeedback playing(true, 8000), quiet(true, 8000);
    check(!playing.takeObservation(true, observation), "no synthetic first observation");
    playing.observeTrigger(.8, 36);
    playing.observeTrigger(.6, 42);
    frames(playing, .1, 8000);
    frames(quiet, 0, 8000);
    PerformerObservation work, rest;
    check(playing.takeObservation(false, work), "real signal observed");
    check(quiet.takeObservation(true, rest), "actual quiet window observed");
    check(work.noteDurationSeconds == 1 && work.restSeconds == 0 && work.density == 1,
          "sample-count duration and occupancy");
    check(std::fabs(work.requestedEffort - .7) < 1e-12 && work.difficulty == .25,
          "effort and interval from dispatched events");
    check(work.realizedIntegrity == 1 && !work.recoveredFromFailure,
          "numerical integrity does not invent failure recovery");
    check(rest.restSeconds == 1 && rest.noteDurationSeconds == 0 && rest.requestedEffort == 0,
          "quietness not falsely treated as a missed attack");
    check(!playing.takeObservation(false, observation), "window consumed exactly once");
    const PerformerState initial{};
    const auto worked = PerformerStateModel::advance(initial, work);
    const auto rested = PerformerStateModel::advance(initial, rest);
    check(worked.energy < rested.energy && worked.fatigue > rested.fatigue,
          "activity and pause have distinct state consequences");

    // Same prescription, different observed execution, all three actual adapters.
    for (unsigned seed = 1; seed <= 32; ++seed) {
        auto activeRequest = request(seed);
        activeRequest.hasObservedFeedback = true;
        activeRequest.previousObservation = work;
        auto restRequest = activeRequest;
        restRequest.previousObservation = rest;
        const auto active = BassPerformancePlanner::plan(activeRequest);
        const auto resting = BassPerformancePlanner::plan(restRequest);
        check(active.valid() && resting.valid(), "feedback plans valid");
        check(active.performerAtPlan.energy == worked.energy, "one state advance only");
        for (const auto availability : {BassBodyAvailability{true, false, true},
                BassBodyAvailability{false, true, true}, BassBodyAvailability{false, false, true}}) {
            const auto bodyA = BassBodyAdapterPlanner::plan(active, availability);
            const auto bodyB = BassBodyAdapterPlanner::plan(resting, availability);
            check(bodyA.valid() && bodyB.valid(), "body-specific destinations valid");
            bool changed = false;
            for (std::size_t i = 0; i < bodyA.stepCount; ++i)
                changed = changed || bodyA.steps[i].pressure != bodyB.steps[i].pressure
                    || bodyA.steps[i].brightness != bodyB.steps[i].brightness
                    || bodyA.steps[i].attack != bodyB.steps[i].attack;
            check(changed, "observations change applied articulation/material destinations");
        }
    }

    // Numerical contamination is accounted once per frame, not per voice.
    BassExecutionFeedback invalid(true, std::numeric_limits<double>::quiet_NaN());
    invalid.observeTrigger(std::numeric_limits<double>::infinity(), 36);
    for (int i = 0; i < 100; ++i) {
        invalid.beginFrame();
        invalid.observeVoice(std::numeric_limits<double>::quiet_NaN());
        invalid.observeVoice(std::numeric_limits<double>::infinity());
        invalid.finishFrame();
    }
    check(invalid.takeObservation(false, observation), "invalid samples bounded");
    check(observation.realizedIntegrity == 0 && observation.density == 0
        && observation.requestedEffort == 0, "invalid samples are not a phantom note");
    check(PerformerStateModel::advance(initial, observation).valid(), "state remains finite");

    // Sustained signal and finite trace capacity over a longer cohort.
    PerformerState state = initial;
    BassExecutionFeedback longRun(true, 8000), repeat(true, 8000);
    PerformerState repeatedState = initial;
    for (int cycle = 0; cycle < 512; ++cycle) {
        for (auto* observer : {&longRun, &repeat}) {
            observer->observeTrigger(.65, 36 + cycle % 7);
            frames(*observer, cycle % 3 == 0 ? 0 : .1, 800);
        }
        PerformerObservation first, second;
        check(longRun.takeObservation(cycle % 4 == 0, first), "long window exists");
        check(repeat.takeObservation(cycle % 4 == 0, second), "repeat window exists");
        state = PerformerStateModel::advance(state, first);
        repeatedState = PerformerStateModel::advance(repeatedState, second);
        check(state.valid() && repeatedState.valid(), "long state bounds");
        check(state.energy == repeatedState.energy && state.precision == repeatedState.precision,
              "same observed execution is deterministic");
        longRun.applied(state); repeat.applied(repeatedState);
    }
    std::ostringstream json, repeatedJson;
    longRun.writeJson(json); repeat.writeJson(repeatedJson);
    check(json.str() == repeatedJson.str(), "bounded trace repeatable");
    check(json.str().find("\"omittedWindows\":448") != std::string::npos,
          "only 64 windows retained with omission count");
    check(longRun.updates() == 512 && longRun.triggers() == 512, "persistent updates counted");
    std::cout << "PASS bass observed feedback: boundaries/quietness/finite/bodies/repeat/trace\n";
}
