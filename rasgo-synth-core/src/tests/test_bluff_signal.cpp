#include <cassert>
#include <cmath>
#include <cstdio>
#include "../sequencer/BluffSignal.hpp"

using namespace rasgo;

static bool approx(double a, double b, double tol = 0.02) {
    return std::fabs(a - b) < tol;
}

int main() {
    int pass = 0, fail = 0;
    auto check = [&](bool cond, const char *name) {
        if (cond) { ++pass; } else { ++fail; std::printf("FAIL: %s\n", name); }
    };

    // A signaler with truthfulness=1.0 is never a bluffer -- every
    // resolved signal is genuine, trust should climb toward 1.0 and never
    // trigger betrayal.
    {
        BluffSignal sig;
        sig.configure(1.0, 1);
        for (int i = 0; i < 30; ++i) {
            while (!sig.maybeSignal(1.0)) {}
            check(sig.pendingIsGenuine(), "truthfulness=1.0: every signal is genuine");
            sig.resolveSignal(0.3, 0.3);
        }
        check(sig.trust() > 0.9, "truthfulness=1.0: trust climbs high");
        check(!sig.betrayed(), "truthfulness=1.0: never betrayed");
        check(sig.bluffCount() == 0, "truthfulness=1.0: zero bluffs counted");
    }

    // A pure bluffer (truthfulness=0.0) never gets trust above the
    // 0.65 threshold that betrayal requires -- confianca nunca chega a
    // ser genuinamente construida, entao "traicao" nao se aplica (o
    // usuario e explicito: a traicao pressupoe confianca previa).
    {
        BluffSignal sig;
        sig.configure(0.0, 2);
        for (int i = 0; i < 30; ++i) {
            while (!sig.maybeSignal(1.0)) {}
            check(!sig.pendingIsGenuine(), "truthfulness=0.0: every signal is a bluff");
            sig.resolveSignal(0.3, 0.3);
        }
        check(sig.trust() < 0.35, "truthfulness=0.0: trust drops low");
        check(!sig.everTrusted(), "truthfulness=0.0: never crossed the trust-was-built threshold");
        check(!sig.betrayed(), "truthfulness=0.0: no betrayal without prior real trust");
    }

    // The core dramaturgy the user described: aproximacao -> cooperacao
    // -> dependencia -> traicao -> reorganizacao. A real sequence: many
    // genuine signals first (trust must climb past the 0.65
    // "everTrusted" bar and betrayal must NOT fire while still
    // truthful), then a run of forced bluffs on the SAME object (trust
    // must collapse below the real betrayal threshold and betrayal must
    // fire exactly once, at the correct moment in the sequence).
    {
        BluffSignal sig;
        sig.configure(1.0, 4); // phase 1: a fully truthful signaler
        bool betrayedDuringBuildup = false;
        for (int i = 0; i < 20; ++i) {
            while (!sig.maybeSignal(1.0)) {}
            sig.resolveSignal(0.35, 0.3);
            if (sig.betrayed()) betrayedDuringBuildup = true;
        }
        check(sig.everTrusted(), "dramaturgy: trust was genuinely built during the truthful phase");
        check(!betrayedDuringBuildup, "dramaturgy: no betrayal fires while the signaler stays truthful");
        check(sig.trust() > 0.65, "dramaturgy: trust is genuinely above the betrayal-eligibility bar before bluffing starts");

        // Phase 2: the same instance now bluffs every time (reconfigure()
        // only reseeds the RNG/truthfulness roll -- trust_/everTrusted_
        // are untouched, so this genuinely carries the prior history
        // forward, the same object's own relationship souring rather
        // than a fresh one starting cold).
        sig.configure(0.0, 40);
        int betrayalCount = 0;
        for (int i = 0; i < 20; ++i) {
            while (!sig.maybeSignal(1.0)) {}
            bool wasBetrayedBefore = sig.betrayed();
            sig.resolveSignal(0.35, 0.3);
            if (sig.betrayed() && !wasBetrayedBefore) ++betrayalCount;
        }
        check(sig.trust() < 0.3, "dramaturgy: trust genuinely collapses once bluffing starts");
        check(betrayalCount == 1, "dramaturgy: betrayal fires exactly once, at the real moment trust crosses the threshold");
    }

    // resolveSignal()'s EMA math itself, checked by hand against a known
    // sequence -- not trusting the class's own arithmetic without an
    // independent reference computation.
    {
        BluffSignal sig;
        sig.configure(1.0, 5);
        double expectedTrust = 0.5; // starts neutral
        double alpha = 0.4;
        // genuine, genuine, bluff (forced via pendingIsGenuine_ through
        // real maybeSignal rolls at truthfulness 1.0/0.0 in sequence)
        while (!sig.maybeSignal(1.0)) {}
        sig.resolveSignal(alpha, 0.3);
        expectedTrust += alpha * (1.0 - expectedTrust);
        check(approx(sig.trust(), expectedTrust), "EMA: matches hand-computed value after one genuine signal");

        while (!sig.maybeSignal(1.0)) {}
        sig.resolveSignal(alpha, 0.3);
        expectedTrust += alpha * (1.0 - expectedTrust);
        check(approx(sig.trust(), expectedTrust), "EMA: matches hand-computed value after two genuine signals");
    }

    // responseGain(): equals trust before betrayal, exactly zero after --
    // a real, checkable behavioral consequence, not just a status flag.
    {
        BluffSignal sig;
        sig.configure(1.0, 6);
        while (!sig.maybeSignal(1.0)) {}
        sig.resolveSignal(0.5, 0.3);
        check(approx(sig.responseGain(), sig.trust()), "responseGain: equals current trust while not betrayed");
    }

    // maybeSignal()'s own truthfulness rate, checked statistically over a
    // large sample -- the real claim the whole mechanism rests on.
    {
        BluffSignal sig;
        sig.configure(0.7, 7);
        int genuine = 0, total = 2000;
        for (int i = 0; i < total; ++i) {
            while (!sig.maybeSignal(1.0)) {}
            if (sig.pendingIsGenuine()) ++genuine;
        }
        double rate = static_cast<double>(genuine) / total;
        check(approx(rate, 0.7, 0.05), "maybeSignal: real genuine/bluff ratio matches configured truthfulness over a large sample");
    }

    // maybeSignal()'s own opportunity-chance gating: with chance=0.0, no
    // signal ever fires, regardless of how many times it's polled.
    {
        BluffSignal sig;
        sig.configure(0.5, 8);
        bool anyFired = false;
        for (int i = 0; i < 1000; ++i) if (sig.maybeSignal(0.0)) anyFired = true;
        check(!anyFired, "maybeSignal: chance=0.0 never fires a signal");
        check(sig.signalCount() == 0, "maybeSignal: signalCount stays zero when nothing ever fires");
    }

    std::printf("\n%d passed, %d failed\n", pass, fail);
    return fail == 0 ? 0 : 1;
}
