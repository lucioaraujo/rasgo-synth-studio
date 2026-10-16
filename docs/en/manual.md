# Rasgo Synth Studio — Manual

Experimental version 0.1.1 — 2026

Reviewed on 2026-10-12 against current controls and contracts. [Step-by-step tutorial](../../website/quickstart.html#en).

## 1. Concept

Rasgo Synth Studio is a generative musical instrument: you choose composition conditions, and it synthesizes a whole piece. The seed is a starting point; tempo, module selection, expressive options, musical direction and relevant state also matter. It is not a note-by-note keyboard instrument or a manually cabled rack.

Sounds are synthesized rather than assembled from an external library of recorded samples. Scales, rhythms, voices, textures and narrative form share an authored space of possibilities. A piece may repeat, transform, interrupt or leave material silent; variation does not guarantee a new sound at every event.

WILD MODE is on by default in Studio and adds entropy during rendering. For controlled comparison, switch it off and retain the seed, tempo, choices, version and relevant external state. A seed alone does not guarantee identical audio.

A single piece's duration emerges from its narrative. In album mode you can keep that emergent duration or enable duration planning. Studio composes/renders first and plays back afterwards; listening controls do not rewrite the saved WAV in real time. Studio and Performance are separate instruments with independent releases.

## 2. History

Rasgo Synth is one work within Rasgo, Lucio de Araujo's umbrella project for his authorial musical instruments and software. Antitotem predates Rasgo and was later incorporated as its precursor and patron. Rasgo's physical modules were already conceived with the Eurorack standard in view, since 2013; today the name covers a broader musical-instrument project.

Kosmische is a separate GPLv3 Android instrument by Nicolas Steven Miller. It was one of the conceptual references studied at the beginning of Rasgo Synth. It is not part of Rasgo, and Rasgo Synth is neither a fork nor an authorial continuation of it.

The project-specific implementation was rewritten from scratch in C++17 and JUCE, under Lucio de Araujo's direction, with Anthropic Claude used as a development tool. No source code from Kosmische or Antitotem was copied. The instrument subsequently received many systems, voices and compositional mechanisms beyond those early references -- at last count, more than eighty documented development phases.

## 3. Research and provenance

The nine chaotic maps are independent implementations of published mathematical recurrences. Kosmische and the SuperCollider Chaos UGens created by Lance Putnam were examined as technical context; *The Chaos Cookbook* itself is Joe Pritchard's 1992 book. These references do not make the Rasgo implementations source-code ports.

Hardware and software instruments cited by Rasgo are design references, not claims of authorship or exact emulation. The modal resonator, granular texture, random sources, filters and circuit-inspired modules are original behavioral interpretations based on public concepts, literature, documentation and schematics. Historic creators and collaborators are named in [CREDITS.md](../../CREDITS.md).

The eight percussion voices are original digital stylizations inspired in part by documented Roland TR-808 behavior. They use no samples, copy no firmware and are not transistor-level circuit replicas. Every piece picks among eight possible rhythmic archetypes: Soul, Jazz, Metal, March, Samba, two real Balkan aksak archetypes (documented by Constantin Brăiloiu and Stoyan Dzhudzhev), and one rhythmic sieve algorithmically generated via Xenakis's sieve theory.

For the authoritative provenance list, see [CREDITS.md](../../CREDITS.md). Incorporated libraries and their licensing terms are listed separately in [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md).

## 4. Selected references

- Lucio de Araujo: Rasgo and Antitotem.
- Nicolas Steven Miller: Kosmische (GPLv3), an external conceptual reference.
- Joe Pritchard: *The Chaos Cookbook* (1992).
- Lance Putnam: SuperCollider Chaos UGens (2004).
- Forrest M. Mims III: the Stepped-Tone Generator popularly known as the Atari Punk Console.
- Iannis Xenakis: dynamic stochastic synthesis and sieve theory (*Formalized Music*, ch. XI).
- Instruō, Ben "DivKid" Wilson (ochd) and Jason Lim (Harmonàig).
- Qu-Bit Electronix, Andrew Ikenberry: Bloom.
- Émilie Gillet / Mutable Instruments: published Eurorack DSP concepts.
- Roland Corporation: TR-808 documentation and sound-design reference.
- Roger Caillois: *Man, Play and Games* (1958), the four categories of play (Alea/Agon/Ilinx/Mimicry).
- Constantin Brăiloiu, Stoyan Dzhudzhev: Balkan additive/aksak rhythm theory.
- The remaining creators listed in [CREDITS.md](../../CREDITS.md).

## 5. License

Rasgo Synth Studio's own code is distributed under the GNU Affero General Public License version 3 or later (AGPL-3.0-or-later); see `LICENSE`. Kosmische's GPLv3 license applies to Kosmische and does not establish shared authorship or copied code.

The desktop application uses JUCE. The local JUCE 9 source offers AGPLv3 or a commercial JUCE license, so anyone distributing a combined build must comply with the applicable JUCE terms in addition to Rasgo Synth's own license. Other bundled or linked components retain their respective licenses; see [THIRD_PARTY_NOTICES.md](../../THIRD_PARTY_NOTICES.md).

## 6. Tutorial

### Before you start

This guide describes experimental Studio 0.1.1, not Rasgo Modular or Performance. Keep listening volume low, choose a writable output folder and work on one piece at a time. The eighth-note icon ♪ starts composition; PLAY plays rendered audio. The control names below match the English interface.

### Your first piece — six steps

1. Check the audio output, lower MASTER and choose the output folder. Optionally enter title and artist; files are saved there.

    The artist field is optional: leaving it empty assigns no artist to the
    audio. A name you enter is saved in your preferences. The initial output
    folder is `Rasgo Synth` inside your own Music folder; you can change it.
2. Set SEED and TEMPO. Turn WILD MODE off for this exercise. Leave MUSICAL DIRECTION and EXPRESSIVE off, with modules on Automatic, as a starting reference.
3. Press COMPOSE (♪). Wait for rendering and the saved confirmation; PLAY is not how you start composition.
4. Press PLAY. Listen without changing anything first: notice entrances, exits, rests, low register, percussion and textures.
5. Drag on the waveform to select a region and use PLAY REGION. SEED FROM REGION derives another starting point, not a sample export. Compose again to hear it.
6. Keep the WAV and companion files. Use ALEA for another seed, or follow the exercises below changing one choice at a time.

### Seed and tempo

What it is: SEED chooses a starting point; TEMPO sets a piece's initial tempo.

How to think about it: the seed is a composition condition, not a preset name explaining the entire result. The engine can change tempo inside a piece. The engine chooses the scale; this panel has no manual scale selector.

Controls: SEED, ALEA, TEMPO and WILD MODE. In album mode the theme supplies the base BPM; the single-piece slider is disabled.

Try it: write down seed and tempo, render with WILD MODE off and listen. Keep the seed, change only TEMPO and render again. Compare gesture length, spacing and perceived energy; this is not simply a speed change applied to the same WAV.

### Choose who may play

What it is: CHOOSE MODULES opens 34 participants covering sound sources, textures and drums.

How to think about it: you choose eligibility, not a solo or mute of current audio. Include permits participation; notes, entrances, rests and dependencies still shape performance. Other mechanisms and effects remain automatic. The activity grid is not this control list.

Controls: Automatic preserves the generative choice; Include permits the designated source; Exclude blocks it. Confirm with APPLY TO NEXT RENDER. Closing without applying discards edits; ALL AUTOMATIC resets the draft before confirmation. Restarting the app resets choices.

Try it: keep seed, tempo and modes. Include Colored noise bed, apply and render. Then exclude that same layer and render again. Compare texture and the space occupied by noise. Other generators can still produce noise: Exclude does not silence a whole sonic family.

### Drums and groove

What it is: the kit contains kick, snare, hi-hat, cowbell, clap, toms, cymbal and maracas, synthesized rather than sampled.

How to think about it: participants have rhythmic roles, but Studio offers no individual tuning or volume knobs for them. Excluding Drum kit blocks its parts even if Kick is marked Include. Including a participant does not force constant hits.

Controls: CHOOSE MODULES; switch on MUSICAL DIRECTION and EXPRESSIVE before composing to explore additional groove and interpretation studies. Seeds and sections select families and responses; there is no manual genre or 3:2 polyrhythm selector.

Try it: include Drum kit, Kick, Snare and Hi-hat and render. Keep everything else unchanged, exclude Cowbell and Maracas, then render again. Listen for a more legible pulse. Do not raise MASTER just to compensate for one part: uneven mixing is a known limitation, not your mistake.

### Low register and bass line

What it is: Acid bass and Tuba are selectable sources; internal core-sub bass is a different function from Core voice.

How to think about it: low frequency, a musical bass line and volume are different things. A bass line needs events, register and articulation; a module name does not guarantee clear presence in the mix. Core-sub fallback has no separate selection item.

Controls: Acid bass/Tuba selection and MUSICAL DIRECTION + EXPRESSIVE. There is no individual bass fader or solo button here.

Try it: with both modes on, include Acid bass and exclude Tuba; apply and render. Then reverse that choice. Listen to attack, sustain, rests and the relationship with kick. Excluding both can leave fallback available depending on the musical plan; it does not guarantee recognizable bass. Report seed and choices if the low register is masked.

### Melody, timbre and relationships

What it is: voices such as FM flute, Theremin and Choir, and textures such as Granular, offer different sonic material.

How to think about it: selection is not turning knobs in a rack. Trajectories are calculated during rendering. Choir, Scat and Harmonaig depend on the generated chord progression; an absent dependency may block participation, recorded as `blockedBy` in metadata.

Controls: CHOOSE MODULES, MUSICAL DIRECTION and EXPRESSIVE. Direction organizes entrances, withdrawals and relationships; Expressive adds experimental interpretation. They are separate options, off by default, and require a new render.

Try it: render a reference with both modes off. Turn only Direction on and render; then also enable Expressive and render again. Keep seed, tempo, modules and Stem Audit unchanged. Compare rests, responses, attacks and timbre, not just volume. Contrast can be subtle; greater emotion or audible dialogue is not guaranteed in every piece.

### Listening and visualizations

What it is: waveform, spectrum, spectrogram and other displays help observe audio.

How to think about it: a module indicator shows activity, not proof of audibility or musical quality. Website animations are illustrations, not your application's audio.

Controls: PLAY, waveform position, PLAY REGION, MASTER and listening brightness. These do not rewrite the saved WAV. In smaller windows, scroll vertically to reach log and displays.

Try it: listen to a region and relate a percussion attack to the waveform; then look for a sustained texture in the spectrum. A small waveform alone does not prove poor mastering. If you cannot identify a voice, report it rather than increasing volume without limits.

### Your first album

What it is: ALBUM MODE makes a collection from a seed, theme and plan.

How to think about it: the theme supplies base BPM; planning affects future renders, not saved tracks. There is no compositional memory that relearns themes across tracks yet.

Controls: theme, TRACKS (1–40; default 8), VARY BPM (off by default; spread 0–35%, default 15%), balanced/contrasting/rising/arch contour. PLAN DURATION is also off initially. When enabled, it provides a 1–45-minute total target and balanced/contrasting/free distribution. The plan bounds tracks to 15 seconds–8 minutes and total to 45 minutes; preview distinguishes requested and planned totals when a request cannot fit.

Try it: choose 2 tracks, enable PLAN DURATION and request 2 minutes total. Read the preview; enable VARY BPM to compare initial tempos. Compose the album and listen with the LISTEN selector. PLAY restores displays; CONFIGURE ALBUM reopens options. Planned musical duration is not processing time or a cut applied afterwards to the WAV.

Album calibration passes precede final renders. `album_plan.json` records the plan and actual files; interrupted albums are not completed albums. Module selection and expressive modes apply to all subsequent tracks. Stem Audit inherits the option set in piece mode even though its control is disabled in album mode: set it beforehand. Its old on-screen tooltip does not correctly describe this inheritance.

### Progress, files and problems

What it is: percentage follows musical frames in the current pass, not elapsed wall-clock time.

How to think about it: 100% composition can still be followed by final processing and saving. In an album, percentage and calibration do not form a single estimate of total remaining time. Stem Audit adds analysis and processing and can affect the result, not just render time.

Controls: log, COPY, output directory, ABOUT and TUTORIAL. ABOUT shows version and build date/time. Closing during rendering requests cancellation and waits for a checkpoint; a write in progress may need to finish. This does not guarantee crash or power-loss recovery.

Try it: after your first piece, find the WAV and companions in the folder. If something fails, copy the log and report version, system, device, seed, modes, selection and preceding action. Do not upload personal recordings automatically. Avoid concurrent renders/builds when memory is under pressure.

### Limits of this version

An experimental release is not an approved mix: bass can be quiet, some percussion stands out and timbres may recur. Generative mixing and interpretive diversity remain post-publication development work. Noise, silence and rupture are possibilities, not mandatory aesthetics. Reviewing instructions does not replace listening, accessibility or all-keyboard/language validation.

[Whole-instrument guide](Instrument_Overview.md) · [Credits](../../CREDITS.md) · [Rhythm method and references](../GROOVE_RHYTHM.md)
