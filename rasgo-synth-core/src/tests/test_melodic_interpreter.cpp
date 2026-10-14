#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../engine/MelodicInterpreter.hpp"

namespace {
void require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::abort();
    }
}
}

int main() {
    using namespace rasgo;
    MelodicEvent event;
    event.midi = 67.0;
    event.duration = 0.6;
    event.density = 0.25;
    event.accent = 0.8;
    event.phrasePosition = 0.5;

    MelodicInterpreter a(41u);
    MelodicInterpreter b(41u);
    const auto first = a.interpret(event);
    const auto repeat = b.interpret(event);
    require(first.gesture == repeat.gesture, "same seed gives same gesture");
    require(std::fabs(first.arrivalMidi - repeat.arrivalMidi) < 1.0e-12,
            "same seed gives same arrival");
    require(first.arrivalMidi != first.targetMidi,
            "interpretation can create an arrival gesture");
    require(first.livingMidi >= std::min(first.targetMidi, first.arrivalMidi)
                && first.livingMidi <= std::max(first.targetMidi, first.arrivalMidi),
            "living pitch stays between target and arrival");
    require(first.connection >= 0.0 && first.connection <= 1.0
                && first.attack >= 0.0 && first.attack <= 1.0
                && first.sustain >= 0.0 && first.sustain <= 1.0
                && first.release >= 0.0 && first.release <= 1.0,
            "articulation contract remains bounded");
    require(first.vibratoDepth > 0.0 && first.timbreBrightness > 0.0,
            "interpretation exposes expressive destinations");
    require(first.morphology.valid(),
            "interpretation exposes a bounded note morphology plan");

    MelodicInterpreter sparse(7u);
    MelodicEvent sparseEvent = event;
    sparseEvent.density = 0.0;
    sparseEvent.phraseBoundary = true;
    const auto sparseResult = sparse.interpret(sparseEvent);
    require(sparseResult.restAfter, "phrase boundary can produce a rest proposal");

    MelodicInterpreter tired(9u);
    MelodicEvent shortEvent = event;
    shortEvent.duration = 0.05;
    shortEvent.accent = 0.0;
    for (int i = 0; i < 12; ++i) tired.interpret(shortEvent);
    require(tired.breath() < 1.0, "repeated events consume breath");
    require(std::isfinite(tired.breath()), "breath remains finite");

    MelodicInterpreter memory(13u);
    auto memoryFirst = memory.interpret(event);
    auto memorySecond = memory.interpret(event);
    require(std::isfinite(memoryFirst.gestureMemory)
                && std::isfinite(memorySecond.gestureMemory)
                && memoryFirst.gestureMemory >= 0.0 && memoryFirst.gestureMemory <= 1.0
                && memorySecond.gestureMemory >= 0.0 && memorySecond.gestureMemory <= 1.0,
            "gesture memory remains bounded and finite");
    require(memorySecond.morphology.valid(),
            "note morphology remains valid across interpreter memory");

    MelodicInterpreter feedback(17u);
    const double confidenceBefore = feedback.performerState().confidence;
    feedback.observeRealization({0.0, 0.0, 0.0, 0.4, 0.8, 0.25, false, false});
    const double confidenceAfterFailure = feedback.performerState().confidence;
    feedback.observeRealization({0.0, 0.4, 0.0, 0.4, 0.2, 1.0, true, true});
    require(confidenceAfterFailure < confidenceBefore,
            "realized low integrity lowers later confidence");
    require(feedback.performerState().valid(),
            "realization feedback remains bounded after recovery");

    std::cout << "MelodicInterpreter tests passed\n";
    return 0;
}
