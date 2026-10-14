#pragma once
#include <array>
#include <random>
#include <string>
#include "Scales.hpp"

namespace rasgo {

// The melody/flute object's own harmonic engine (task #31) -- runs
// independently of StepSequencer::chordProgressionEnabled_ (which is only
// active in ~50% of pieces by design, a real gap found auditing the
// project's own cross-seed standardization, task #29), so this always has
// somewhere real to go. "Eixo A" of a three-axis design (vertical
// structure and dissonance treatment are separate, independent axes) --
// this axis alone decides the sequence of key centers (root + scale) the
// flute's phrases wander through, one seed-picked real technique per
// piece, advanced at genuine arc-section boundaries (the same "passagens"
// convention rhythm archetypes -- fase 67 -- and tonal modulation -- fase
// 32/33 -- already retrigger on).
//
// Six real, documented harmonic-movement techniques, deliberately chosen
// so their root-motion INTERVALS are distinct from each other (audibly
// and structurally, not just labeled differently):
//   Coltrane        -- root always +4 semitones (major third), cycling
//                       through 3 key centers before repeating -- the real
//                       "Giant Steps" (1959) interval cycle, dividing the
//                       octave into three equal parts.
//   TritoneSub       -- real jazz ii-V-I root motion (descending fifths,
//                       -7 semitones) MOST of the time; a real tritone
//                       substitution turns that fifths motion chromatic
//                       (+-1 semitone) instead -- the actual, well
//                       documented effect of substituting a dominant for
//                       another a tritone away.
//   ChromaticMediant -- root jumps by a bare third (+-3 or +-4 semitones)
//                       with NO functional constraint on direction -- the
//                       real "non-functional" root motion Jobim's bossa
//                       nova harmony (e.g. "Wave", "Corcovado") is known
//                       for, distinct from tritone-sub's motion because
//                       there is no implied resolution here at all.
//   ModalInterchange -- root never moves; only the MODE changes, borrowed
//                       from the parallel major/minor-family modes on the
//                       same tonic -- real "borrowed chord" technique.
//   ModalJazz        -- mostly static (same key center held across a
//                       section, the real modal-jazz idiom -- Miles
//                       Davis/"Kind of Blue"), with a real, occasional
//                       stepwise shift (the "So What" half-step-up bridge)
//                       rather than every section changing.
//   BackdoorIiV      -- root always +2 semitones (whole step) -- the real
//                       bVII7->I "backdoor" cadence's root motion.
enum class HarmonicMovement {
    Coltrane,
    TritoneSub,
    ChromaticMediant,
    ModalInterchange,
    ModalJazz,
    BackdoorIiV,
    Count
};

struct KeyCenter {
    int rootMidiNote;
    Scale scale;
};

class HarmonicWanderer {
public:
    void configure(HarmonicMovement movement, int initialRootMidiNote, Scale initialScale, unsigned seed) {
        movement_ = movement;
        current_ = { initialRootMidiNote, initialScale };
        rng_.seed(seed);
    }

    HarmonicMovement movement() const { return movement_; }
    const KeyCenter &current() const { return current_; }

    // Called at real arc-section boundaries -- returns the new key centre.
    KeyCenter advance() {
        switch (movement_) {
            case HarmonicMovement::Coltrane:         advanceColtrane(); break;
            case HarmonicMovement::TritoneSub:        advanceTritoneSub(); break;
            case HarmonicMovement::ChromaticMediant:  advanceChromaticMediant(); break;
            case HarmonicMovement::ModalInterchange:  advanceModalInterchange(); break;
            case HarmonicMovement::ModalJazz:         advanceModalJazz(); break;
            case HarmonicMovement::BackdoorIiV:        advanceBackdoorIiV(); break;
            default: break;
        }
        ++advanceCount_;
        return current_;
    }

    int advanceCount() const { return advanceCount_; }

    static std::string movementName(HarmonicMovement m) {
        switch (m) {
            case HarmonicMovement::Coltrane:         return "Ciclo de Coltrane (Giant Steps)";
            case HarmonicMovement::TritoneSub:        return "Substituicao tritonica + ii-V-I";
            case HarmonicMovement::ChromaticMediant:  return "Giro cromatico nao-funcional (bossa nova/Jobim)";
            case HarmonicMovement::ModalInterchange:  return "Intercambio modal";
            case HarmonicMovement::ModalJazz:         return "Jazz modal (Miles Davis)";
            case HarmonicMovement::BackdoorIiV:        return "Backdoor ii-V (bVII7->I)";
            default: return "";
        }
    }

private:
    double unit() { return std::uniform_real_distribution<double>(0.0, 1.0)(rng_); }

    Scale pickAnyScale() {
        int count = static_cast<int>(Scale::Count);
        return static_cast<Scale>(static_cast<int>(unit() * count) % count);
    }

    // Curated "modes sharing a tonic" set for ModalInterchange -- real
    // borrowed-chord relationships (major-family <-> minor-family on the
    // same root), not every catalog scale indiscriminately (the whole
    // point of this technique is the shared tonic, not just variety).
    Scale pickBorrowedMode() {
        static const Scale modes[] = {
            Scale::Major, Scale::Lydian, Scale::Mixolydian,
            Scale::Aeolian, Scale::Dorian, Scale::Phrygian, Scale::HarmonicMinor,
        };
        return modes[static_cast<size_t>(unit() * 7.0) % 7];
    }

    void advanceColtrane() {
        current_.rootMidiNote += 4;
        current_.scale = pickAnyScale();
    }

    void advanceTritoneSub() {
        bool tritoneSubUsed = unit() < 0.4;
        current_.rootMidiNote += tritoneSubUsed ? (unit() < 0.5 ? 1 : -1) : -7;
        current_.scale = pickAnyScale();
    }

    void advanceChromaticMediant() {
        int interval = (unit() < 0.5 ? 3 : 4);
        int direction = (unit() < 0.5 ? 1 : -1);
        current_.rootMidiNote += interval * direction;
        current_.scale = pickAnyScale();
    }

    void advanceModalInterchange() {
        // Root deliberately untouched -- the technique's whole identity is
        // staying on one tonic while the mode above it changes.
        current_.scale = pickBorrowedMode();
    }

    void advanceModalJazz() {
        if (unit() < 0.7) return; // mostly static -- the real modal-jazz idiom
        current_.rootMidiNote += (unit() < 0.5 ? 1 : 2); // the real "So What" half/whole-step shift
        // Scale/mode shape carries over unchanged -- a transposition, not
        // a new mode, same as the real "So What" bridge.
    }

    void advanceBackdoorIiV() {
        current_.rootMidiNote += 2;
        current_.scale = pickAnyScale();
    }

    HarmonicMovement movement_ = HarmonicMovement::ModalJazz;
    KeyCenter current_{60, Scale::Major};
    std::mt19937 rng_;
    int advanceCount_ = 0;
};

} // namespace rasgo
