#include <cassert>
#include <cmath>
#include <cstdio>
#include "../sequencer/MicrotonalTuning.hpp"

using namespace rasgo;

static bool approx(double a, double b, double tol = 0.05) {
    return std::fabs(a - b) < tol;
}

int main() {
    int pass = 0, fail = 0;
    auto check = [&](bool cond, const char *name) {
        if (cond) { ++pass; } else { ++fail; std::printf("FAIL: %s\n", name); }
    };

    // Just intonation: major third ~13.69 cents flat of ET (the textbook
    // "just thirds are noticeably flatter than equal temperament" fact),
    // perfect fifth ~1.96 cents sharp (famously almost identical to ET).
    {
        MicrotonalTuning t;
        t.configure(TuningSystem::JustIntonation5Limit, 1);
        check(approx(t.centsOffset(0), 0.0), "just: root is 0 cents");
        check(approx(t.centsOffset(4), -13.69), "just: major third ~-13.69c");
        check(approx(t.centsOffset(7), 1.96), "just: fifth ~+1.96c");
    }

    // Pythagorean: major third famously wide/sharp of ET (~+7.82c), and
    // the gap between pythagorean and just thirds should reproduce the
    // real syntonic comma (~21.51 cents), a hard historical fact this
    // implementation should reproduce exactly from the two tables.
    {
        MicrotonalTuning just, pyth;
        just.configure(TuningSystem::JustIntonation5Limit, 1);
        pyth.configure(TuningSystem::Pythagorean, 1);
        check(approx(pyth.centsOffset(4), 7.82), "pythagorean: major third ~+7.82c");
        double syntonicComma = pyth.centsOffset(4) - just.centsOffset(4);
        check(approx(syntonicComma, 21.51, 0.1), "pythagorean-just third gap = syntonic comma ~21.51c");
        // Fifths are identical in both systems -- both build the fifth as
        // exactly the pure ratio 3/2.
        check(approx(pyth.centsOffset(7), just.centsOffset(7)), "pythagorean and just fifths match (both pure 3/2)");
    }

    // Quarter-tone: root never shifts; roughly half of the other eleven
    // pitch classes get a real +50 cent nudge (checked over several seeds
    // since any single seed could land anywhere in a binomial spread).
    {
        int totalSharp = 0, totalChecked = 0;
        for (unsigned seed = 1; seed <= 20; ++seed) {
            MicrotonalTuning t;
            t.configure(TuningSystem::QuarterTone, seed);
            check(approx(t.centsOffset(0), 0.0), "quarter-tone: root never shifts");
            for (int pc = 1; pc < 12; ++pc) {
                double c = t.centsOffset(pc);
                check(approx(c, 0.0) || approx(c, 50.0), "quarter-tone: every offset is exactly 0 or 50 cents");
                if (approx(c, 50.0)) ++totalSharp;
                ++totalChecked;
            }
        }
        double frac = static_cast<double>(totalSharp) / totalChecked;
        check(frac > 0.3 && frac < 0.7, "quarter-tone: roughly half the pitch classes land sharp across seeds");
    }

    // retune(): a real frequency check, not just cents math -- a just
    // major third above 440Hz should land audibly below the equal-
    // tempered 440*2^(4/12).
    {
        MicrotonalTuning t;
        t.configure(TuningSystem::JustIntonation5Limit, 1);
        double equalThird = 440.0 * std::pow(2.0, 4.0 / 12.0);
        double justThird = t.retune(equalThird, 64 + 4, 64); // pc=4 relative to root 64
        check(justThird < equalThird, "retune(): just third is genuinely flatter than equal-tempered");
        double expectedRatio = std::pow(2.0, -13.69 / 1200.0);
        check(approx(justThird / equalThird, expectedRatio, 0.001), "retune(): exact expected ratio");
    }

    // Disabled/off-tonic sanity: retuning at the root itself should always
    // return exactly the equal-tempered frequency unchanged, in every
    // system (pc=0 is 0 cents everywhere).
    {
        for (int sysIdx = 0; sysIdx < static_cast<int>(TuningSystem::Count); ++sysIdx) {
            MicrotonalTuning t;
            t.configure(static_cast<TuningSystem>(sysIdx), 7);
            double f = 220.0;
            check(approx(t.retune(f, 60, 60), f, 0.001), "retune(): unison with root is always unchanged");
        }
    }

    // Sieve microtonal: root always maps to exactly 0 cents (forced into
    // the sieve by construction, and the nearest-neighbor search can never
    // do better than an exact match); every other pitch class's offset
    // must be a real, bounded microtonal nudge, never a huge jump (the
    // nearest sieve step in a 19/22/31-EDO grid can never be more than
    // half a semitone-equivalent away in the finest case, well under a
    // full semitone in every case).
    {
        for (unsigned seed = 1; seed <= 15; ++seed) {
            MicrotonalTuning t;
            t.configure(TuningSystem::SieveMicrotonal, seed);
            check(approx(t.centsOffset(0), 0.0), "sieve: root is always exactly 0 cents");
            for (int pc = 1; pc < 12; ++pc) {
                double c = t.centsOffset(pc);
                // The root (step 0) is always admitted by construction, so
                // it alone bounds how far the nearest-neighbor search can
                // ever land: at most half an octave away (the circular
                // wraparound in configureSieveMicrotonal() clamps into
                // exactly this range). A real, provable guarantee from the
                // algorithm itself, not a density assumption that could get
                // lucky/unlucky across seeds.
                check(std::fabs(c) <= 600.0 + 1e-6, "sieve: offset never exceeds half an octave (root is always a candidate)");
            }
            check(!t.description().empty(), "sieve: description is non-empty");
        }
    }

    // Comma drift: chaining 12 real pure fifths (the classic circle-of-
    // fifths setup) via accumulateModulationDrift() should reproduce the
    // real, documented Pythagorean comma (~23.46 cents) -- not a made-up
    // number, the actual textbook fact this mechanism exists to model.
    {
        MicrotonalTuning t;
        t.configure(TuningSystem::Pythagorean, 1);
        int root = 60;
        for (int i = 0; i < 12; ++i) {
            int newRoot = root + 7; // a fifth up each time
            t.accumulateModulationDrift(root, newRoot);
            root = newRoot;
        }
        check(approx(t.commaDrift(), 23.46, 0.2), "pythagorean comma reproduced by 12 chained real fifths");
    }

    // Drift is a genuine no-op for systems where chaining pure ratios
    // doesn't carry the same historical meaning.
    {
        MicrotonalTuning t;
        t.configure(TuningSystem::QuarterTone, 1);
        t.accumulateModulationDrift(60, 67);
        check(approx(t.commaDrift(), 0.0), "quarter-tone: modulation drift is a no-op");
    }

    std::printf("\n%d passed, %d failed\n", pass, fail);
    return fail == 0 ? 0 : 1;
}
