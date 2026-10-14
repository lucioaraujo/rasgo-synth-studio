# Rasgo Synth — whole-instrument overview

This guide explains the instrument as a whole: what enters the system, how a
piece is built, how to read the Studio, and how the module catalogue relates to
the music.

## Studio and Performance

Rasgo Synth Studio is the existing desktop instrument for composition,
rendering, listening and research. Rasgo Synth Performance is a separate
live-DSP instrument with local contracts, engine, standalone host and an initial
four-scene/five-gesture surface. It remains unreleased: graphic, audio-device,
musical and portable-build validation are still open. The Studio's live controls
do not constitute a Performance release.

## From seed to piece

Rasgo Synth is a generative software instrument, not a fixed loop, sample
player, or note-by-note keyboard. A seed selects a trajectory through an
authorial space of identity, scales, rhythms, voices, probabilities, narrative
roles and sonic matter. The engine then renders a complete piece.

The path has nine layers:

1. **Identity:** seed, tonal centre, scale family, material focus and voice
   possibilities.
2. **Game:** Alea, Agon, Ilinx and Mimicry bias chance, competition, instability
   and imitation.
3. **Time:** grid, pulse, subdivision, swing, microtiming, humanization, gate,
   probability, phase, polymeter, drift and memory.
4. **Matter:** oscillators, resonators, filters, noise, granular, FM, vector and
   synthesized percussion bodies.
5. **Form:** a Markov narrative walk shapes opening, development, climax,
   rupture, settling and closure.
6. **Relations:** voices may answer, compete, interrupt, preserve, deform or
   yield to one another.
7. **Expression:** expressive A/B adds articulation, internal dynamics, pitch
   approach, gesture, failure and recovery to selected events.
8. **Attention and mix:** post-space measurement and limited narrative mixing
   protect events without forcing every module to be loud.
9. **Record:** PCM24/48 kHz WAV, metadata, a textual score and telemetry.

Not every module is active in every seed. `Available`, `manifested`, `audible`
and `influential` are different states. A tuba, flute or bass body is never a
proxy for the whole instrument.

## Bass, melody and percussion

The bass is separated into function, phrase and body. Function may be root,
pedal, pathway, tension, counterpoint or space. Phrase controls register,
direction, repetition, relation to the kick and density. The body may be a
synthetic tuba, an acid voice or a core-sub fallback. The body does not define
the line by itself.

Melody uses scales, voice leading, contour, voicing, dissonance treatment,
expressive tuning and memory. Percussion uses independent voices, real rhythmic
archetypes, Euclidean and aksak structures, articulation, timbral variation and
interlocking. The temporal groove imprint currently feeds expressive bass and
archetype percussion with bounded influence.

## Reading the Studio

- **SEED / ALEA:** choose or derive the starting point.
- **TEMPO:** set the initial pace; the arc may drift internally.
- **MASTER:** set final output level, not compositional energy.
- **WILD MODE:** allow additional entropy and alternate trajectories.
- **STEM AUDIT:** measure post-space survival and narrative attention.
- **EXPRESSIVE A/B:** enable experimental temporal and gestural behavior.
- **COMPOSE:** render the complete piece in the background.
- **LANG:** switch English, Portuguese, French or Spanish.

The panels are readings of one result: waveform, oscilloscope, spectrum,
spectrogram, goniometer, phase portrait, granulation, arc genealogy, game
weights, module grid and log. The `.txt` companion is a readable score-like
account of sections, decisions, voices and mechanisms; it is not conventional
staff notation.

**STEM AUDIT** is a measurement lens, not a requirement for listening. **COPY**
copies the log and **CLEAR** clears its presentation without deleting an audio
file. Region selection can play a fragment or derive a new seed from the raw
sample bytes. **VARY SEED** derives a new composition from a moment of playback.

## Album mode

Album Mode creates several related tracks from an album seed. `AlbumRenderer`
derives track seeds, preserves broad tonal continuity and gives every track its
own narrative and module decisions. It is a family of pieces, not one long
file cut into sections. The track selector loads each result into the same
waveform and listening panels.

## Flowchart and catalogue

The architecture flowchart shows causality: seed feeds identity, sequencing,
the arc and DSP; the arc feeds density, register and energy; game, relations
and mix observe or influence within limits. Solid arrows are primary flows;
dotted arrows are bounded influence or observation.

The web catalogue contains 142 mechanisms with category, description and
reference. It is an index of possibilities, not a promise that every mechanism
will be heard in every piece. See the [Portuguese integral guide](../pt/Instrumento_Integral.md)
for the fuller operational vocabulary and [the catalogue](../../website/assets/mechanisms.pt.json).

## Limits and next work

Some performance, memory, attention and mix contracts still have local or
observational consumers rather than universal PCM behavior. Musical acceptance
therefore requires both telemetry and human listening. The next work is to make
the grave line more legible without forcing a body, strengthen counterpoint
when the form allows it, and extend note morphology to more voice families with
tests and bounded contracts.
