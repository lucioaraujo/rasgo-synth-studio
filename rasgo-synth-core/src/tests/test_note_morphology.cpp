#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../engine/NoteMorphology.hpp"

namespace {
int failures = 0;
void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}
}

int main() {
    using namespace rasgo;

    const NotePrescription prescription{72.0, 1.8, 0.72, 0.65, false};
    const NotePerformanceContext context{60.0, 0.62, 0.78, 0.68};
    PerformerState expressive;
    expressive.energy = 0.76;
    expressive.effort = 0.74;
    expressive.confidence = 0.66;
    expressive.precision = 0.72;
    expressive.tension = 0.58;
    expressive.fatigue = 0.24;
    expressive.risk = 0.67;
    expressive.expressivity = 0.82;
    expressive.attention = 0.78;

    const auto intention = NoteMorphology::intend(prescription, context, expressive);
    const auto plan = NoteMorphology::plan(prescription, context, expressive, intention);
    check(intention.valid(), "note intention is bounded and valid");
    check(plan.valid(), "note morphology plan is bounded and valid");
    check(plan.peakIntensity >= plan.initialIntensity,
          "internal dynamic shape can grow beyond its onset intensity");
    check(plan.vibratoDepthCents > 0.0 && plan.vibratoDelaySeconds > 0.0,
          "long expressive note receives delayed nonzero vibrato");
    check(plan.pitchApproach != PitchApproach::Direct,
          "difficult octave can acquire pitch through a trajectory");
    check(plan.approachCents != 0.0 || plan.overshootCents != 0.0,
          "non-direct acquisition carries a signed pitch trajectory");

    const auto nextIntention = NoteMorphology::intend(
        NotePrescription{76.0, 0.8, 0.64, 0.72, false},
        NotePerformanceContext{72.0, 0.74, 0.68, 0.42}, expressive);
    const auto nextPlan = NoteMorphology::plan(
        NotePrescription{76.0, 0.8, 0.64, 0.72, false},
        NotePerformanceContext{72.0, 0.74, 0.68, 0.42}, expressive, nextIntention);
    const auto edge = NoteMorphology::transition(plan, nextPlan, 4.0, 0.74);
    check(edge.valid(), "transition plan remains bounded");
    check(edge.pitchContinuity > 0.0 && edge.timbralContinuity > 0.0,
          "transition carries tonal and timbral continuity");

    PerformerState tired = expressive;
    tired.confidence = 0.18;
    tired.precision = 0.22;
    tired.fatigue = 0.91;
    tired.attention = 0.20;
    const auto tiredIntent = NoteMorphology::intend(prescription, context, tired);
    const auto tiredPlan = NoteMorphology::plan(prescription, context, tired, tiredIntent);
    check(tiredPlan.valid(), "fragile morphology remains bounded");
    check(tiredPlan.preparation > plan.preparation,
          "fatigue and low confidence increase preparation");
    check(tiredPlan.onsetCertainty < plan.onsetCertainty,
          "fatigue and low confidence reduce onset certainty");
    check(tiredPlan.failureLikelihood > plan.failureLikelihood,
          "failure likelihood follows correlated performer state");
    check(tiredPlan.reactionLatencySeconds > plan.reactionLatencySeconds,
          "low attention and fatigue delay recovery reaction");

    const NotePrescription ending{67.0, 2.4, 0.5, 1.0, true};
    const NotePerformanceContext endingContext{69.0, 0.8, 0.35, 0.2};
    const auto endingIntent = NoteMorphology::intend(ending, endingContext, tired);
    const auto endingPlan = NoteMorphology::plan(ending, endingContext, tired, endingIntent);
    check(endingPlan.finalIntensity < endingPlan.peakIntensity,
          "phrase ending carries an internal decrescendo");
    check(endingPlan.releaseGesture == NoteReleaseGesture::Breath,
          "tired phrase ending selects a breath release gesture");

    NotePrescription invalid;
    invalid.targetMidi = std::nan("");
    invalid.durationSeconds = -4.0;
    invalid.dynamic = 12.0;
    const auto safeIntent = NoteMorphology::intend(invalid, {}, {});
    const auto safePlan = NoteMorphology::plan(invalid, {}, {}, safeIntent);
    check(safeIntent.valid() && safePlan.valid(),
          "invalid score values cannot escape contract bounds");
    check(NoteMorphology::transition({}, {}, 999.0, 8.0).valid(),
          "invalid transition inputs are bounded");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "NoteMorphology tests passed\n";
    return EXIT_SUCCESS;
}
