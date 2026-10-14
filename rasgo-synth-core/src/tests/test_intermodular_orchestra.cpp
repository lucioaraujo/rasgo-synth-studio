#include <cmath>
#include <cstdlib>
#include <iostream>

#include "../engine/IntermodularOrchestra.hpp"

namespace {

int failures = 0;

void check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

rasgo::CompositionIdentity identityFor(unsigned seed) {
    return rasgo::makeCompositionIdentity(seed);
}

rasgo::GamePlan gameFor(rasgo::GameStrategy strategy) {
    rasgo::GamePlan game;
    game.strategy = strategy;
    game.rule = rasgo::GameRule::Exchange;
    game.energy = 0.72;
    game.memory = 0.42;
    game.risk = 0.76;
    game.freedom = 0.78;
    game.hiddenInformation = 0.18;
    return game;
}

void feedStrongBar(rasgo::IntermodularOrchestra &orchestra) {
    constexpr int samples = 48000;
    for (int i = 0; i < samples; ++i) {
        float hit = (i % 3000) < 90 ? 0.52f * std::exp(-static_cast<float>(i % 3000) / 25.0f) : 0.0f;
        orchestra.observeRhythmSample(hit);
    }
}

void feedSilentBar(rasgo::IntermodularOrchestra &orchestra) {
    // Silence metrics are invariant to bar sample count; a short synthetic
    // bar keeps the temporal state sweep fast without weakening the check.
    for (int i = 0; i < 512; ++i) orchestra.observeRhythmSample(0.0f);
}

} // namespace

int main() {
    using namespace rasgo;

    HarmonyObservation harmony;
    harmony.rootMidiNote = 57;
    harmony.chordOffsets = {0, 3, 7, 10};

    {
        auto identity = identityFor(101u);
        IntermodularOrchestra orchestra(101u, identity, gameFor(GameStrategy::Transform), 48000.0, false);
        feedStrongBar(orchestra);
        auto rhythm = orchestra.finishRhythmBar(0.5);
        auto decision = orchestra.interpretBar(rhythm, harmony, 1.0, 1);
        check(!rhythm.manifested, "disabled orchestra does not claim a rhythm manifestation");
        check(!decision.thereminResponded, "disabled orchestra is transparent");
        check(decision.harmonyShiftSemitones == 0, "disabled orchestra cannot reconfigure harmony");
        check(std::fabs(decision.rhythmVelocityScale - 1.0) < 1.0e-12,
              "disabled orchestra leaves rhythm gain neutral");
    }

    {
        auto identity = identityFor(202u);
        IntermodularOrchestra orchestra(202u, identity, gameFor(GameStrategy::Counter), 48000.0, true);
        for (int i = 0; i < 48000; ++i) orchestra.observeRhythmSample(0.0f);
        auto rhythm = orchestra.finishRhythmBar(0.0);
        auto decision = orchestra.interpretBar(rhythm, harmony, 1.0, 1);
        check(!rhythm.manifested, "silence is not mistaken for a playing rhythm module");
        check(!decision.thereminResponded, "theremin does not respond to a silent flag");
        check(orchestra.rhythmObservationCount() == 0, "silent bar creates no observation count");
    }

    {
        auto identity = identityFor(303u);
        auto game = gameFor(GameStrategy::Transform);
        IntermodularOrchestra a(303u, identity, game, 48000.0, true);
        IntermodularOrchestra b(303u, identity, game, 48000.0, true);
        feedStrongBar(a);
        feedStrongBar(b);
        auto ra = a.finishRhythmBar(0.31);
        auto rb = b.finishRhythmBar(0.31);
        auto da = a.interpretBar(ra, harmony, 1.2, 3);
        auto db = b.interpretBar(rb, harmony, 1.2, 3);
        check(ra.manifested && rb.manifested, "actual drum signal manifests in both deterministic runs");
        check(da.thereminResponded && db.thereminResponded, "strong transported gesture crosses sensitivity");
        check(std::fabs(da.thereminTargetMidi - db.thereminTargetMidi) < 1.0e-12,
              "same seed and lived input produce the same interpretation");
        check(da.harmonyDisposition == db.harmonyDisposition,
              "same seed and lived input produce the same harmonic response");
        check(da.rhythmVelocityScale == db.rhythmVelocityScale
              && da.rhythmDelaySamples == db.rhythmDelaySamples,
              "same harmonic response produces the same return to rhythm");
        check(da.thereminTargetMidi >= 40.0 && da.thereminTargetMidi <= 96.0,
              "independent theremin proposal remains in a bounded register");
        check(da.consequence, "a completed relation reports a consequence");
        check(a.thereminResponseCount() == 1 && a.harmonyResponseCount() == 1
              && a.rhythmFeedbackCount() == 1,
              "causal chain counts perception, interpretation and feedback separately");

        const double rememberedEncounter = a.memory();
        for (int bar = 4; bar < 7; ++bar) {
            feedSilentBar(a);
            auto silence = a.finishRhythmBar(0.0);
            a.interpretBar(silence, harmony, 0.0, bar);
        }
        check(a.memory() > 0.0 && a.memory() < rememberedEncounter,
              "memory survives silence and decays without commanding a response");
    }

    {
        const GameStrategy strategies[] = {
            GameStrategy::Follow, GameStrategy::Transform,
            GameStrategy::Counter, GameStrategy::Interrupt,
            GameStrategy::Conceal, GameStrategy::Negotiate
        };
        const RelationVerb expected[] = {
            RelationVerb::Follow, RelationVerb::Transform,
            RelationVerb::Counter, RelationVerb::Aggress,
            RelationVerb::Conceal, RelationVerb::Negotiate
        };
        std::array<bool, 6> heard{};
        for (std::size_t i = 0; i < heard.size(); ++i) {
            auto identity = identityFor(static_cast<unsigned>(800 + i));
            IntermodularOrchestra orchestra(
                static_cast<unsigned>(800 + i), identity,
                gameFor(strategies[i]), 48000.0, true);
            check(orchestra.verb() == expected[i],
                  "each game strategy retains its own relational verb");
            heard[static_cast<std::size_t>(orchestra.verb())] = true;
        }
        check(std::all_of(heard.begin(), heard.end(), [](bool value) { return value; }),
              "the isolated orchestra exposes all six relational verbs without forcing rare modules into pieces");
    }

    {
        bool heardResistance = false;
        bool heardReconfiguration = false;
        for (unsigned seed = 1; seed <= 96; ++seed) {
            auto identity = identityFor(seed);
            for (GameStrategy strategy : {GameStrategy::Transform, GameStrategy::Counter,
                                          GameStrategy::Interrupt, GameStrategy::Conceal}) {
                auto game = gameFor(strategy);
                const bool highResistance = (seed % 2u) == 0u;
                if (highResistance) {
                    game.memory = 0.96;
                    game.risk = 0.04;
                    game.freedom = 0.22;
                }
                IntermodularOrchestra orchestra(seed, identity, game, 48000.0, true);
                feedStrongBar(orchestra);
                auto rhythm = orchestra.finishRhythmBar(0.8);
                auto decision = orchestra.interpretBar(
                    rhythm, harmony, highResistance ? 0.88 : 1.35, 2);
                heardResistance = heardResistance
                    || decision.harmonyDisposition == HarmonyDisposition::Resist;
                heardReconfiguration = heardReconfiguration
                    || decision.harmonyDisposition == HarmonyDisposition::Reconfigure;
                check(std::isfinite(orchestra.memory()) && std::isfinite(orchestra.resistance()),
                      "relational state remains finite");
                check(orchestra.resistance() >= 0.04 && orchestra.resistance() <= 0.96,
                      "resistance remains inside its explicit contract");
                check(decision.rhythmVelocityScale >= 0.65 && decision.rhythmVelocityScale <= 1.38,
                      "feedback cannot erase or explode the drum bus");
                check(decision.rhythmDelaySamples >= 0 && decision.rhythmDelaySamples <= 1000,
                      "feedback delay is bounded and never schedules impossible negative time");
            }
        }
        check(heardResistance, "the orchestra can genuinely resist an aggression/proposal");
        check(heardReconfiguration, "the orchestra can genuinely let a proposal alter harmony");
    }

    {
        bool foundInitiallyLatent = false;
        bool foundInitiallySustained = false;
        bool foundContinuousTrajectory = false;
        bool foundSilentTrajectory = false;
        bool foundNoMandatoryTransition = false;
        bool foundEntry = false;
        bool foundWithdrawal = false;
        const GameStrategy temporalStrategies[] = {
            GameStrategy::Follow, GameStrategy::Transform,
            GameStrategy::Counter, GameStrategy::Interrupt,
            GameStrategy::Conceal, GameStrategy::Negotiate
        };
        for (unsigned seed = 1; seed <= 192; ++seed) {
            auto identity = identityFor(seed);
            IntermodularOrchestra orchestra(
                seed, identity,
                gameFor(temporalStrategies[(seed - 1u) % 6u]), 48000.0, true);
            foundInitiallyLatent = foundInitiallyLatent
                || orchestra.currentThereminPresence() < 0.075;
            foundInitiallySustained = foundInitiallySustained
                || orchestra.currentThereminPresence() > 0.65;
            for (int bar = 0; bar < 24; ++bar) {
                if (bar == 2 || bar == 7) feedStrongBar(orchestra);
                else feedSilentBar(orchestra);
                auto rhythm = orchestra.finishRhythmBar(bar == 2 ? 0.48 : 0.0);
                orchestra.interpretBar(rhythm, harmony, rhythm.manifested ? 1.15 : 0.0, bar);
                check(orchestra.currentThereminPresence() >= 0.0
                      && orchestra.currentThereminPresence() <= 1.0,
                      "temporal presence remains a bounded orchestral degree");
            }
            foundContinuousTrajectory = foundContinuousTrajectory
                || orchestra.thereminSilentBarCount() == 0;
            foundSilentTrajectory = foundSilentTrajectory
                || orchestra.thereminSilentBarCount() > 0;
            foundNoMandatoryTransition = foundNoMandatoryTransition
                || (orchestra.thereminEntryCount() == 0
                    && orchestra.thereminWithdrawalCount() == 0);
            foundEntry = foundEntry || orchestra.thereminEntryCount() > 0;
            foundWithdrawal = foundWithdrawal
                || orchestra.thereminWithdrawalCount() > 0;
        }
        check(foundInitiallyLatent && foundInitiallySustained,
              "different identities permit both latency and sustained initial presence");
        check(foundContinuousTrajectory && foundSilentTrajectory,
              "the seed set permits continuity and silence without requiring either in every piece");
        check(foundNoMandatoryTransition,
              "orchestration does not force every seed through an entrance/withdrawal script");
        check(foundEntry,
              "entrance remains a reachable possibility across identities");
        check(foundWithdrawal,
              "withdrawal remains a reachable possibility across identities");
    }

    if (failures != 0) return EXIT_FAILURE;
    std::cout << "IntermodularOrchestra tests passed\n";
    return EXIT_SUCCESS;
}
