#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../engine/OrganismAudibility.hpp"

namespace {

int failures = 0;

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

rasgo::GamePlan game() {
    rasgo::GamePlan out;
    out.strategy = rasgo::GameStrategy::Negotiate;
    out.rule = rasgo::GameRule::Exchange;
    out.energy = 0.72;
    out.risk = 0.64;
    out.freedom = 0.81;
    out.hiddenInformation = 0.37;
    return out;
}

rasgo::OrganismStemActiveSet activeSet() {
    rasgo::OrganismStemActiveSet active{};
    active[static_cast<std::size_t>(rasgo::OrganismStem::Core)] = true;
    active[static_cast<std::size_t>(rasgo::OrganismStem::Theremin)] = true;
    active[static_cast<std::size_t>(rasgo::OrganismStem::Drums)] = true;
    active[static_cast<std::size_t>(rasgo::OrganismStem::Sieve)] = true;
    return active;
}

} // namespace

int main() {
    using namespace rasgo;

    {
        const auto identity = makeCompositionIdentity(122u);
        OrganismAudibility off(
            122u, false, activeSet(), identity, game(), 48000.0);
        off.enterSection(0.9, AgencyFamily::Polyphony, AgencyFamily::Rhythm);
        for (int i = 0; i < 2000; ++i) off.tick();
        for (int stem = 0; stem < static_cast<int>(OrganismStem::Count); ++stem) {
            check(std::fabs(off.gain(static_cast<OrganismStem>(stem)) - 1.0) < 1.0e-12,
                  "disabled ecology is exactly transparent");
        }
        check(off.movementCount() == 0, "disabled ecology invents no movement");
    }

    {
        const auto identity = makeCompositionIdentity(122u);
        OrganismAudibility a(
            122u, true, activeSet(), identity, game(), 48000.0);
        OrganismAudibility b(
            122u, true, activeSet(), identity, game(), 48000.0);
        check(a.gain(OrganismStem::Sieve) < a.gain(OrganismStem::Drums),
              "measured Sieve/drum scale disparity is calibrated before movement");
        check(a.gain(OrganismStem::Theremin) > 3.0,
              "quiet theremin desk retains meaningful emergence headroom");
        check(std::fabs(a.gain(OrganismStem::Choir) - 1.0) < 1.0e-12,
              "inactive organism remains neutral rather than being fabricated");

        for (int section = 0; section < 5; ++section) {
            const double density = 0.15 + 0.16 * section;
            a.enterSection(density, AgencyFamily::Polyphony, AgencyFamily::Rhythm);
            b.enterSection(density, AgencyFamily::Polyphony, AgencyFamily::Rhythm);
            for (int sample = 0; sample < 24000; ++sample) {
                a.tick();
                b.tick();
            }
        }
        for (int stem = 0; stem < static_cast<int>(OrganismStem::Count); ++stem) {
            const auto organism = static_cast<OrganismStem>(stem);
            check(std::fabs(a.gain(organism) - b.gain(organism)) < 1.0e-12,
                  "same seed and sections reproduce the same desk movement");
        }
        check(a.movementCount() == 20,
              "only the four active desks move across five sections");
        check(a.emergenceCount() > 0 && a.yieldCount() > 0,
              "independent section decisions admit both openings and withdrawals");
        check(a.maximumGain(OrganismStem::Theremin)
                  > a.minimumGain(OrganismStem::Theremin),
              "an active desk has a smoothed temporal range, not one album-wide gain");
        check(a.maximumGain(OrganismStem::Sieve) <= 0.82 + 1.0e-12,
              "Sieve can move without regaining its measured monopolistic scale");
    }

    {
        auto active = activeSet();
        active[static_cast<std::size_t>(OrganismStem::Tuba)] = true;
        const auto identity = makeCompositionIdentity(123u);
        check(baseOrchestralProfile(OrganismStem::Theremin, identity).role
                  != OrchestralRole::Event,
              "high-register voice has a negotiated orchestral role");
        check(baseOrchestralProfile(OrganismStem::BassFallback, identity).registerBand
                  == RegisterBand::Low
                  && baseOrchestralProfile(OrganismStem::BassFallback, identity)
                         .spectralOccupation == SpectralOccupation::Foundation,
              "fallback bass is measured as a low foundation desk");
        OrganismAudibility neutral(
            123u, true, active, identity, game(), 48000.0);
        OrganismAudibility answering(
            123u, true, active, identity, game(), 48000.0);
        OrchestralSectionReport crowded;
        crowded.highConflictCount = 3;
        crowded.highBandShare = 0.61;
        AgencyActiveSet agencyActive{};
        agencyActive.fill(true);
        MaterialAgency agency(123u, identity, game(), agencyActive, 48000.0);
        neutral.enterSection(0.55, AgencyFamily::Core, AgencyFamily::Rhythm);
        answering.enterSection(0.55, agency, crowded);
        check(answering.spectralRoleResponseCount() > 0,
              "measured high-register conflict produces a response");
        check(answering.targetGain(OrganismStem::Theremin)
                  < neutral.targetGain(OrganismStem::Theremin),
              "non-dominant air voice yields under repeated high conflict");
        check(answering.targetGain(OrganismStem::Tuba)
                  > neutral.targetGain(OrganismStem::Tuba),
              "low foundation receives a bounded answer to high conflict");
    }

    {
        const auto identity = makeCompositionIdentity(124u);
        OrganismAudibility ecology(
            124u, true, activeSet(), identity, game(), 48000.0);
        const double before = ecology.targetGain(OrganismStem::Core);
        NarrativeMixDirective directive;
        directive.valid = true;
        directive.focus = OrganismStem::Core;
        directive.reason = NarrativeAttentionReason::Presence;
        directive.gainDb = 1.0;
        ecology.applyNarrativeDirective(directive);
        check(ecology.targetGain(OrganismStem::Core) > before,
              "bounded narrative directive enters the existing desk target");
        check(ecology.narrativeDirectiveCount() == 1
                  && ecology.narrativeMaximumGainDb() == 1.0,
              "narrative actuation telemetry remains explicit");
        const double inactiveBefore = ecology.targetGain(OrganismStem::Choir);
        directive.focus = OrganismStem::Choir;
        ecology.applyNarrativeDirective(directive);
        check(ecology.targetGain(OrganismStem::Choir) == inactiveBefore
                  && ecology.narrativeDirectiveCount() == 1,
              "inactive desks cannot be fabricated by narrative attention");
    }

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "OrganismAudibility tests passed\n";
    return EXIT_SUCCESS;
}
