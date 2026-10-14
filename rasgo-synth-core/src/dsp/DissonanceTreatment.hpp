#pragma once
#include <random>
#include <string>
#include <vector>

namespace rasgo {

// The melody/flute object's "Eixo C" (task #31) -- how each new phrase
// note relates to the scale it's drawn from, independent of Eixo A (which
// key centre is active) and Eixo B (how the accompaniment is voiced
// underneath). Four real, documented dissonance-treatment principles, one
// seed-picked per piece, applied as a pure transform on the semitone
// offset the flute was about to play:
//   ToneAsColor -- the control condition: whatever scale degree was
//                  picked plays exactly as-is, no special treatment
//                  (extensions read as colour, never NEEDING resolution --
//                  the real modal-jazz/impressionist stance).
//   WrongNote   -- a real, bounded chance the note lands a semitone off
//                  from the real scale degree instead, and NOTHING
//                  resolves it afterward -- the deliberate, unapologetic
//                  chromatic clash Stravinsky/Poulenc are known for, a
//                  "wrong" note that's simply never fixed.
//   Bebop       -- a real chromatic passing tone approaching the target
//                  from a semitone below, placed deliberately ON the
//                  strong beat (the note that actually sounds when the
//                  phrase note starts) -- the target itself only sounds
//                  after a short real delay, the classic bebop device of
//                  putting the "wrong" note on the beat and the "right"
//                  one just after.
//   Suspension  -- a real 4-3/9-8 suspension: the PREVIOUS note's pitch is
//                  held past the moment a new one was due, then resolves
//                  down by a real step to the actual target -- the one
//                  style here that always resolves, by construction.
enum class DissonanceStyle {
    ToneAsColor,
    WrongNote,
    Bebop,
    Suspension,
    Count
};

// One note event the flute will actually play, in order -- Bebop/
// Suspension produce two (approach + resolve); ToneAsColor/WrongNote
// produce one.
struct DissonanceNoteEvent {
    int semitoneOffset;
    double fractionOfNoteDuration; // 0..1, when within the note this starts
};

class DissonanceTreatment {
public:
    void configure(DissonanceStyle style, unsigned seed) {
        style_ = style;
        rng_.seed(seed);
    }

    DissonanceStyle style() const { return style_; }

    // targetSemitoneOffset: the real scale degree the harmonic system
    // actually wants played. previousSemitoneOffset: whatever the flute
    // was last sounding (for Suspension's hold-over). Returns 1 or 2 real
    // note events to actually trigger, in order.
    std::vector<DissonanceNoteEvent> apply(int targetSemitoneOffset, int previousSemitoneOffset) {
        switch (style_) {
            case DissonanceStyle::WrongNote: {
                // A real, bounded chance per note -- not every note, or it
                // would just read as permanently out of tune rather than a
                // deliberate occasional clash.
                if (unit() < 0.35) {
                    int direction = (unit() < 0.5) ? 1 : -1;
                    return { { targetSemitoneOffset + direction, 0.0 } };
                }
                return { { targetSemitoneOffset, 0.0 } };
            }
            case DissonanceStyle::Bebop: {
                if (unit() < 0.5) {
                    int approach = targetSemitoneOffset - 1;
                    return {
                        { approach, 0.0 },
                        { targetSemitoneOffset, 0.2 },
                    };
                }
                return { { targetSemitoneOffset, 0.0 } };
            }
            case DissonanceStyle::Suspension: {
                if (unit() < 0.4 && previousSemitoneOffset != targetSemitoneOffset) {
                    return {
                        { previousSemitoneOffset, 0.0 },
                        { targetSemitoneOffset, 0.55 },
                    };
                }
                return { { targetSemitoneOffset, 0.0 } };
            }
            case DissonanceStyle::ToneAsColor:
            default:
                return { { targetSemitoneOffset, 0.0 } };
        }
    }

    static std::string styleName(DissonanceStyle s) {
        switch (s) {
            case DissonanceStyle::ToneAsColor: return "Tensao como cor, sem resolucao (modal/impressionista)";
            case DissonanceStyle::WrongNote:   return "Nota errada cromatica nunca resolvida (Stravinsky/Poulenc)";
            case DissonanceStyle::Bebop:       return "Bebop -- passagem cromatica no tempo forte";
            case DissonanceStyle::Suspension:  return "Suspensao classica (preparacao-suspensao-resolucao)";
            default: return "";
        }
    }

private:
    double unit() { return std::uniform_real_distribution<double>(0.0, 1.0)(rng_); }

    DissonanceStyle style_ = DissonanceStyle::ToneAsColor;
    std::mt19937 rng_;
};

} // namespace rasgo
