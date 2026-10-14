#pragma once
#include <array>
#include <string>

namespace rasgo {

// Fase 67 -- real rhythmic identity for the drum kit, requested directly
// after a listening pass reported the percussion "always sounds like the
// same rhythm" across a whole album. The existing PolyrhythmDivider (CGS22
// XOR clock, see PieceRenderer.hpp) only ever permutes 6 fixed stage-pairs
// derived from ONE shared divide-by-2/4/8/16 counter -- structurally
// incapable of producing genuinely different-FEELING patterns (it's
// always "16th-note grid with a different XOR offset"). This is a wholly
// separate, alternative pattern source: explicit fixed 16-step onset grids
// for kick/snare/hihat, one per archetype, chosen per SECTION (not per
// piece) so a piece can move between feels across its own arc -- the
// "variacao e mistura deles, com passagens" the user asked for.
//
// Honesty discipline (same as fase 57's Bjorklund/Toussaint naming):
// each archetype below is grounded in a real, well-documented, easily
// verifiable rhythmic characteristic -- backbeat position, swing
// subdivision, blast-beat density, a march's steady on-beat pulse. Where
// the correspondence to a real tradition is a stylization rather than a
// literal transcription of one specific school/pattern (samba batucada
// has many named, more intricate variants this single 16-step grid can't
// represent), the comment says so plainly rather than claiming more
// precision than is actually there.
struct DrumArchetype {
    std::string name;
    std::array<bool, 16> kick{};
    std::array<bool, 16> snare{};
    std::array<bool, 16> hihat{};
    // Legacy swing delays odd-indexed sixteenth positions by a fraction of
    // one step. This is NOT the same subdivision as swung jazz eighths;
    // keep its historical PCM, without claiming exact jazz timing.
    // GrooveDialogue supplies separately documented original studies.
    double swingAmount = 0.0;
};

// index 0..6, matched 1:1 with the descriptions below.
inline const std::array<DrumArchetype, 7> &drumArchetypes() {
    static const std::array<DrumArchetype, 7> table = {{
        // Marcha: steady on-beat pulse on BOTH kick and snare, every
        // quarter note -- the actual defining trait of a march (unlike a
        // backbeat, which accents only 2 and 4). Real, simple, verifiable.
        DrumArchetype{
            "Marcha",
            {true,false,false,false, true,false,false,false, true,false,false,false, true,false,false,false},
            {true,false,false,false, true,false,false,false, true,false,false,false, true,false,false,false},
            {false,false,true,false, false,false,true,false, false,false,true,false, false,false,true,false},
            0.0
        },
        // Soul/backbeat: snare hard on 2 and 4 -- THE most documented,
        // safest rhythmic claim in all of popular-music theory. Kick on 1
        // and the syncopated "and of 2", steady 8th-note hihat.
        DrumArchetype{
            "Soul (backbeat)",
            {true,false,false,false, false,false,true,false, false,false,false,false, false,false,false,false},
            {false,false,false,false, true,false,false,false, false,false,false,false, true,false,false,false},
            {true,false,true,false, true,false,true,false, true,false,true,false, true,false,true,false},
            0.0
        },
        // Metal (blast beat): kick and snare alternating on every single
        // 16th note, maximum density -- the real, named "blast beat"
        // technique, immediately and audibly distinct from everything
        // else this engine generates.
        DrumArchetype{
            "Metal (blast beat)",
            {true,false,true,false, true,false,true,false, true,false,true,false, true,false,true,false},
            {false,true,false,true, false,true,false,true, false,true,false,true, false,true,false,true},
            {false,false,false,false, false,false,false,false, false,false,false,false, false,false,false,false},
            0.0
        },
        // Jazz (swing): sparse kick ("feathering"), sparse off-beat snare
        // comping, and a swung quarter-note-plus-and ride pattern on the
        // hihat -- the real "ding-ding-a-ding" shape, with swingAmount
        // actually delaying the "and" positions rather than just placing
        // them differently. Comping positions are a stylization (real
        // jazz comping is improvised, not a fixed 16-step grid) -- kept
        // deliberately sparse/irregular-looking rather than a tidy pattern
        // that would misrepresent it as more rule-bound than it is.
        DrumArchetype{
            "Jazz (swing)",
            {true,false,false,false, false,false,false,false, false,false,false,true, false,false,false,false},
            {false,false,false,true, false,false,false,false, false,false,false,false, true,false,false,false},
            {true,false,false,true, true,false,false,true, true,false,false,true, true,false,false,true},
            0.62
        },
        // Samba-inspirado: the surdo (bass drum)'s heavy accent on beat 2
        // -- real samba is often described as "felt on the two", distinct
        // from the backbeat's own emphasis -- plus a continuous 16th-note
        // subdivision (standing in for the tamborim/ganza's steady drive)
        // and syncopated snare/clap-register accents off the beat. This
        // is a stylization capturing that one well-documented character,
        // not a transcription of any specific named batucada pattern
        // (surdo primeiro/segundo, partido-alto etc. -- real schools with
        // more nuance than one 16-step grid can honestly represent).
        DrumArchetype{
            "Samba (inspirado)",
            {false,false,false,false, true,false,false,true, false,false,false,false, true,false,false,false},
            {false,false,true,false, false,false,false,false, false,true,false,true, false,false,false,false},
            {true,true,true,true, true,true,true,true, true,true,true,true, true,true,true,true},
            0.0
        },
        // Aksak 9+7 -- a real Balkan additive-rhythm structure (Brăiloiu/
        // Dzhudzhev's aksak theory: asymmetric long(3)/short(2) beat
        // groupings, not stylized into this engine's usual straight-8th/
        // 16th grid). Two real, individually-documented aksak cells --
        // a 9-count (2+2+2+3) and a 7-count (2+2+3), both common Balkan
        // meters on their own -- concatenated to fill this engine's fixed
        // 16-step frame exactly (9+7=16), rather than forcing an actual
        // different bar length engine-wide (a bigger architectural change
        // this project deliberately isn't taking on right now). Kick marks
        // the start of every group (the "limping" aksak pulse itself);
        // snare doubles the two "slow" (3-count) group onsets specifically
        // -- a real aksak technique, the long beat getting the heavier
        // accent, not an arbitrary backbeat position. Steady 16th hihat,
        // same convention as Metal/Samba above.
        DrumArchetype{
            "Aksak 9+7 (2+2+2+3+2+2+3)",
            {true,false,true,false,true,false,true,false,false,true,false,true,false,true,false,false},
            {false,false,false,false,false,false,true,false,false,false,false,false,false,true,false,false},
            {true,true,true,true, true,true,true,true, true,true,true,true, true,true,true,true},
            0.0
        },
        // Aksak 7+7+2 -- a different real aksak cell (2+2+3, the most
        // commonly cited Balkan/Turkish 7-count grouping) stated twice,
        // with two leftover steps at the end read as a short anacrusis/
        // pickup back into the top of the cycle (a real, documented way
        // aksak phrases get extended or bridged, not padding invented to
        // make the arithmetic work). Same accent logic as the archetype
        // above: kick on every group start, snare doubling the two
        // "slow" onsets.
        DrumArchetype{
            "Aksak 7+7+2 (2+2+3 x2 + pickup)",
            {true,false,true,false,true,false,false,true,false,true,false,true,false,false,true,false},
            {false,false,false,false,true,false,false,false,false,false,false,true,false,false,false,false},
            {true,true,true,true, true,true,true,true, true,true,true,true, true,true,true,true},
            0.0
        },
    }};
    return table;
}

} // namespace rasgo
