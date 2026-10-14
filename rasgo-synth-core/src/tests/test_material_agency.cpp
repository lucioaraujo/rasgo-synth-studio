#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

#include "engine/MaterialAgency.hpp"

using namespace rasgo;

#ifdef NDEBUG
#undef assert
#define assert(expression) ((expression) ? static_cast<void>(0) : std::abort())
#endif

int main() {
    AgencyActiveSet all{};
    all.fill(true);

    CompositionIdentity identity = makeCompositionIdentity(57);
    GamePlan game = makeGamePlan(57, identity, 0.25, 0.25, 0.25, 0.25);
    game.strategy = GameStrategy::Transform;
    MaterialAgency agency(57, identity, game, all, 48000.0);

    // No sound has yet been observed: claiming a manifested foreground here
    // would repeat the old confusion between availability and life.
    assert(!agency.hasForeground());
    assert(std::string(agency.foregroundLabel()) == "none");
    assert(agency.claimantCount() == 0);

    auto observation = [](int section, unsigned salt = 0U) {
        AgencyObservation out;
        out.frames = 48000;
        for (std::size_t i = 0; i < out.rms.size(); ++i) {
            const double motion = 0.65 + 0.35 * std::sin(
                static_cast<double>((section + 1) * static_cast<int>(i + 2)
                                    + static_cast<int>(salt % 17U)));
            out.rms[i] = 0.008 + 0.045 * motion;
            out.audibleRatio[i] = 0.08 + 0.62 * motion;
        }
        return out;
    };

    MaterialAgency repeat(57, identity, game, all, 48000.0);
    for (int section = 1; section < 18; ++section) {
        const auto heard = observation(section);
        agency.enterSection(heard);
        repeat.enterSection(heard);
        for (int sample = 0; sample < 4800; ++sample) agency.tick();
        for (int family = 0; family < static_cast<int>(AgencyFamily::Count); ++family) {
            const auto material = static_cast<AgencyFamily>(family);
            const double gain = agency.gain(material);
            assert(std::isfinite(gain));
            assert(gain >= 0.0 && gain <= 4.0);
            assert(repeat.disposition(material) == agency.disposition(material));
            assert(std::fabs(repeat.targetGain(material)
                             - agency.targetGain(material)) < 1.0e-12);
        }
        assert(std::string(repeat.foregroundLabel()) == agency.foregroundLabel());
        assert(repeat.claimantCount() == agency.claimantCount());
    }
    assert(agency.observedSectionCount() == 17);
    assert(agency.relationalReactionCount() > 0);
    assert(agency.foregroundHistory() == repeat.foregroundHistory());
    assert(agency.claimantHistory() == repeat.claimantHistory());

    MaterialAgency disabled(57, identity, game, all, 48000.0, false);
    assert(!disabled.enabled());
    assert(!disabled.hasForeground());
    for (int family = 0; family < static_cast<int>(AgencyFamily::Count); ++family)
        assert(disabled.gain(static_cast<AgencyFamily>(family)) == 1.0);
    disabled.enterSection(observation(2));
    assert(disabled.observedSectionCount() == 0);
    assert(!disabled.hasForeground());

    AgencyActiveSet sparse{};
    sparse[static_cast<std::size_t>(AgencyFamily::Core)] = true;
    sparse[static_cast<std::size_t>(AgencyFamily::Voice)] = true;
    game.strategy = GameStrategy::Counter;
    MaterialAgency sparseAgency(91, identity, game, sparse, 48000.0);
    for (int section = 0; section < 8; ++section) {
        assert(sparseAgency.targetGain(AgencyFamily::Texture) == 0.0);
        assert(sparseAgency.disposition(AgencyFamily::Texture)
               == AgencyDisposition::Dormant);
        for (const auto claimant : sparseAgency.claimantHistory().back())
            assert(sparse[static_cast<std::size_t>(claimant)]);
        sparseAgency.enterSection(observation(section, 91U));
    }

    // The vocabulary is validated across a population, never imposed as the
    // same miniature drama on each seed.
    bool populationHasLeaderlessObservedSection = false;
    bool populationHasPluralSection = false;
    bool populationHasPersistentClaim = false;
    bool populationHasSingleClaim = false;
    std::set<std::string> temporalSignatures;
    for (unsigned seed = 1; seed <= 768; ++seed) {
        auto candidateIdentity = makeCompositionIdentity(seed);
        auto candidateGame = makeGamePlan(
            seed, candidateIdentity, 0.25, 0.25, 0.25, 0.25);
        MaterialAgency candidate(
            seed, candidateIdentity, candidateGame, all, 48000.0);
        std::string signature = candidate.foregroundLabel();
        std::string previous = candidate.foregroundLabel();
        for (int section = 1; section < 11; ++section) {
            candidate.enterSection(observation(section, seed));
            signature += "/";
            signature += candidate.foregroundLabel();
            if (section > 1 && candidate.hasForeground()
                && previous == candidate.foregroundLabel())
                populationHasPersistentClaim = true;
            previous = candidate.foregroundLabel();
            if (candidate.claimantCount() == 0)
                populationHasLeaderlessObservedSection = true;
            if (candidate.claimantCount() == 1) populationHasSingleClaim = true;
            if (candidate.claimantCount() > 1) populationHasPluralSection = true;
        }
        temporalSignatures.insert(signature);
    }
    assert(populationHasLeaderlessObservedSection);
    assert(populationHasSingleClaim);
    assert(populationHasPluralSection);
    assert(populationHasPersistentClaim);
    assert(temporalSignatures.size() > 500);

    // A rare family can remain active at its event gain without being
    // eligible to reserve a whole section. Interrupt must respect that same
    // foreground set instead of reopening a side door to silent rupture.
    AgencyActiveSet sustained = all;
    sustained[static_cast<std::size_t>(AgencyFamily::Rupture)] = false;
    game.strategy = GameStrategy::Interrupt;
    MaterialAgency eventShaped(
        555, identity, game, all, sustained, 48000.0);
    for (int section = 0; section < 21; ++section) {
        assert(eventShaped.disposition(AgencyFamily::Rupture)
               == AgencyDisposition::Event);
        for (const auto claimant : eventShaped.claimantHistory().back())
            assert(claimant != AgencyFamily::Rupture);
        assert(eventShaped.targetGain(AgencyFamily::Rupture) > 0.0);
        eventShaped.enterSection(observation(section, 555U));
    }

    // Frames with no family emission may alter refusal/listening but cannot be
    // promoted into a fictitious audible claim.
    AgencyObservation silence;
    silence.frames = 48000;
    MaterialAgency silentAgency(612, identity, game, all, 48000.0);
    for (int section = 0; section < 8; ++section) {
        silentAgency.enterSection(silence);
        assert(!silentAgency.hasForeground());
        assert(silentAgency.claimantCount() == 0);
    }

    AgencyContributionMeter meter;
    for (int frame = 0; frame < 100; ++frame) {
        meter.add(AgencyFamily::Texture, 0.1, -0.1);
        meter.add(AgencyFamily::Texture, 0.05, -0.05);
        if (frame < 50) meter.add(AgencyFamily::Voice, 0.02, 0.02);
        meter.finishFrame();
    }
    assert(std::fabs(meter.rms(AgencyFamily::Texture) - 0.15) < 1.0e-12);
    assert(std::fabs(meter.peak(AgencyFamily::Texture) - 0.15) < 1.0e-12);
    assert(meter.audibleRatio(AgencyFamily::Texture) == 1.0);
    assert(std::fabs(meter.audibleRatio(AgencyFamily::Voice) - 0.5) < 1.0e-12);
    assert(meter.rms(AgencyFamily::Core) == 0.0);

    const AgencyObservation measured = meter.takeSectionObservation();
    assert(measured.frames == 100);
    assert(std::fabs(measured.rms[static_cast<std::size_t>(AgencyFamily::Texture)]
                     - 0.15) < 1.0e-12);
    assert(std::fabs(measured.audibleRatio[
                         static_cast<std::size_t>(AgencyFamily::Voice)] - 0.5)
           < 1.0e-12);
    assert(meter.takeSectionObservation().frames == 0);

    std::cout << "material agency tests passed\n";
    return 0;
}
