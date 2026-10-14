#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../engine/NarrativeMixDirective.hpp"

namespace {
int failures = 0;

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

rasgo::OrganismSectionSnapshot snapshot() {
    rasgo::OrganismSectionSnapshot section{};
    auto &core = section[static_cast<std::size_t>(rasgo::OrganismStem::Core)];
    core.frames = 48000;
    core.rms = 0.18;
    core.peak = 0.30;
    core.audibleRatio = 0.82;
    auto &tuba = section[static_cast<std::size_t>(rasgo::OrganismStem::Tuba)];
    tuba.frames = 48000;
    tuba.rms = 0.12;
    tuba.peak = 0.20;
    tuba.audibleRatio = 0.70;
    auto &footnote = section[static_cast<std::size_t>(rasgo::OrganismStem::Footnote)];
    footnote.frames = 48000;
    footnote.rms = 0.04;
    footnote.peak = 0.56;
    footnote.audibleRatio = 0.14;
    return section;
}

rasgo::NarrativeAttentionEngine::ProfileSet profiles() {
    rasgo::NarrativeAttentionEngine::ProfileSet result{};
    for (auto &profile : result) profile.role = rasgo::OrchestralRole::Support;
    result[static_cast<std::size_t>(rasgo::OrganismStem::Footnote)].role =
        rasgo::OrchestralRole::Event;
    return result;
}

rasgo::NarrativeAttentionDecision attention(rasgo::OrganismStem focus,
                                            double value,
                                            rasgo::NarrativeAttentionReason reason) {
    rasgo::NarrativeAttentionDecision result;
    result.valid = true;
    result.focus = focus;
    result.value = value;
    result.reason = reason;
    result.presence = value;
    return result;
}
}

int main() {
    using namespace rasgo;
    const auto section = snapshot();
    const auto orchestralProfiles = profiles();

    NarrativeMixDirector disabled(false);
    const auto off = disabled.decide(
        attention(OrganismStem::Core, 0.8, NarrativeAttentionReason::Presence),
        section, orchestralProfiles);
    check(!off.valid, "disabled director is exactly neutral");

    NarrativeMixDirector director;
    OrganismSectionSnapshot absence{};
    const auto absent = director.decide(
        attention(OrganismStem::Core, 0.9, NarrativeAttentionReason::Presence),
        absence, orchestralProfiles);
    check(!absent.valid, "an absent body cannot receive a directive");

    const auto first = director.decide(
        attention(OrganismStem::Core, 0.66, NarrativeAttentionReason::Presence),
        section, orchestralProfiles);
    check(first.valid && first.focus == OrganismStem::Core,
          "manifested focus receives a directive");
    check(first.finite() && first.gainDb > 0.0 && first.gainDb <= 1.25,
          "directive gain is finite and bounded");

    const auto held = director.decide(
        attention(OrganismStem::Tuba, 0.71, NarrativeAttentionReason::Presence),
        section, orchestralProfiles);
    check(held.valid && held.focus == OrganismStem::Core && held.heldByHysteresis,
          "small non-event focus change is held only while incumbent manifests");

    const auto switched = director.decide(
        attention(OrganismStem::Tuba, 0.85, NarrativeAttentionReason::Presence),
        section, orchestralProfiles);
    check(switched.valid && switched.focus == OrganismStem::Tuba
              && !switched.heldByHysteresis,
          "materially stronger evidence can replace the focus");

    NarrativeMixDirector eventDirector;
    const auto event = eventDirector.decide(
        attention(OrganismStem::Footnote, 0.70, NarrativeAttentionReason::Gesture),
        section, orchestralProfiles);
    check(event.valid && event.reason == NarrativeAttentionReason::Gesture
              && event.gainDb > first.gainDb,
          "a manifested event is valued above a comparable continuous presence");
    check(eventDirector.decisionCount() == 1
              && eventDirector.eventDirectiveCount() == 1,
          "event directive telemetry remains explicit");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "NarrativeMixDirective tests passed\n";
    return EXIT_SUCCESS;
}
