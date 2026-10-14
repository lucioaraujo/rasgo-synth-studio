# Rasgo Synth — Project Reference Document

Generative instrument — concept, history, architecture, features, usage flow, work performed, validations, security, layout and open items.

> **About this document**: this is the project's stable technical reference, updated as real upgrades happen (not every session). For the phase-by-phase logbook, with the complete history of decisions and the reason behind each one, the source of truth remains [`Plano_de_Upgrade_Instrumento_Generativo.md`](../../Plano_de_Upgrade_Instrumento_Generativo.md) (Portuguese only) at the project root — in case of divergence between the two, that document is authoritative.

Last updated: August 22, 2026 (reference synchronized through the operational work of phases 118–123).

---

## 1. Concept

Rasgo Synth is a **generative instrument**: you set an initial configuration (a numeric seed, a tempo) and press compose, and the software writes an entire piece by itself, with continuous development and variation over time, not a repeating loop. A live-performance console remains possible as a secondary mode (the app already plays audio in real time via `juce_audio_devices`), but the primary target was always something else: to serve as the basis for album tracks.

The phrase that sums up the project, and that appears in the app's own interface today, is: **"it takes a risk, and it sounds"** ("se arrisca, e soa"). Every run is sonically unique by nature; the engine is built on chaotic dynamics (nine maps — Lorenz, Hénon, Latoocarfian and six others from the "Chaos Cookbook") and unstable oscillators that cross-modulate and feed back into themselves, the same sonic quality found while experimenting with the CMOS circuits of the Antitotem hardware project that precedes this software.

A design principle that runs through the whole project: **port the idea, don't copy the circuit**. Whenever a real schematic (CBS, Buchla, TR-808, MFOS/Ray Wilson, Instruō, Qu-Bit Electronix and several others) was studied, the goal was to understand the sonic behavior and reimplement it in plain C++, never to simulate transistor by transistor.

The philosophical why behind this technique is recorded in [`Manifesto.md`](Manifesto.md).

Another principle, reaffirmed several times throughout development: when a parameter can be decided either by the seed or by manual user control, the project tends to pick one or the other, not an arbitrary mix. The scale itself stopped being a manual control (phase 48) — today it is always the seed's own choice, from a repertoire of 39 researched scales plus a slice of algorithmically generated scales (Xenakis sieves, phase 61). Manual controls survived only where they make sense as "tuning the instrument" (tempo, output volume), not as an overlay on top of structural decisions the seed already makes.

An explicit turning point happened at phase 80: from "adding isolated modules" to "multiplying encounters between modules that already exist." The **Caillois game weights** (Alea/Agon/Ilinx/Mimicry, see section 4.9) formalize this into a real rule-making machine, and phase 84 (Curto-Circuito/Short-Circuit, section 4.10) pushes that logic to an extreme: a single event can rupture the very protocol the seed itself had established.

---

## 2. History

### 2.1 Kosmische (2013)

One of the initial technical references was a forensic study of Kosmische, an external project by Nicolas Steven Miller: an Android app that embedded a full SuperCollider server via the NDK. Kosmische's native binary carried three relevant UGen families: `ChaosUGens` (the nine chaotic maps), `GendynUGens` (Xenakis's dynamic stochastic synthesis) and `AY_UGen` (AY-3-8910 chip emulation). That investigation informed concepts in Rasgo Synth, but Kosmische is not part of Rasgo and is not its authorial origin.

### 2.2 Antitotem and Rasgo's physical modules

Antitotem is a hand-built, modular, CMOS/discrete-logic hardware instrument that precedes Rasgo and was later incorporated into the project as its precursor and patron. The phrase "Rasgo in hardware" refers to the set of authorial physical modules from that phase (since 2013), already conceived with the Eurorack standard in view.

Since then, Rasgo has grown into something larger: today it is the umbrella project that gathers all of Lucio de Araujo's authorial musical instruments and software. Rasgo Synth is one work within that set, while Antitotem simultaneously remains an incorporated work and the lineage's symbolic origin. Rasgo Synth Performance is now an independent local standalone prototype for live DSP; an embedded console or hardware remains a later possibility, not the definition of that software instrument.

### 2.3 Architecture decision

The technical decision was to not revive the SuperCollider server as a runtime dependency. The concepts investigated were reimplemented in C++ as Rasgo Synth's own code, and then extensively expanded — today, dozens of mechanisms beyond what existed in 2013.

### 2.4 Licensing

Rasgo Synth Studio's current authorial code uses **AGPL-3.0-or-later**. Kosmische is also GPLv3, independently. The specific conditions of the app built with JUCE are recorded in `THIRD_PARTY_NOTICES.md`; full per-mechanism attribution is in `CREDITS.md`.

---

## 3. JUCE / C++ Architecture

The project is split into two independent subprojects.

### 3.1 `rasgo-synth-core` — the DSP engine

Plain C++17, with no platform dependency. Compiled directly with g++ (no CMake on this machine).

- **`src/dsp/`** — dozens of DSP classes: classic oscillators with PolyBLEP; nine chaotic maps; a Walsh oscillator (±1 orthogonal basis, phase 59); VOSIM (decaying sin² pips, phase 72); Moog ladder filter, Chamberlin state-variable filter and self-oscillating Polivoks filter (phase 75); diode waveshaper; modal resonator; granular cloud; XOR polyrhythmic clock divider; eight digital percussion voices; `RhythmSieve.hpp` (Xenakis rhythmic sieve, phase 82) and `DrumArchetypes.hpp` (7 named archetypes + sieve slot); Rungler, Fracture Folder, Leibniz Rectifier; `ParenthesisGlitch.hpp` (phase 70); `SieveScale.hpp` (Xenakis sieve-generated scale, phase 61); Gendyn, AY-3-8910; a Buchla-style random voltage source.
- **`src/sequencer/`** — 39 researched scales (`Scales.hpp`, phase 48) plus generated sieves; a step sequencer (`StepSequencer.hpp`) with real semitone-distance voice leading, hocketing/polyphonic cascade, chord progressions and Neo-Riemannian P/L/R transformations (mutually exclusive); a narrative arc via Markov chain with pluggable grammars (`GenerativeArc.hpp` + `ArcTemplates.hpp`).
- **`src/engine/`** — `PieceRenderer.hpp` (the main render function, ~50 seed-gated mechanisms) and `AlbumRenderer.hpp` (phase 83: each album track occupies an interpolated point on a Caillois game-weight trajectory, freely picking its own scale — no longer a forced shared tonal center).
- **`src/io/`** — `WavWriter.hpp` (metadata via RIFF LIST/INFO chunk) and `Recorder.hpp`.
- **CLI demos** — `rasgo_integrated_demo` (standalone piece) and `rasgo_album_demo` (track set).
- **`regression_test.py`** — automated test script (section 7).

### 3.2 `rasgo-synth-app` — the JUCE desktop application

- **`src/MainComponent.h`** — the main screen (section 5).
- **`src/RasgoLookAndFeel.h`** — the "Cobre" (Copper) visual identity (section 9), including `LogoMark` (the author's own real vector logo, loaded at runtime from an SVG file via `juce::Drawable`).
- **`src/ArcTimelineView.h`, `WaveformView.h`, `OscilloscopeView.h`** (now stereo), **`SpectrumView.h`** (Goertzel filter bank), **`LevelMetersView.h`** (horizontal RMS/peak), **`GameWeightsView.h`**, **`GranulationView.h`** (particles) — the main screen's visualizations.
- **`src/BufferPlaybackSource.h`** — the JUCE playback source built from the already-rendered buffer, with a live filter and a master gain stage.
- **`src/Main.cpp`** — the window opens maximized by default (`setFullScreen`); a `juce::Viewport` (`RasgoViewport`) guarantees real scrolling on monitors smaller than the comfortable layout requires, instead of clipping or overlapping controls.
- **`build_juce_minimal.sh` / `build_app.sh`** — the build recipe without CMake/Projucer.
- **`autotest.sh`** — the app's automated smoke test (section 7).

Live audio is served by `juce_audio_devices`/`juce_audio_basics`, with ALSA as the only path on Linux, and the "pulse" pseudo-device preferred over the raw "default."

---

## 4. Features

### 4.1 Sound core

Six distinct synthesis principles coexist: subtractive (oscillators + digital ladder, Chamberlin or Polivoks), additive (Walsh, VOSIM), resonant (modal resonator, including the continuously-blown tuba/breath voice), granular (`GranularCloud`), stochastic (`GendynOscillator`) and vector (Vector Synthesis, phase 76 — a real 2D crossfade across a dedicated bank of Gendyn/Walsh/Atari Punk/colored noise, position driven by two chaos maps). All nine chaotic maps and four slow LFOs remain active.

The two main oscillators (`osc1`/`osc2`) can modulate each other in **bidirectional cross-FM**. A slow envelope follower can feed the piece's own audio output back into the Lorenz map's own rate (**self-feedback**). A **colored noise** layer adds to the granular/Gendyn texture.

Additive accompaniment layers: **Harmonàig** (phase 77, a real 4-voice chord pad — root/3rd/5th/7th, inspired by the real Instruō module by Jason Lim), **Xenakis sieve polyphony** (phase 78, 4 independent channels — wood/breath, metal/glass, water, reactive plasma) and **Bloom** (phase 79, a fractal Trunk/Branch/Path melody, inspired by the real Qu-Bit Electronix module by Andrew Ikenberry).

### 4.2 The narrative layer — the generative arc

The duration and shape of every piece is a **Markov walk** over five structural roles, ending only at the coda. Two pluggable, blendable grammars: **Dramatic** (Freytag's pyramid) and **Kishōtenketsu**. An accelerando toward the climax (phase 68) mirrors the already-existing coda ritardando.

### 4.3 Sequencing and scales

39 researched scales (phase 48) plus scales generated algorithmically via Xenakis sieve theory (phase 61, rejection-sampled against the real catalog). Real semitone-distance voice leading (phase 62), genuine chord progressions (phase 34), and, as a mutually exclusive alternative, Neo-Riemannian P/L/R triadic transformations (phase 65, verified parsimonious voice leading). Hocketing with a shift-register-style polyphonic cascade (phase 63). An XOR clock divider generates polyrhythm independent of the main euclidean grid; the melody's own read pointer can be addressed by a dedicated chaotic source instead of the linear clock (phase 64).

### 4.4 Percussion

Eight independent digital voices (kick, snare, hihat, cowbell, clap, tom/conga, cymbal, maracas), inspired in part by the TR-808. Presence is now tied to the arc's own density, not a fixed constant (phase 66) — 12% of pieces have no drums at all, and a fraction has real per-hit failures. Eight rhythmic archetype options per arc section (phases 67 + 82): March, Soul-backbeat, Metal-blast-beat, Jazz-swing, Samba-inspired, two real Balkan aksak archetypes (Constantin Brăiloiu/Stoyan Dzhudzhev, 9+7 and 7+7+2), and one generated Xenakis rhythmic sieve (same math as section 4.3, applied to points in time). Real euclidean rhythm (Bjorklund/Toussaint) as an alternative for the cowbell (phase 57). Expanded dynamics: hihat in the chaos contagion, tom ratchets, extended cymbal sustain (phase 56).

### 4.5 Stereo output

Channels genuinely diverge from the delay/reverb stage onward (independent, slightly desynchronized lines). A chaotic binaural beat (phase 73) adds a second, real and verifiable source of L/R divergence, independent of pan/delay.

### 4.6 Album mode

An album seed deterministically derives N track seeds. Through phase 82, every track shared a forced tonal center (same scale/root). **Starting at phase 83**, that changed: the album now walks a **straight-line trajectory across the four Caillois game weights** (section 4.9) — the first track is born at one seed-derived point, the last track ends at another seed-derived point, every track in between occupies its own interpolated point, and each track freely picks its own scale, just as a standalone piece would. A named **theme** still sets bpm/root note/energy bias (mood), but no longer the scale.

### 4.7 Sound quality — technical hygiene and mastering

TPDF dither, `DcBlocker`, `LookaheadLimiter` + safety saturator, PolyBLEP (~46dB of measured aliasing reduction), oversampling in the diode mix, two-pass album mastering. A real bug of sustained instability in the tuba/wood channel's `ModalResonator` (continuous excitation pushing the resonator near its own safety ceiling, ~25x louder than the piece) was fixed in phase 81 with a soft `tanh` output limiter, the same pattern already used for Polivoks.

### 4.8 Capture, metadata and index

Every WAV carries embedded metadata (RIFF LIST/INFO chunk) and a companion `.txt` file readable as a score (phase 54) with the piece's full trajectory. A plain-text account of the piece itself (seed, sections) lives in a trailing RIFF chunk readable in a text/hex editor, invisible to any player (self-epitaph, phase 46). Every successful recording appends a line to `rasgo_index.jsonl`.

### 4.9 Caillois game weights (phase 80)

Four continuous weights — **Alea, Agon, Ilinx, Mimicry** — sampled uniformly from the 4-simplex via "stick-breaking" (three sorted random cut points, the four gaps are the weights), so every piece gets its own authorial blend instead of picking from a fixed list of "games." Alea has no mechanism cluster of its own — it IS the absence of thematic bias, the engine's own baseline behavior. The other three nudge (never force) eleven already-existing mechanisms: **Agon** (voices competing/coordinating) → Agon itself, Harmonàig, sieve polyphony; **Ilinx** (vertigo/loss of control) → Rungler, Rössler collapse, thermal fade, self-feedback, stochastic mute, Leibniz rectifier — and, since phase 84, also Curto-Circuito (section 4.10); **Mimicry** (simulacrum) → apocryphal tear, self-epitaph, foreign signal, parenthesis, the strange element ("elemento estranho"). Honestly attributed to Roger Caillois (*Les Jeux et les Hommes*, 1958).

### 4.10 Curto-Circuito / "Short-Circuit" (phase 84)

A single per-piece event, biased by Ilinx, with two functions in the same instance. **A real hard cut**: a gate (~3ms linear ramp, never a fade curve) closes to true silence for 100-400ms and reopens the same way — the exact signature identified in a real waveform/spectrogram analysis of one of the author's own reference tracks. A noise "spark" rides only the transition edges, literalizing "short-circuit" as an electrical artifact, not an ordinary mute. **Gatecrashing mechanisms**: at the same instant, six mechanisms the seed had already rolled OFF for this piece (apocryphal tear, free intonation, tonal modulation, self-feedback, rungler, the strange element) each get a real, elevated 40% chance to turn on anyway — reusing the exact same "forced activation" idiom the phase-40 intensity guarantee already established, just triggered by a mid-piece rupture instead of a start-of-piece safety net.

### 4.11 Other ruptures and glitches

Fracture Folder + Rungler (phase 41), Leibniz Rectifier (phase 42), chaos contagion (phase 44), thermal fade (phase 45), Rössler collapse + self-epitaph (phase 46), stochastic mute (phase 47), the strange element (phase 50), a real foreign signal via host temperature/load (phase 51, the one honest exception to pure reproducibility), Q1.15 fixed-point overflow (phase 52), FFT/IFFT-based spectral ghost (phase 53), sample-hold glitch (phase 55), cross-domain contagion (an audio peak pushing the clock, phase 60), Parenthesis (reroute to a stutter loop, phase 70), chaos-addressed step reading (phase 64), and Curto-Circuito (section 4.10, phase 84) crowning the family.

---

### Current operational state (August 22, 2026)

The Studio now includes material agency, post-space audibility observation,
relational orchestration memory, expanded melodic/scale contracts, varied
percussion articulation and an auditable Wild Mode. Wild Mode is the Studio
default: real entropy reseeds the shared generator at bar boundaries. Turning
it off restores seed-based reproducibility when tempo, configuration, engine
version and relevant external state match. Recent same-seed normal/wild takes
had different SHA-256 hashes while retaining the same macro-material, confirming
that the seed is a starting point rather than a frozen song in Wild Mode.
The current verification baseline is CTest 19/19, normal regression 6/6 and
Wild Mode regression 3/3, all without clipping. The latest work also keeps the
snare's colour compensation local to its components, never to the master bus.

## 5. User flow

### 5.1 In the desktop app

1. Open the app: `cd rasgo-synth-app && ./rasgo_synth_app` — opens maximized.
2. Set the seed in the SEED field, or click ALEA to roll a new one.
3. Adjust the tempo on the TEMPO slider (20–220 bpm). The scale is no longer a manual control — the seed picks it.
4. Adjust the output volume on the MASTER slider (next to NÍVEL, a horizontal RMS/peak meter).
5. Check/adjust the OUTPUT DIRECTORY, TITLE and ARTIST (narrower now, sharing space with PESOS DE JOGO/game weights).
6. Click the round COMPOR button (an eighth-note ♪ icon, red) — rendering runs on a separate thread; the arc timeline lights up the active section, the waveform draws itself.
7. Click PLAY (a play/pause icon, gold) to hear the result, or click directly on the waveform (titled WAVEFORM) to play from that point.
8. Drag across the waveform to select a span — enables TOCAR TRECHO ("play span") and SEED DO TRECHO ("seed from span," deriving a new seed via an FNV-1a hash and immediately triggering a new composition).
9. During playback: the BRILHO AO VIVO ("live brightness") slider applies a real, live-adjustable low-pass filter; VARIAR SEED captures the exact instant and derives a new seed from it.
10. Below the waveform: ESPECTRO/SPECTRUM (Goertzel bank) and GRANULAÇÃO/GRANULATION (particles reacting to amplitude). PESOS DE JOGO/GAME WEIGHTS shows the current piece's four Alea/Agon/Ilinx/Mimicry colored bars. The module panel shows, in a grid, which of the engine's ~50 mechanisms actually manifested in this specific piece.
11. OSCILOSCÓPIO (ESTÉREO)/OSCILLOSCOPE (STEREO) (gold) and LOG (with COPIAR/LIMPAR below the box) split the bottom row, 60%/40% width.
12. TUTORIAL and ABOUT (top right corner) open themed dialogs with usage instructions and the project's real description.
13. MODO ALBUM/ALBUM MODE (a rectangular button, fills copper when on) swaps the panel to TEMA/FAIXAS/OUVIR (theme/tracks/listen).

### 5.2 Via the command line

```bash
# Standalone piece
./rasgo_integrated_demo [seed] [outDir] [bpm]

# Track set (album) -- game-weight trajectory, no longer a fixed scale
./rasgo_album_demo [albumSeed] [trackCount] [outDir] [theme]

# Regression suite
python3 regression_test.py --seeds 20

# App smoke test
./autotest.sh --skip-build --timeout=30
```

---

## 6. Work performed (phase-by-phase timeline)

The project's living document ([`Plano_de_Upgrade_Instrumento_Generativo.md`](../../Plano_de_Upgrade_Instrumento_Generativo.md), Portuguese only) is the complete source of truth, phase by phase, with the reason and the test for each one. Chronological summary:

- **Phases 1–23** — DSP core validated, sequencer, narrative layer via Markov chain, "Sinal" visual reskin (later replaced by "Cobre"/Copper, see section 9), eight percussion voices, real stereo output, initial album mode, audiophile-grade sound quality (dither/DC-block/limiter/PolyBLEP), all nine chaotic maps active, real circuit modules (Atari Punk, envelope-oscillator, Noise Cornucopia), Turing Machine character in the sequencer, real audio self-feedback, cross-FM, colored noise.
- **Phases 24–48** — real audible drums with per-seed mix jitter, apocryphal tear, real waveform interaction, album mode in the UI, tempo variation, initial functional harmony, live performance (filter + VARIAR AGORA), tonal-center modulation, modal modulation, real chord progressions, rhythmic humanization, per-voice percussion effects, hocketing, cross-influence network, free intonation, intensity guarantee (every piece tests itself against at least one risk mechanism), Fracture Folder + Rungler, Leibniz Rectifier, tuba voice, chaos contagion, thermal fade, Rössler collapse + self-epitaph, stochastic mute, and the scale expansion from 9 to 39 with the ESCALA control removed from the UI.
- **Phases 49–68** — Agon (four voices competing for power, with a "rusga"), the strange element (internal and literal via a real host read), fixed-point overflow, spectral ghost, companion .txt score, sample-hold glitch, expanded drum dynamics, real euclidean rhythm, Walsh oscillator, cross-domain contagion (audio→clock), Xenakis sieve-generated scale, real voice leading, polyphonic cascade, chaos-addressed step reading, Neo-Riemannian transformations, drums as a real instrument (from real listening to a full album), rhythmic archetypes with real identity, accelerando toward the climax.
- **Phases 69–76** — probability market (prices persisted between renders), Parenthesis (reroute to a stutter loop), Heisenberg (rhythmic/pitch precision inversely coupled), VOSIM, chaotic binaural beat, sidechain asphyxia envelope, self-oscillating Polivoks filter, Vector Synthesis (real 2D crossfade).
- **Phases 77–82** — Harmonàig (4-voice pad, credited to Jason Lim/Instruō), Xenakis sieve polyphony (4 channels), Bloom (fractal melody, credited to Andrew Ikenberry/Qu-Bit Electronix), Caillois game weights (phase 80, an explicit turning point), fixing a real sustained-instability bug in the tuba/wood `ModalResonator` (phase 81), deep rhythm research — two real aksak archetypes + a generated Xenakis rhythmic sieve (phase 82).
- **Phase 83** — the end of the album's forced tonal affinity, replaced by a straight-line trajectory across the Caillois game weights; each track freely picks its own scale.
- **Phase 84** — "Curto-Circuito" ("Short-Circuit"): a real hard cut (gate + noise spark) that, at the same instant, gives mechanisms the seed had left off a second chance to gatecrash the piece.
- **App layout redesign** (between phases 82 and 83) — visual identity changed from phosphor green ("Sinal") to copper; the author's real logo vectorized and applied as an emblem; the module panel expanded into a grid covering ~50 mechanisms; stereo oscilloscope; new ESPECTRO/NÍVEL/GRANULAÇÃO/PESOS DE JOGO panels (Goertzel banks, RMS/peak meter, particles, colored bars); BPM and MASTER as horizontal sliders; real icons for COMPOR/PLAY; themed ABOUT/TUTORIAL; the window opens maximized with a scroll fallback.

---

## 7. Validations

### 7.1 What exists

- **`regression_test.py`** — builds the engine, renders a batch of seeds, measures duration/peak/clipping fraction/section history. Criteria: duration 15–300s, clipping < 0.05%, non-empty history.
- **`autotest.sh`** — builds the app, runs with `RASGO_AUTOTEST=1`, confirms the "gravado:" line in the log and a clean native console.

### 7.2 What has actually been verified

Every phase since 13 has passed `regression_test.py` (batches of 15–20 seeds) with zero saturated samples, and the app was rebuilt and passed `autotest.sh` after each one. Beyond that, every new seed-gated mechanism since phase 49 received an **empirical firing-rate audit** (render a batch of seeds and confirm in the metadata that the mechanism actually fires at the expected rate, not just that it compiles) — a discipline that has already caught real bugs (an 11.6% collision rate in the pitch sieve before it was fixed, a gap counter in the wrong unit in Agon, the tuba's `ModalResonator` instability). Several phases were born directly from the user's **real listening**, not just measurement: the drums (phase 66), the tuba's audibility (phase 81), and the very waveform/spectrogram analysis that originated Curto-Circuito (phase 84).

### 7.3 What has not yet been verified

Not every possible combination of the engine's ~50 mechanisms has been listened to by hand — coverage comes from empirical metadata audits (confirming real firing) plus targeted listening guided by specific user reports, not an exhaustive sweep of every combination.

---

## 8. Security

- **Entirely local execution** — no network, telemetry or personal data collection in any engine or app component.
- **No credentials or sensitive data** anywhere in the code or build/test scripts.
- **Care around disk space** and `recordings/` — this is the user's real output directory (real albums, an accumulated index); test scripts write to isolated subfolders or clean up exactly the file they just created, never a broad sweep.
- **AGPL-3.0-or-later license** for current authorial code; full text in `LICENSE`.
- **Build with no elevated privileges** — compilation (direct g++) requires no sudo.

---

## 9. Layout — the "Cobre" (Copper) visual identity

The visual identity changed from "Sinal" (phosphor green over cool near-black) to **"Cobre"/"Copper"**: the accent color is an oxidized copper orange (`#e8865f`), chosen because it directly references the real physical object behind the name "rasgo" — acid-etched Eurorack panels the author has hand-built since 2013. The background remains near-black, now with a warm rather than cool bias.

- **Emblem** — the author's real logo (his own Inkscape vectorization) loaded via `juce::Drawable::createFromSVGFile`, tinted copper, applied next to the title.
- **Expanded palette** — `Palette::action` (gold, `#f2c14e`) for the primary action buttons (COMPOR/PLAY); `Palette::danger` (red) reused specifically as COMPOR's own color; a small copper/red/gold cycle (`vizAccent`) colors the bars of the new panels (game weights, per-band spectrum, L/R meters) instead of everything in monochrome copper.
- **Real icons** — COMPOR uses the Unicode eighth-note glyph (♪, more reliable than a hand-drawn path); PLAY uses a triangle/two bars depending on playback state.
- **Stereo oscilloscope** — two independent lanes (L/R), gold, a longer time window than before.
- **Typography** — monospaced throughout title, labels and log; sizes increased relative to the "Sinal" version.
- **Background texture** — an oscilloscope graticule grid behind the entire window.
- **Window** — opens maximized by default (`setFullScreen`), with a real scroll fallback (`juce::Viewport`) on monitors smaller than the comfortable layout (1100×760) requires.

**Reverted attempt**: a visual "tear" effect (torn paper crossing the control panel and/or the window's top) was implemented and then completely removed at the user's request — do not reintroduce without a new, explicit request.

---

## 10. Open items and next steps

See the full living list in `CONTINUIDADE.md` ("Tarefas em aberto," Portuguese only) and in the Plano de Upgrade ("Em aberto"). Summary of what remains genuinely open:

- **A "footnote" and a narrating voice** — the footnote mechanism exists; a richer literary/narrative voice remains open.
- **More physically articulated vocal synthesis** — Scat and Choir exist as original formant voices; a fuller vocal-tract subsystem remains a long-term frontier.
- **Expansion of relational roles** — theremin, flute FM, tuba, voices, polyphonies and archive still need deeper causal contracts beyond the current nuclei.
- **Real meter/time-signature changes** — connects with the rhythm research (phase 82), deliberately out of scope so far (would change the fixed 16-step rhythmic backbone).
- **The embedded console for the Performance Version** — a stage after the Studio Version, by the user's explicit decision.
- **Documentation synchronization** — the four public core documents are now synchronized in EN/FR/ES/PT; internal phase audits and the living plan remain Portuguese working documents.

---

## 11. References and influences

A vacuum does not explain Rasgo Synth; this is a generative synthesis project that honestly declares itself in direct dialogue with real circuits, commercial instruments, academic papers and the author's own trajectory. No source code from another instrument nor any exact electrical layout was copied: behaviors were originally implemented in C++ from public concepts and documentation. See `CREDITS.md` for the complete per-mechanism table.

### 11.1 Authorial trajectory and initial external reference

- **Antitotem and Rasgo's physical modules (2013–)** — works built by the author. Antitotem precedes Rasgo, was incorporated as its precursor and patron.
- **Kosmische (2013)** — Nicolas Steven Miller's generative Android instrument (github.com/zrnsm/kosmische, GPLv3), an initial technical and conceptual reference, not part of Rasgo.

### 11.2 Commercial hardware and publicly documented DIY circuits

- **Roland TR-808** — sonic reference for part of the eight percussion voices.
- **Music Thing Modular — Turing Machine** (Tom Whitwell) — the sequencer's lock/step-mutation character.
- **Mutable Instruments** (Émilie Gillet) — Rings, Clouds, Branches, reinterpreted.
- **Instruō — ochd** (Ben "DivKid" Wilson) and **Harmonàig** (Jason Lim) — the LFO bank and the 4-voice chord pad, respectively.
- **Qu-Bit Electronix — Bloom** (Andrew Ikenberry) — the fractal Trunk/Branch/Path melody.
- **Frap Tools — Sapèl** — four colors of noise.
- **Make Noise — Maths** / no-input mixing — real audio self-feedback.
- **Buchla** — leveler/lag processors.
- **Rob Hordijk — Benjolin** — bidirectional cross-FM.
- **Ken Stone — CGS22 "Master Divider"** — the XOR polyrhythmic clock divider.
- **Forrest Mims III — Stepped Tone Generator** ("Atari Punk Console") — stepped/glitch texture.
- **Ray Wilson — MFOS "Noise Cornucopia"** — grainy noise and random gate.
- **General Instrument AY-3-8910** — chiptune arpeggio/texture.
- **Pantala Labs** — lo-fi delay mode, tape/tube saturators.
- **Robert Moog; Tim Stilson and Julius O. Smith** — the ladder filter.
- **Hal Chamberlin** — the state-variable filter.
- **Polivoks (Formanta, USSR)** — the third filter color, real self-oscillation.
- **Rob Hordijk/Benjolin, Xaoc Devices Leibniz-Rectifier** — Rungler and bit rectification/corruption.
- **Sequential Circuits Prophet VS** — real 2D crossfade (Vector Synthesis).
- **Sidechain/ducking technique from techno/EDM production** — the asphyxia envelope.

### 11.3 Mathematics, computer science and academic synthesis

- **Iannis Xenakis** — Dynamic Stochastic Synthesis ("Gendyn"); and, in *Formalized Music* (chapter XI, "Sieves: A User's Guide"), the modulus/residue congruence sieve theory, applied both to pitch classes (phase 61) and points in time (phase 82).
- **Joe Pritchard — *The Chaos Cookbook* (1992)**; **Lance Putnam — SuperCollider's Chaos UGens (2004)** — research references for the nine chaotic maps.
- **Vesa Välimäki and Antti Huovilainen — PolyBLEP** — the oscillators' anti-aliasing.
- **TPDF dither** — a standard digital audio processing technique.
- **Werner Kaegi and Stan Tempelaars, Institute of Sonology, Utrecht — VOSIM** — synthesis via decaying sin² pips.
- **Constantin Brăiloiu; Stoyan Dzhudzhev** — the theory of Balkan additive/aksak rhythm, two real archetypes (9+7 and 7+7+2).
- **David Lewin; Neo-Riemannian theory** — P/L/R triadic transformations.

### 11.4 Narrative, ludological and compositional structures

- **Gustav Freytag — Freytag's Pyramid** (1863) — the "Dramatic" narrative grammar.
- **Kishōtenketsu** — the 4-part Chinese/Japanese narrative structure (qǐ-chéng-zhuǎn-hé).
- **Roger Caillois — *Man, Play and Games*** (*Les Jeux et les Hommes*, 1958) — the four categories of play (Alea, Agon, Ilinx, Mimicry), mechanically formalized as continuous weights biasing eleven existing mechanisms (phase 80).

Whoever wants to dig deeper into any of these starting points before porting the next idea should keep the same standard of honesty: cite the real source, understand the behavior before reimplementing, and make clear in this section (or in the corresponding phase of `Plano_de_Upgrade_Instrumento_Generativo.md`) that the idea is in dialogue with someone else's work, not that it was born in isolation.
