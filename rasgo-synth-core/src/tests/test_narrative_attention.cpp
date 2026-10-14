#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../engine/NarrativeAttentionEngine.hpp"

namespace {
int failures = 0;

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

rasgo::OrganismSectionMetrics body(double rms, double peak,
                                   double audible, double low,
                                   double mid, double high) {
    rasgo::OrganismSectionMetrics metrics;
    metrics.frames = 48000;
    metrics.rms = rms;
    metrics.peak = peak;
    metrics.audibleRatio = audible;
    metrics.lowBandRms = low;
    metrics.midBandRms = mid;
    metrics.highBandRms = high;
    return metrics;
}
}

int main() {
    using namespace rasgo;
    constexpr auto index = [](OrganismStem stem) {
        return static_cast<std::size_t>(stem);
    };

    NarrativeAttentionEngine::ProfileSet profiles{};
    for (std::size_t i = 0; i < profiles.size(); ++i) {
        profiles[i].role = OrchestralRole::Support;
        profiles[i].presenceMode = PresenceMode::Continuous;
        profiles[i].spectralOccupation = SpectralOccupation::Body;
    }
    profiles[index(OrganismStem::Core)].role = OrchestralRole::Protagonist;
    profiles[index(OrganismStem::Footnote)].role = OrchestralRole::Event;
    profiles[index(OrganismStem::Footnote)].presenceMode = PresenceMode::Event;
    profiles[index(OrganismStem::Footnote)].spectralOccupation =
        SpectralOccupation::Transient;
    profiles[index(OrganismStem::FmFlute)].role = OrchestralRole::Interlocutor;

    OrganismSectionSnapshot silence{};
    NarrativeAttentionEngine silentEngine(1u);
    const auto silent = silentEngine.observe(silence, profiles, {});
    check(!silent.valid && silent.manifestedBodies == 0,
          "silence does not fabricate an attention event");

    OrganismSectionSnapshot first{};
    first[index(OrganismStem::Core)] = body(0.22, 0.32, 0.94, 0.08, 0.19, 0.04);
    NarrativeAttentionEngine engineA(42u);
    NarrativeAttentionEngine engineB(42u);
    const auto firstA = engineA.observe(first, profiles, {0.35, 0.20, 0.40});
    const auto firstB = engineB.observe(first, profiles, {0.35, 0.20, 0.40});
    check(firstA.valid && firstA.focus == OrganismStem::Core,
          "a sole sustained body remains eligible for attention");
    check(firstA.focus == firstB.focus && firstA.reason == firstB.reason
              && std::fabs(firstA.value - firstB.value) < 1.0e-12,
          "same seed and observations produce deterministic interpretation");

    OrganismSectionSnapshot emergence = first;
    emergence[index(OrganismStem::Footnote)] =
        body(0.055, 0.62, 0.08, 0.005, 0.014, 0.05);
    const auto emergedA = engineA.observe(
        emergence, profiles, {0.52, 0.78, 0.45});
    const auto emergedB = engineB.observe(
        emergence, profiles, {0.52, 0.78, 0.45});
    check(emergedA.focus == OrganismStem::Footnote,
          "a rare transient can outrank a louder continuous bed");
    check(emergedA.reason == NarrativeAttentionReason::Emergence
              || emergedA.reason == NarrativeAttentionReason::Gesture,
          "rare transient is valued for event qualities rather than loudness alone");
    check(emergedA.focus == emergedB.focus
              && std::fabs(emergedA.value - emergedB.value) < 1.0e-12,
          "stateful attention remains deterministic across sections");

    OrganismSectionSnapshot consequence = first;
    consequence[index(OrganismStem::FmFlute)] =
        body(0.075, 0.16, 0.42, 0.004, 0.025, 0.07);
    const auto causal = engineA.observe(
        consequence, profiles,
        {0.42, 0.50, 0.90, true, OrganismStem::FmFlute});
    check(causal.valid && causal.focus == OrganismStem::FmFlute,
          "a manifested causal target can be valued as consequence");
    check(causal.consequence > 0.0,
          "causal valuation remains explicit in the decision");

    OrganismSectionSnapshot absentTarget = first;
    const auto absent = engineA.observe(
        absentTarget, profiles,
        {0.42, 0.50, 1.0, true, OrganismStem::FmFlute});
    check(absent.focus != OrganismStem::FmFlute,
          "consequence cannot fabricate an absent target");

    NarrativeAttentionEngine persistent(7u);
    auto persistentDecision = persistent.observe(first, profiles, {});
    for (int section = 0; section < 8; ++section)
        persistentDecision = persistent.observe(first, profiles, {});
    check(persistentDecision.focus == OrganismStem::Core,
          "fatigue does not force rotation when only one body manifests");
    check(persistentDecision.finite() && persistentDecision.value >= 0.0
              && persistentDecision.value <= 1.0,
          "attention values remain finite and bounded");

    NarrativeAttentionEngine disabled(3u, false);
    check(!disabled.observe(first, profiles, {}).valid
              && disabled.observedSections() == 0,
          "disabled attention is an exactly neutral observer");

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "NarrativeAttentionEngine tests passed\n";
    return EXIT_SUCCESS;
}
