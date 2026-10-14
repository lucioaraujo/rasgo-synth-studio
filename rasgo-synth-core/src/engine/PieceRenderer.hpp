// Rasgo Synth Core -- shared rendering engine.
//
// This is the phase-4 "integrated piece" patch (diode-mixed voice, ring
// modulation fading with the arc's resonance, Buchla-style stepped CV on
// delay/reverb) pulled out of the standalone CLI demo so both the CLI demo
// and the GUI app can drive the exact same engine. Nothing about the DSP
// changed -- it's a straight extraction into a callable function with an
// optional progress callback, so a UI can show what's happening (current
// section, playhead position) while the render runs on a background thread.
//
// Phase 5 folded in two of the three later circuit-translation modules
// (multiband filter bank, morph oscillator) that had so far only lived in
// isolated demos: osc2's pulse-width modulation became a full continuous
// waveform morph, and the Buchla-style stepped source now also drives a
// 4-band spectral bloom that fades in with the arc's resonance, using the
// two stepped outputs that used to go unused (only 2 of the 4 correlated
// outputs were previously in play, for delay time / reverb mix).
//
// Phase 7 brings back a piece of the original Kosmische engine itself: its
// SuperCollider patch defined four complete LFOs, but the Android UI built
// on top of it only ever exposed two, leaving half the modulation matrix
// dead code inside the compiled binary (see Kosmische_Pericia_Tecnica.md).
// All four now run here, each aimed at one of the same modulation targets
// the original patch offered -- pitch drift, osc2's shape/width, filter
// cutoff, delay time -- as a slow breathing layer underneath the existing
// audio-rate chaos modulation.
//
// Phase 10 adds three ideas surveyed from the Lunetta CMOS-logic synth
// scene -- the folk tradition this project's own hardware precursors
// (Antitotem, the original Rasgo Synth) belong to -- that were missing
// pieces rather than duplicates of what was already here: a CGS22-style
// XOR clock divider (PolyrhythmDivider) driving a percussion layer
// (DrumVoices: kick/snare/hi-hat) this engine never had at all, and a
// second filter colour (the legacy-named SallenKeyFilter, a Chamberlin
// state-variable filter) crossfaded
// against the existing 4-pole MoogFilter ladder using GbmanMap -- one of
// the six independently implemented chaotic maps added in phase 8 but left unwired into the
// actual patch until now. Phase 10 also added KSRandomizerCore's
// probability-bias idea, driving the melodic sequencer's octave choice.
//
// Phase 12 closes out the two synthesis engines that were the last
// unfinished piece of Kosmische's own inheritance: its native binary
// carried GendynUGens (Xenakis's Dynamic Stochastic Synthesis) and AY_UGen
// (an AY-3-8910 chip model) compiled in, but kosmische.sc's actual patch
// never used either -- see Kosmische_Pericia_Tecnica.md. GendynOscillator
// runs as a continuous background texture (a self-mutating breakpoint
// waveform that never repeats and never settles, a genuinely different
// synthesis principle from anything else in this engine), more present
// when a section thins out, the same density-driven logic GranularCloud
// already uses. AY8910Chip runs as a chiptune-flavoured arpeggio layer
// tuned to the current chord tone, fading in with the arc's resonance
// alongside the ring modulator and multiband bloom.

#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "RenderProgress.hpp"
#include "DurationTarget.hpp"

#include "../dsp/Oscillator.hpp"
#include "../dsp/ChaosMaps.hpp"
#include "../dsp/MoogFilter.hpp"
#include "../dsp/Envelope.hpp"
#include "../dsp/Effects.hpp"
#include "../dsp/DiodeShaper.hpp"
#include "../dsp/RingModulator.hpp"
#include "../dsp/BuchlaRandomSource.hpp"
#include "../dsp/MultibandFilter.hpp"
#include "../dsp/MorphOscillator.hpp"
#include "../dsp/LfoBank.hpp"
#include "../dsp/SignalLeveler.hpp"
#include "../dsp/LagProcessor.hpp"
#include "../dsp/ModalResonator.hpp"
#include "../dsp/GranularCloud.hpp"
#include "../dsp/PolyrhythmDivider.hpp"
#include "../dsp/SallenKeyFilter.hpp"
#include "../dsp/PolivoksFilter.hpp"
#include "../dsp/PolivoksVCA.hpp"
#include "../dsp/AcidBasslineVoice.hpp"
#include "../dsp/BassPhraseTimbre.hpp"
#include "GrooveDialogue.hpp"
#include "../dsp/EnvelopeLoopPulse.hpp"
#include "../dsp/FootnotePluck.hpp"
#include "../dsp/GaloVoice.hpp"
#include "../dsp/ScatVoice.hpp"
#include "../dsp/ChoirEnsemble.hpp"
#include "../dsp/DejaVuVoice.hpp"
#include "../dsp/DrumVoices.hpp"
#include "../dsp/GendynOscillator.hpp"
#include "../dsp/AtariPunkConsole.hpp"
#include "../dsp/EnvelopeOscillator.hpp"
#include "../dsp/TapeSaturator.hpp"
#include "../dsp/TubeSaturator.hpp"
#include "../dsp/GrainyNoise.hpp"
#include "../dsp/NoiseGateGenerator.hpp"
#include "RasgarTimingMemory.hpp"
#include "RasgarAccentField.hpp"
#include "RasgarPercussionArticulation.hpp"
#include "RasgarGateWindow.hpp"
#include "PerformanceActorRegistry.hpp"
#include "PerformanceGesturePlans.hpp"
#include "RasgoTerritoryManifest.hpp"
#include "BassBodyAdapters.hpp"
#include "BassExecutionFeedback.hpp"
#include "BassKickInteraction.hpp"
#include "../dsp/ColoredNoise.hpp"
#include "../dsp/OchdBank.hpp"
#include "../dsp/BernoulliGate.hpp"
#include "../dsp/ApocryphalTear.hpp"
#include "../dsp/AY8910Chip.hpp"
#include "../dsp/DcBlocker.hpp"
#include "../dsp/LookaheadLimiter.hpp"
#include "../dsp/DeliveryPeakGuard.hpp"
#include "../dsp/LinkedOverloadProtector.hpp"
#include "../dsp/FractureFolder.hpp"
#include "../dsp/Rungler.hpp"
#include "../dsp/LeibnizRectifier.hpp"
#include "../dsp/BreathExciter.hpp"
#include "../dsp/RosslerMap.hpp"
#include "../dsp/FixedPointOverflow.hpp"
#include "../dsp/SpectralGhost.hpp"
#include "../dsp/SampleHoldGlitch.hpp"
#include "../dsp/ParenthesisGlitch.hpp"
#include "../dsp/WalshOscillator.hpp"
#include "../dsp/VosimOscillator.hpp"
#include "../dsp/HardSyncVoice.hpp"
#include "ForeignSignal.hpp"
#include "DrumArchetypes.hpp"
#include "RhythmSieve.hpp"
#include "RhythmGenerator.hpp"
#include "../dsp/ThereminVoice.hpp"
#include "../dsp/MontageEditCut.hpp"
#include "../dsp/AtropeloEnsemble.hpp"
#include "../dsp/FmFluteVoice.hpp"
#include "../dsp/MelodyVoicingBank.hpp"
#include "../dsp/DissonanceTreatment.hpp"
#include "ProbabilityMarket.hpp"
#include "CompositionIdentity.hpp"
#include "SeedLineage.hpp"
#include "GamePlan.hpp"
#include "MaterialAgency.hpp"
#include "IntermodularOrchestra.hpp"
#include "OrganismStemAuditor.hpp"
#include "OrganismAudibility.hpp"
#include "OrchestralRole.hpp"
#include "OrchestralSectionMemory.hpp"
#include "ModuleLifeState.hpp"
#include "RelationalChamber.hpp"
#include "ChamberResponse.hpp"
#include "NarrativeAttentionEngine.hpp"
#include "NarrativeMixDirective.hpp"
#include "RasgarGesture.hpp"
#include "ExpressiveEventLedger.hpp"
#include "../sequencer/StepSequencer.hpp"
#include "../sequencer/SieveScale.hpp"
#include "../sequencer/NarrativeArc.hpp"
#include "../sequencer/GenerativeArc.hpp"
#include "../sequencer/HarmonicWanderer.hpp"
#include "MelodicInterpreter.hpp"
#include "MaterialExpressionField.hpp"
#include "SpatialExpressionField.hpp"
#include "ExpressiveInterpretation.hpp"
#include "MusicalDirection.hpp"
#include "MusicalRelations.hpp"
#include "ModuleSelection.hpp"
#include "KitInterpretation.hpp"
#include "../sequencer/MicrotonalTuning.hpp"
#include "../dsp/SpectralObject.hpp"
#include "../sequencer/PolymeterEnsemble.hpp"
#include "../sequencer/BluffSignal.hpp"
#include "../dsp/RasgoFracture.hpp"
#include "../sequencer/CoherenceCollapse.hpp"
#include "../io/Recorder.hpp"
#include "../io/PublishedScore.hpp"

namespace rasgo {

struct PieceConfig {
    // The one input here that isn't tuned, only rolled -- every other
    // "seed-derived" comment in this file traces back to this single
    // number. In this project's own vocabulary that's the rasgo: not a
    // knob, the originary rupture the whole piece's trajectory branches
    // from.
    unsigned seed = 2026u;
    std::string outDir = "recordings";
    // Album calibration is an in-memory render, not a disposable recording.
    // False skips WAV, index, score and persisted probability-market updates.
    bool recordOutput = true;
    // Frozen album calibration/final input; ordinary pieces still load history.
    bool probabilityMarketOverride = false;
    ProbabilityMarket probabilityMarketInput;
    double bpm = 100.0;
    // Duration is no longer a target you set -- see GenerativeArc.hpp
    // (phase 13): the piece ends when the Markov walk over structural roles
    // reaches its coda, an emergent property of the seed, same as
    // everything else this engine decides rather than takes as a dial.
    // This is only the runaway-prevention ceiling for pathological seeds
    // that keep failing to roll a coda naturally.
    double maxDurationSeconds = 240.0;
    double targetDurationSeconds = 0.0; // opt-in album narrative sizing; zero is unchanged
    Scale scale = Scale::HarmonicMinor;
    int rootMidiNote = 57;
    // Standalone composition derives key centre and register from the seed.
    // Albums/mashups can retain an intentional centre by enabling this.
    bool rootOverride = false;

    // Phase 18 (AlbumRenderer.hpp): scales the energy the generative arc
    // rolls for every section, applied before clamping -- 1.0 is
    // unchanged (the seed alone decides how far a climax reaches, same as
    // always), <1.0 caps how intense the whole piece is allowed to get
    // (a somber/restrained theme), >1.0 pushes toward more extremes. A
    // single piece rendered on its own has no reason to touch this; it
    // exists for AlbumConfig's themed presets to bias a whole set of
    // tracks toward a mood without hand-tuning every section.
    double energyBias = 1.0;

    // Optional album context -- set by renderAlbum() so a track's own
    // metadata records which album it belongs to and its place in it.
    // -1 means "not part of an album" (an ordinary standalone render).
    long long albumId = -1;
    int albumTrackIndex = -1;
    int albumTrackCount = -1;
    std::string albumTheme;

    // Phase 19 (album mastering pass, AlbumRenderer.hpp): a final output
    // gain trim applied after the limiter but still inside the safety
    // saturator, so it can never reintroduce clipping regardless of value.
    // 1.0 (default) is exactly the old behaviour. AlbumRenderer uses this
    // to gently pull outlier tracks' loudness toward an album's own
    // average -- a relative *alignment*, not normalizing every track to
    // identical loudness, which would flatten the very dynamic variation
    // between a quiet piece and an intense one that's the point of this
    // engine.
    double outputGainTrim = 1.0;

    // User-set overrides for the WAV's own title/artist fields -- empty
    // means "use the default" (a plain description for a standalone
    // piece, or "{albumTheme} {trackIndex+1}" for an album track, set by
    // renderAlbum()/AlbumRenderer.hpp when titleOverride is still empty at
    // that point). artistOverride is the one field the app persists
    // across sessions as a stored preset (see MainComponent.h) -- sensible
    // since an artist name doesn't usually change take to take, project to
    // project, the way a title does.
    std::string titleOverride;
    std::string artistOverride;

    // Fase 83 -- album cohesion via a Caillois game-weight trajectory
    // (AlbumRenderer.hpp) instead of a shared scale: when set, the four
    // gameWeight* fields below replace the per-piece stick-breaking roll
    // (see gameCuts in renderIntegratedPiece) so an album's tracks can
    // walk from one dominant game to another across the set, each track
    // still choosing its own scale independently. Unused (false) by a
    // standalone piece, which keeps rolling its own bespoke blend.
    bool gameWeightOverride = false;
    double gameWeightAleaOverride = 0.25;
    double gameWeightAgonOverride = 0.25;
    double gameWeightIlinxOverride = 0.25;
    double gameWeightMimicryOverride = 0.25;

    // Fase 100 -- mashup/recombinacao estrutural de seeds: same idea as
    // gameWeightOverride just above, extended to the scale roll. Since
    // fase 48 removed the ESCALA UI box, the scale a standalone piece
    // gets has been entirely seed-derived (see initialScale below) --
    // this is the first way to force a specific one instead, so a piece
    // can genuinely inherit another piece's own scale rather than only
    // its game-weight bias.
    bool scaleOverride = false;
    Scale scaleOverrideValue = Scale::HarmonicMinor;

    // Phase 164 -- explicit seed lineage. Unlike scaleOverride, this carries
    // the complete stable identity-domain recombination. It does not force
    // optional mechanisms; the fresh child seed still decides their detailed
    // trajectory. Metadata makes ancestry and mutation auditable.
    bool compositionIdentityOverride = false;
    CompositionIdentity compositionIdentityOverrideValue{};
    bool seedLineageEnabled = false;
    unsigned seedLineageParentA = 0;
    unsigned seedLineageParentB = 0;
    unsigned seedLineageGeneration = 1;
    double seedLineageParentBWeight = 0.5;
    double seedLineageMutationProbability = 0.0;
    std::uint16_t seedLineageInheritedBMask = 0;
    std::uint16_t seedLineageBlendedMask = 0;
    std::uint16_t seedLineageMutationMask = 0;

    // Fase 115 -- jogo aplicado alem do motor, tambem no album (task #6,
    // pedido explicito do usuario: "jogo nao somente no motor, pode ser
    // aplicado de diversas maneiras no instrumento"). Mesmo idioma de
    // gameWeightOverride acima: quando setado, a taxa de veracidade do
    // sinalizador de blefe (BluffSignal.hpp) desta peca vem de um ponto
    // interpolado numa trajetoria real album-wide (AlbumRenderer.hpp) em
    // vez de sorteada isoladamente por peca -- um album pode ter seu
    // proprio arco real de confiabilidade (comeca confiavel, termina
    // blefando descaradamente, ou o inverso), nao so um blend de pesos de
    // jogo compartilhado. Unused (false) por uma peca avulsa, que sorteia
    // sua propria taxa independente.
    bool bluffTruthfulnessOverride = false;
    double bluffTruthfulnessOverrideValue = 0.7;

    // "Modo selvagem" -- an explicit, opt-in break from this whole
    // engine's central promise ("mesma seed produz peca identica"). Off
    // by default, and every mechanism in this file already handles
    // whatever unit(rng) hands it safely by construction (that's the
    // existing design, not something this mode has to work around) --
    // so real, non-reproducible hardware entropy can safely reseed the
    // shared rng stream periodically (see the wildMode block in
    // renderIntegratedPiece) without touching any individual mechanism's
    // own code. A piece rendered with this on is honestly exposed as
    // such in its own metadata (wildMode/wildReseedCount) -- the same
    // disclosure discipline as the foreign-signal exception (fase 51),
    // just chosen deliberately instead of drawn rarely by the seed.
    bool wildMode = false;

    // First Orquestra Relacional RASGO increment. Enabled by default for
    // phase-121 renders, but explicitly switchable for same-seed
    // counterfactual A/B: false leaves the pre-existing theremin/rhythm/
    // harmony paths untouched and creates no relational topology regions.
    bool relationalOrchestraEnabled = true;

    // Second relational nucleus. It requires the explicit stem audit so the
    // chamber reacts to measured bodies rather than availability flags.
    bool relationalChamberEnabled = true;

    // Phase 123 relational material agency. Disabled is an exactly neutral
    // family-gain counterfactual; organism-level calibration remains a
    // separate phase-122 switch so each intervention can be heard alone.
    bool materialAgencyEnabled = true;

    // Expensive, explicit offline audit. Duplicates the linear delay/reverb
    // path per organism and measures post-space survival without changing
    // the audible master. Ordinary renders keep it off.
    bool organismStemAuditEnabled = false;

    // First observational stage of narrative mixing. It requires the section
    // stem snapshot. Attention remains separately switchable so the first
    // bounded actuation can retain an honest same-seed counterfactual.
    bool narrativeAttentionEnabled = true;

    // Phase 165 -- modest, section-boundary narrative actuation. It has no
    // effect without the stem audit and attention evidence; the directive can
    // only invite one manifested desk through OrganismAudibility smoothing.
    bool narrativeMixEnabled = true;

    // Phase 167 -- read-only event provenance. Enabled by default because it
    // cannot touch PCM; false exists solely to prove neutral observation in a
    // same-seed counterfactual and to measure its offline bookkeeping cost.
    bool expressiveEventLedgerEnabled = true;

    // Phase 168 -- opt-in A/B actuation. The expressive timing/accent
    // adapters remain inert by default so observational and legacy renders
    // stay comparable until an explicit counterfactual is requested.
    bool expressiveActuationEnabled = false;
    bool musicalDirectionEnabled = false;
    bool musicalRelationsEnabled = true; // only effective with musical direction; API counterfactual
    // Experimental core-sub spectral articulation; the existing Direction +
    // Expressive switches gate it. False isolates it in same-seed API A/B.
    bool bassPhraseTimbreEnabled = true;
    // Observed execution updates the next bass-cycle interpreter state.
    // Only with Expressive + Direction; false preserves the preceding PCM.
    bool bassExecutionFeedbackEnabled = true;
    // Brief causal core-sub reduction from accepted kick dispatches instead
    // of permanent per-note duck. False preserves the preceding implementation.
    bool bassKickInteractionEnabled = true;
    // Original rhythmic families, gated by Expressive + Musical Direction.
    // False keeps the preceding rhythmic implementation for API A/B.
    bool grooveDialogueEnabled = true;
    // False retains GrooveDialogue v1 rhythms without within-step responses.
    bool grooveSubdivisionsEnabled = true;
    ModuleSelection moduleSelection;

    // Phase 122 mobile desk ecology. It repairs measured inter-module scale
    // disparities and moves each active desk independently at section
    // boundaries. The switch exists for honest same-seed counterfactuals.
    bool organismAudibilityEnabled = true;
};

struct PieceProgress {
    double t;                 // exact generated frames / planned composition frames
    double seconds;
    ArcState arcState;
    float ringAmount;
    std::uint64_t completedFrames = 0;
    std::uint64_t totalFrames = 0;
    PieceRenderPhase phase = PieceRenderPhase::Composition;
};

// Every seed-derived "is this feature present in this piece" choice,
// mirroring exactly what already goes into the WAV's own metadata JSON --
// exposed as real fields (not something a UI has to re-parse back out of a
// JSON string embedded in an audio file) so an app can show a live status
// panel of which modules are actually active in the current piece.
struct PieceModuleInfo {
    // Phase 118 -- decisions above individual modules. These are the
    // perceptual identity of the piece, not another set of sound layers.
    std::string materialFocus;
    std::string scaleFamily;
    std::string coreVoiceModel;
    std::string sequenceRhythmModel;
    std::string leadVoiceModel;
    std::string harmonicLanguage;
    std::string ensembleModel;
    CompositionIdentity compositionIdentityValue{};
    bool seedLineage = false;
    unsigned seedLineageParentA = 0;
    unsigned seedLineageParentB = 0;
    unsigned seedLineageGeneration = 0;
    double seedLineageParentBWeight = 0.0;
    double seedLineageMutationProbability = 0.0;
    std::uint16_t seedLineageInheritedBMask = 0;
    std::uint16_t seedLineageBlendedMask = 0;
    std::uint16_t seedLineageMutationMask = 0;
    std::string gameStrategy;
    std::string gameRule;
    double gameEnergy = 0.0;
    double gameMemory = 0.0;
    double gameRisk = 0.0;
    double gameFreedom = 0.0;
    double gameHiddenInformation = 0.0;
    std::string rasgarMotion;
    std::string rasgarFinalPhase;
    int rasgarTearCount = 0;
    int rasgarUnstableBars = 0;
    double rasgarMaxInstability = 0.0;
    double rasgarMeanPulseDeviation = 0.0;
    int topologyRegionCount = 0;
    int topologyPassageCount = 0;
    int topologyTraversalCount = 0;
    int topologyRerouteCount = 0;
    int topologySplitCount = 0;
    int topologyArchivePageCount = 0;
    int archiveReadCount = 0;
    int archiveRereadCount = 0;
    int archiveReinterpretCount = 0;
    int archiveRecreateCount = 0;
    int topologyHarmonicShiftCount = 0;
    std::string agencyInitialForeground;
    std::string agencyFinalForeground;
    int agencyForegroundChangeCount = 0;
    int agencyDispositionChangeCount = 0;
    int agencyLeaderlessSectionCount = 0;
    int agencyPluralSectionCount = 0;
    int agencyObservedSectionCount = 0;
    int agencyRelationalReactionCount = 0;
    std::array<double, static_cast<std::size_t>(AgencyFamily::Count)> agencyRms{};
    std::array<double, static_cast<std::size_t>(AgencyFamily::Count)> agencyPeak{};
    std::array<double, static_cast<std::size_t>(AgencyFamily::Count)> agencyAudibleRatio{};
    bool relationalOrchestra = false;
    std::string relationalVerb;
    int relationalRhythmObservationCount = 0;
    int relationalThereminResponseCount = 0;
    int relationalHarmonyResponseCount = 0;
    int relationalHarmonyReconfigurationCount = 0;
    int relationalRhythmFeedbackCount = 0;
    int relationalResistanceCount = 0;
    bool relationalChamber = false;
    bool relationalChamberAuditReady = false;
    int relationalChamberRelationCount = 0;
    int relationalChamberConsequenceCount = 0;
    int relationalChamberDecisionCount = 0;
    int relationalChamberCiteCount = 0;
    int relationalChamberResistCount = 0;
    int relationalChamberDeformCount = 0;
    int relationalChamberScarCount = 0;
    int relationalChamberSpatialCrossingCount = 0;
    int relationalChamberLastManifestedBodies = 0;
    double relationalChamberFluteGain = 1.0;
    double relationalChamberTubaPressure = 1.0;
    double relationalChamberVoiceGain = 1.0;
    double relationalChamberSpatialPolarity = 0.0;
    bool relationalChamberResponseValid = true;
    std::string relationalChamberLastAction;
    int relationalThereminPresentBarCount = 0;
    int relationalThereminSilentBarCount = 0;
    int relationalThereminEntryCount = 0;
    int relationalThereminWithdrawalCount = 0;
    double relationalMemory = 0.0;
    double relationalResistance = 0.0;
    double relationalThereminPresence = 1.0;
    double relationalThereminTargetMidi = 0.0;
    int relationalThereminPitchChangeCount = 0;
    int relationalThereminRepeatedPitchCount = 0;
    int relationalThereminLongestRepeatedBars = 0;
    int relationalThereminPitchMin = 0;
    int relationalThereminPitchMax = 0;
    bool organismStemAudit = false;
    double organismStemMaxReconstructionError = 0.0;
    std::array<OrganismStemMetrics,
        static_cast<std::size_t>(OrganismStem::Count)> organismStemMetrics{};
    // Conservative life evidence. With stem audit disabled, only availability
    // is reported; audibility is never inferred from configuration alone.
    std::array<std::uint8_t,
        static_cast<std::size_t>(OrganismStem::Count)> organismLifeMasks{};
    bool narrativeAttention = false;
    int narrativeAttentionObservedSectionCount = 0;
    int narrativeAttentionFocusChangeCount = 0;
    int narrativeAttentionEventFocusCount = 0;
    int narrativeAttentionManifestedBodies = 0;
    double narrativeAttentionLastValue = 0.0;
    double narrativeAttentionMaximumValue = 0.0;
    std::string narrativeAttentionFinalFocus;
    std::string narrativeAttentionFinalReason;
    bool narrativeMix = false;
    int narrativeMixDirectiveCount = 0;
    int narrativeMixHysteresisHoldCount = 0;
    int narrativeMixEventDirectiveCount = 0;
    double narrativeMixLastGainDb = 0.0;
    double narrativeMixMaximumGainDb = 0.0;
    std::string narrativeMixFinalFocus;
    std::string narrativeMixFinalReason;
    ExpressiveEventSummary expressiveEventSummary{};
    bool expressiveActuation = false;
    int expressiveActuationEventCount = 0;
    int performanceActorCount = 0;
    int performanceHitActorCount = 0;
    int performanceTextureActorCount = 0;
    int performanceToneActorCount = 0;
    std::array<int, static_cast<std::size_t>(PerformanceGestureFamily::Count)>
        performanceActorFamilyCounts{};
    // Read-only Studio -> Performance hand-off. It contains the actors this
    // render actually manifested, never a speculative catalog or DSP state.
    RasgoTerritoryManifest performanceTerritoryManifest{};
    bool organismAudibility = false;
    int organismAudibilityMovementCount = 0;
    int organismAudibilityEmergenceCount = 0;
    int organismAudibilityYieldCount = 0;
    int organismSpectralRoleResponseCount = 0;
    std::array<double, static_cast<std::size_t>(OrganismStem::Count)>
        organismAudibilityMinGain{};
    std::array<double, static_cast<std::size_t>(OrganismStem::Count)>
        organismAudibilityMaxGain{};
    std::array<OrchestralProfile,
        static_cast<std::size_t>(OrganismStem::Count)> orchestralProfiles{};
    int initialRootMidiNote = 57;
    int spectralTechniqueCount = 0;
    double inputTruePeakDbtp = -120.0;
    double preDeliveryTruePeakDbtp = -120.0;
    double outputTruePeakDbtp = -120.0;
    double deliveryTruePeakTrimDb = 0.0;
    double preDeliveryGuardDbtp = -120.0;
    double deliveryGuardProjectedDbtp = -120.0;
    double limiterMaxGainReductionDb = 0.0;
    double overloadMaxGainReductionDb = 0.0;
    std::size_t overloadFrameCount = 0;
    int outputNonFiniteCount = 0;
    // Fase 80 -- Caillois game weights (Alea/Agon/Ilinx/Mimicry), see
    // gameWeightAlea/Agon/Ilinx/Mimicry in renderIntegratedPiece.
    double gameWeightAlea = 0.0;
    double gameWeightAgon = 0.0;
    double gameWeightIlinx = 0.0;
    double gameWeightMimicry = 0.0;
    std::string narrativeTemplate;
    std::string scale;
    // Fase 100 -- mashup/recombinacao: the raw Scale enum this piece's own
    // initialScale actually rolled to, alongside the human-readable
    // description above -- reading a scale back OUT of a rendered piece
    // (to inherit it into a new one) needs the real enum value, not a
    // string that can describe a secondary-scale blend or a generated
    // sieve scale and isn't reliably parseable back into one.
    Scale initialScaleValue = Scale::HarmonicMinor;
    std::string voiceColor;   // "tape" / "tube" / "fracture" / "none"
    bool loFiDelay = false;
    bool staticPop = false;
    std::string noiseColor;   // "white" / "pink" / "red" / "blue"
    int noiseColorChangeCount = 0;
    bool granularTexture = true;
    bool gendynTexture = true;
    bool coloredNoiseBed = true;
    float noiseClimateMin = 0.0f;
    float noiseClimateMax = 0.0f;
    float noiseClimateMean = 0.0f;
    // Percussion noise identities are separate from the continuous bed above;
    // exposing them makes aural variation auditable instead of hiding it in
    // the filtered voice implementation.
    std::string snareNoiseColor;
    std::string clapNoiseColor;
    std::string maracasNoiseColor;
    bool selfFeedback = false;
    double crossFmDepth = 0.0;
    double sequenceLockAmount = 0.0;
    double sequenceJumpiness = 0.0;
    int melodicPitchChangeCount = 0;
    int melodicRepeatedPitchCount = 0;
    int melodicLongestRepeatedSteps = 0;
    int melodicPitchMin = 0;
    int melodicPitchMax = 0;
    int fmFlutePitchChangeCount = 0;
    int fmFluteRepeatedPitchCount = 0;
    int fmFluteLongestRepeatedNotes = 0;
    int fmFlutePitchMin = 0;
    int fmFlutePitchMax = 0;
    // Fase 133 -- phrase-level melodic contour intent. These are telemetry
    // values for the realized seed path, not user controls or extra notes.
    int melodicContourDirection = 0;
    int melodicContourApexBar = 0;
    bool ochdBreathing = false;
    std::string hihatBranchMode; // "toggle" / "bernoulli"
    double hihatOpenProbability = 0.0;
    double drumsPresence = 0.0;
    bool drumsPresenceFloorApplied = false;
    bool drumsProtagonist = false;
    double drumsPeak = 0.0;
    double drumsRms = 0.0;
    double drumsAudibleFrameRatio = 0.0;
    // Fase 66: bateria como instrumento de verdade -- ver drumsEnabled em
    // renderIntegratedPiece.
    bool drumsEnabled = true;
    int drumHitFailureCount = 0;
    // Fase 67: real rhythmic identity (Marcha/Soul/Metal/Jazz/Samba-
    // inspirado) -- see DrumArchetypes.hpp.
    bool drumArchetype = false;
    std::string drumArchetypeName;
    int drumArchetypeChangeCount = 0;
    // Fase 82 -- Xenakis rhythmic sieve, an extra generated-rhythm slot
    // alongside the fixed archetype catalog above -- see RhythmSieve.hpp.
    bool rhythmSieveUsed = false;
    // Fase 85 stage 1 -- two more generated-rhythm slots, generalizing real
    // theory (aksak long/short grouping, Euclidean onset distribution)
    // beyond the fixed cells already in the catalog -- see
    // RhythmGenerator.hpp.
    bool rhythmGenAksakUsed = false;
    bool rhythmGenEuclidUsed = false;
    // Fase 85 stage 2 -- real per-hit dynamics, phrase-level gesture and
    // repetition-with-difference (see RhythmGenerator.hpp/hitVelocity).
    // ghostHihatCount is the one directly-countable, checkable side effect
    // (continuous velocity itself isn't a boolean to audit the same way).
    int ghostHihatCount = 0;
    // Per-hit percussion timbre drift is intentionally a piece-level choice
    // with bounded depth; expose it so a quiet/unchanging take can be
    // distinguished from a render where the feature was not armed.
    bool percussionTimbreVariation = false;
    double percussionTimbreDepth = 0.0;
    bool snareWhiteVariationCompensation = false;
    bool snareColorBalanceCompensation = false;
    // Fase 85 stage 3 -- H/G: how many times orchestration genuinely
    // reshuffled which percussion voices are "in" this section, and how
    // many times Curto-Circuito's aftermath forced the full kit back in.
    int orchestrationChangeCount = 0;
    int orchestralMemorySectionCount = 0;
    int orchestralHighConflictCount = 0;
    int orchestralRepeatedLeaderCount = 0;
    int orchestralRoleResponseCount = 0;
    std::string orchestralLastLeader;
    int postGlitchFullKitEvents = 0;
    // Fase 89 -- real meter change: a section's bar length can differ from
    // the engine's default 16 steps (see RhythmGenerator.hpp/StepSequencer
    // ::setNumSteps). meterChangeCount counts genuine changes; finalBarLength
    // is whichever length the piece ended on.
    bool meterVariance = false;
    int meterChangeCount = 0;
    int finalBarLength = 16;
    bool apocryphalTear = false;
    double apocryphalSourceSeconds = 0.0;
    // Voz de theremin: glide-only continuous sine. In the relational
    // orchestra it interprets the heard drum gesture on a bar-scale pitch
    // trajectory; in the counterfactual/legacy path it remains a shadow of
    // the melody's pitch -- see ThereminVoice.hpp.
    bool thereminVoice = false;
    // "Corte de montagem": a real jump cut to a distant past moment,
    // played once clean and unprocessed, both edges a true zero-crossing
    // hard cut -- see MontageEditCut.hpp.
    bool montageCut = false;
    double montageCutSourceSeconds = 0.0;
    // "Atropelo": competing, interrupting almost-voices with real memory
    // of an unfinished phrase (may resume later, may be genuinely
    // forgotten) that specifically invade stochastic mute's silence
    // windows -- see AtropeloEnsemble.hpp/AtropeloPhrasePool.hpp.
    bool atropelo = false;
    int atropeloEventCount = 0;
    int atropeloInterruptionCount = 0;
    int atropeloResumedCount = 0;
    int atropeloForgottenCount = 0;
    int atropeloSilenceInvasionCount = 0;
    int atropeloNestingCount = 0;
    bool tempoDrift = false;
    double tempoDriftDepth = 0.0;
    int tempoChangeCount = 0;
    bool codaRitardando = false;
    // Fase 68 -- accelerando/stringendo toward the climax, the
    // ritardando's pair -- see accelerandoEnabled in renderIntegratedPiece.
    bool accelerando = false;
    // Fase 70 -- "Parenteses": master-output reroute to a captured
    // stutter loop, exact resumption -- see ParenthesisGlitch.hpp.
    bool parenthesis = false;
    int parenthesisCount = 0;
    std::string parenthesisLastMode;
    double parenthesisLastDurationSeconds = 0.0;
    double parenthesisLastLoopSeconds = 0.0;
    bool agon = false;
    // "Rusga": a real Brazilian Portuguese word for a small, brief
    // disagreement or friction between people -- and a near-anagram of
    // "rasgo" itself (same R-S-G skeleton, only two vowels swapped). Named
    // for this specific moment (a brief conflict between voices), not for
    // the whole Agon module.
    int agonRusgaCount = 0;
    // A deliberately unmotivated, un-smoothed intrusion -- see the
    // declaration comment above estranhoEnabled in renderIntegratedPiece.
    bool estranho = false;
    int estranhoCount = 0;
    // The literal version of "elemento estranho" -- see ForeignSignal.hpp.
    // foreignSignalUsed is true only when this piece both attempted a
    // reading AND the machine actually had one available; a piece where
    // it's true is a piece that is no longer purely reproducible from its
    // seed alone, honestly, on purpose.
    bool foreignSignalUsed = false;
    std::string foreignSignalDescription;
    // "Modo selvagem" -- see PieceConfig::wildMode. wildReseedCount is how
    // many times real hardware entropy actually reseeded the shared rng
    // stream during this render (once per bar while the mode is on).
    bool wildMode = false;
    int wildReseedCount = 0;
    // Real Q1.15 integer wraparound -- see FixedPointOverflow.hpp.
    bool fixedOverflow = false;
    int fixedOverflowCount = 0;
    // Real spectral-domain phase randomization -- see SpectralGhost.hpp.
    bool spectralGhost = false;
    int spectralGhostCount = 0;
    // Real sample-rate corruption -- see SampleHoldGlitch.hpp.
    bool sampleHoldGlitch = false;
    int sampleHoldGlitchCount = 0;
    // Drum-dynamics trio: hihat chaos contagion piggybacks on the existing
    // chaosContagion flag above (no separate field); these two are new.
    bool tomRatchet = false;
    int tomRatchetCount = 0;
    bool cymbalSustain = false;
    int cymbalSustainCount = 0;
    // Real math (Bjorklund/Toussaint), not generated chaos -- see the
    // declaration comment above euclideanCowbellEnabled.
    bool euclideanCowbell = false;
    std::string euclideanCowbellPattern;
    bool harmonicWeight = false;
    double strongBeatPull = 0.0;
    bool tonalModulation = false;
    int modulationCount = 0;
    int finalRootMidiNote = 57;
    bool modalShift = false;
    int modalShiftCount = 0;
    bool chordProgression = false;
    // Neo-Riemannian P/L/R triadic transformations -- see NeoRiemannian.hpp.
    bool neoRiemannian = false;
    int neoRiemannianOpCount = 0;
    // Real semitone-distance voice leading -- see voiceLeadingEnabled_ in
    // StepSequencer.hpp.
    bool voiceLeading = false;
    bool rhythmHumanize = false;
    int humanizeMaxSamples = 0;
    int humanizeApplicationCount = 0;
    int swingApplicationCount = 0;
    int percFxColoredCount = 0;
    int percFxCharacterVcaCount = 0;
    bool hocketing = false;
    int hocketCount = 0;
    int hocketInfluenceLinks = 0;
    // Shift-register cascade flavour of hocketing -- see
    // hocketCascadeEnabled in renderIntegratedPiece.
    bool hocketCascade = false;
    bool freeIntonation = false;
    double freeIntonationDepth = 0.0;
    // Fase 71 -- "Heisenberg": rhythmic and pitch precision coupled
    // inversely -- see heisenbergEnabled in renderIntegratedPiece.
    bool heisenberg = false;
    double heisenbergBalance = 0.0;
    bool intensityGuaranteeTriggered = false;
    bool rungler = false;
    bool fractureFold = false;
    bool leibnizRectifier = false;
    bool tubaVoice = false;
    bool chaosContagion = false;
    bool thermalFade = false;
    bool thermalAbruptDeath = false;
    bool rosslerCollapse = false;
    bool selfEpitaph = false;
    bool stochasticMute = false;
    // Real Walsh-function (Hadamard/sequency-ordered +-1 basis) additive
    // synthesis -- see WalshOscillator.hpp.
    bool walshOscillator = false;
    // Fase 72 -- VOSIM formant synthesis -- see VosimOscillator.hpp.
    bool vosim = false;
    // Fase 92 -- hard sync VCO (see HardSyncVoice.hpp).
    bool hardSync = false;
    // Fase 73 -- chaotic binaural beat -- see binauralEnabled in
    // renderIntegratedPiece.
    bool binaural = false;
    double binauralOffsetHz = 0.0;
    // Fase 74 -- envelope de "asfixia" por sidechain -- see asphyxiaEnabled
    // in renderIntegratedPiece.
    bool asphyxia = false;
    int asphyxiaCount = 0;
    // Fase 75 -- third filter colour, genuine self-oscillation -- see
    // polivoksEnabled in renderIntegratedPiece.
    bool polivoks = false;
    // Fase 93 -- Dual VCA, Polivoks-architecture inspired -- see
    // PolivoksVCA.hpp. Colours the two hocketing voices' own gain stage.
    bool polivoksVca = false;
    // Fase 94 -- acid bassline voice (see AcidBasslineVoice.hpp).
    bool acidBassline = false;
    int acidBasslineAccentCount = 0;
    int acidBasslineSlideCount = 0;
    bool bassBehaviour = false;
    int bassPlanCount = 0;
    int bassEventCount = 0;
    int bassTubaEventCount = 0;
    int bassAcidEventCount = 0;
    int bassCoreSubEventCount = 0;
    bool bassExecutionFeedback = false;
    std::uint64_t bassFeedbackUpdates = 0;
    std::uint64_t bassObservedTriggers = 0;
    std::uint64_t bassObservedActiveFrames = 0;
    bool bassKickInteraction = false;
    std::uint64_t bassKickInteractionTriggers = 0;
    std::uint64_t bassKickReducedFrames = 0;
    bool bassPhraseTimbre = false;
    std::array<std::uint64_t, 3> bassPhraseTimbreNotes{};
    std::uint64_t bassPhraseTimbreFrames = 0;
    bool grooveDialogue = false;
    std::array<std::uint64_t, 6> grooveDialogueBars{};
    std::array<std::uint64_t, 3> grooveDialogueHitPlans{};
    std::uint64_t grooveDialogueBassImprints = 0;
    bool grooveSubdivisions = false;
    std::array<std::uint64_t, 3> grooveSubdivisionExtraPlans{};
    std::array<std::uint64_t, 3> grooveSubdivisionExtraTriggers{};
    int bassGrooveApplicationCount = 0;
    int drumGrooveApplicationCount = 0;
    int bassEarlyTimingClampCount = 0;
    std::string bassFinalRole;
    std::string bassFinalBody;
    // Fase 95 -- envelope loop pulse (see EnvelopeLoopPulse.hpp).
    bool egLoopPulse = false;
    int egLoopPulseCount = 0;
    // Fase 96 -- "nota de rodape" (footnote): see FootnotePluck.hpp.
    bool footnoteGesture = false;
    int footnoteEventCount = 0;
    // Fase 97 -- "Galo" (chamado de animal sintetizado) -- see GaloVoice.hpp.
    bool galoCall = false;
    // Fase 98 -- voz de scat (real vowel-formant syllables) -- see ScatVoice.hpp.
    bool scatVoice = false;
    int scatEventCount = 0;
    // Fase 99 -- "Coral" polifonico (see ChoirEnsemble.hpp).
    bool choir = false;
    int choirPhraseCount = 0;
    // Fase 101 -- "deja vu" (see DejaVuVoice.hpp): a synthesized false
    // memory, built from running statistical averages, never a real
    // quotation of anything that actually happened.
    bool dejaVu = false;
    int dejaVuEventCount = 0;
    // Fase 101 -- linhagem/proveniencia + leitura apocrifa, see the
    // self-epitaph construction near the end of renderIntegratedPiece.
    bool apocryphalReading = false;
    int polivoksSelfOscillationCount = 0;
    // Fase 104 -- objeto de melodia/flauta (tarefa #31): real FM synthesis
    // (Chowning/DX7-flute-style) phrasing over its own independent
    // harmonic engine -- see FmFluteVoice.hpp / HarmonicWanderer.hpp.
    bool fmFlute = false;
    int fmFluteNoteCount = 0;
    std::string fmFluteHarmonicMovement;
    std::string fmFluteVoicingStyle;
    std::string fmFluteDissonanceStyle;
    // Fase 76 -- Vector Synthesis (2D crossfade) -- see vectorSynthEnabled
    // in renderIntegratedPiece.
    bool vectorSynth = false;
    // Fase 77 -- Harmonaig chord pad -- see harmonaigEnabled in
    // renderIntegratedPiece.
    bool harmonaig = false;
    int harmonaigChordCount = 0;
    // Fase 78 -- Xenakis sieve polyphony (4 independent channels) -- see
    // sievePolyphonyEnabled in renderIntegratedPiece.
    bool sievePolyphony = false;
    int sievePolyphonyPlasmaCount = 0;
    std::array<int, 4> sievePitchChangeCount{};
    std::array<int, 4> sieveRepeatedPitchCount{};
    std::array<int, 4> sieveLongestRepeatedNotes{};
    std::array<int, 4> sievePitchMin{};
    std::array<int, 4> sievePitchMax{};
    int sieveWaterArticulationChangeCount = 0;
    double sieveWaterDensityMin = 0.0;
    double sieveWaterDensityMax = 0.0;
    double sieveWaterGrainDurationMin = 0.0;
    double sieveWaterGrainDurationMax = 0.0;
    double sieveWaterPitchJitterMin = 0.0;
    double sieveWaterPitchJitterMax = 0.0;
    // Fase 79 -- Bloom-inspired fractal melody (Trunk/Branch/Path) -- see
    // bloomEnabled in renderIntegratedPiece.
    bool bloom = false;
    int bloomLevelSwitchCount = 0;
    // Audio-domain peak reaching into the clock domain -- see
    // crossDomainContagionEnabled in renderIntegratedPiece.
    bool crossDomainContagion = false;
    int crossDomainContagionCount = 0;
    // Xenakis sieve-generated pitch-class set, not a catalog lookup --
    // see SieveScale.hpp.
    bool generatedScale = false;
    // Bastl Popcorn-style addressable step reading -- see
    // chaosStepAddressEnabled in renderIntegratedPiece.
    bool chaosStepAddress = false;
    int chaosStepAddressCount = 0;
    // Fase 107 -- sequencer wild timing (task #30), see
    // sequencerWildTimingEnabled in renderIntegratedPiece.
    bool sequencerWildTiming = false;
    int sequencerWildRegenCount = 0;
    // Fase 109 -- Atari Punk Console wild clock (task #30), see
    // atariPunkGlitchEnabled in renderIntegratedPiece.
    bool atariPunkGlitch = false;
    int atariPunkGlitchCount = 0;
    // Fase 84 -- "Curto-Circuito": a real hard-gate dropout that also
    // force-activates a curated handful of mechanisms the seed had
    // otherwise left off -- see curtoCircuitoEnabled in
    // renderIntegratedPiece.
    bool curtoCircuito = false;
    int curtoCircuitoGatecrashCount = 0;
    // Fase 111 -- phasing (Steve Reich), see phasingEnabled in
    // renderIntegratedPiece.
    bool phasing = false;
    int phasingEventCount = 0;
    int phasingFullCycles = 0;
    // Fase 112 -- microtonalidade (task #3), see microtonalEnabled in
    // renderIntegratedPiece.
    bool microtonal = false;
    std::string microtonalSystem;
    double microtonalCommaDriftCents = 0.0;
    // Fase 113 -- objeto espectral (task #3), see spectralObjectEnabled in
    // renderIntegratedPiece.
    bool spectralObject = false;
    std::string spectralObjectDescription;
    int spectralObjectAdmittedPartials = 0;
    int spectralObjectPulseCount = 0;
    // Fase 114 -- metrica como sistema (task #3), see polymeterEnabled/
    // metricModulationEnabled in renderIntegratedPiece.
    bool polymeter = false;
    std::string polymeterDescription;
    int polymeterRealignmentCount = 0;
    bool metricModulation = false;
    int metricModulationEventCount = 0;
    // Fase 115 -- jogo: sinal de blefe e confianca (task #6), see
    // bluffSignalEnabled in renderIntegratedPiece.
    bool bluffSignal = false;
    double bluffTruthfulness = 0.0;
    int bluffSignalCount = 0;
    int bluffGenuineCount = 0;
    int bluffBluffCount = 0;
    double bluffFinalTrust = 0.0;
    bool bluffBetrayed = false;
    // Fase 116 -- rasgo espectral (task #7), see rasgoFractureEnabled in
    // renderIntegratedPiece.
    bool rasgoFracture = false;
    int rasgoFractureBreaks = 0;
    double rasgoFractureShapeK = 0.0;
    // Fase 117 -- ruptura por excesso de coerencia (task #7), see
    // coherenceCollapseEnabled in renderIntegratedPiece.
    bool coherenceCollapse = false;
    int coherenceCollapseCount = 0;
    int observableModuleActiveCount = 0;
    int observableModuleCapacity = 90;
};

inline constexpr int observableModuleCapacity() { return 90; }

// Canonical count behind the Studio panel. Presence is intentionally binary
// and does not imply equal perceptual importance; the narrative attention
// layer supplies that separate hierarchy.
inline int observableModuleActiveCount(const PieceModuleInfo &info) {
    int count = 0;
    auto add = [&](bool active) { if (active) ++count; };
    add(info.drumsEnabled);
    add(info.drumsProtagonist);
    add(info.voiceColor != "none");
    add(info.loFiDelay);
    add(info.staticPop);
    add(info.noiseColor != "white");
    add(info.ochdBreathing);
    add(info.selfFeedback);
    add(info.crossFmDepth > 0.01);
    add(info.tempoDrift);
    add(info.codaRitardando);
    add(info.accelerando);
    add(info.harmonicWeight);
    add(info.apocryphalTear);
    add(info.tonalModulation && info.modulationCount > 0);
    add(info.modalShift && info.modalShiftCount > 0);
    add(info.chordProgression);
    add(info.neoRiemannian);
    add(info.voiceLeading);
    add(info.rhythmHumanize);
    add(info.hocketing);
    add(info.hocketCascade);
    add(info.freeIntonation);
    add(info.heisenberg);
    add(info.agon);
    add(info.parenthesis);
    add(info.estranho);
    add(info.foreignSignalUsed);
    add(info.fixedOverflow);
    add(info.spectralGhost);
    add(info.sampleHoldGlitch);
    add(info.tomRatchet);
    add(info.cymbalSustain);
    add(info.euclideanCowbell);
    add(info.drumArchetype);
    add(info.rhythmSieveUsed);
    add(info.rhythmGenAksakUsed);
    add(info.rhythmGenEuclidUsed);
    add(info.ghostHihatCount > 0);
    add(info.orchestrationChangeCount > 0);
    add(info.rungler);
    add(info.fractureFold);
    add(info.leibnizRectifier);
    add(info.tubaVoice);
    add(info.chaosContagion);
    add(info.thermalFade);
    add(info.thermalAbruptDeath);
    add(info.rosslerCollapse);
    add(info.selfEpitaph);
    add(info.stochasticMute);
    add(info.walshOscillator);
    add(info.vosim);
    add(info.binaural);
    add(info.asphyxia);
    add(info.polivoks);
    add(info.polivoksVca);
    add(info.acidBassline);
    add(info.egLoopPulse);
    add(info.footnoteGesture);
    add(info.galoCall);
    add(info.scatVoice);
    add(info.choir);
    add(info.dejaVu);
    add(info.apocryphalReading);
    add(info.vectorSynth);
    add(info.harmonaig);
    add(info.sievePolyphony);
    add(info.bloom);
    add(info.crossDomainContagion);
    add(info.generatedScale);
    add(info.melodicContourDirection > 0);
    add(info.melodicContourDirection < 0);
    add(info.chaosStepAddress);
    add(info.thereminVoice);
    add(info.relationalOrchestra && info.relationalThereminResponseCount > 0);
    add(info.organismAudibility && info.organismAudibilityMovementCount > 0);
    add(info.relationalChamber && info.relationalChamberDecisionCount > 0);
    add(info.narrativeAttention && info.narrativeAttentionObservedSectionCount > 0);
    add(info.narrativeMix && info.narrativeMixDirectiveCount > 0);
    add(info.montageCut);
    add(info.atropelo);
    add(info.hardSync);
    add(info.phasing);
    add(info.microtonal);
    add(info.spectralObject);
    add(info.polymeter);
    add(info.metricModulation);
    add(info.bluffSignal);
    add(info.rasgoFracture);
    add(info.coherenceCollapse);
    return count;
}

// One entry per tempo change, in playback order: the second it takes
// effect (from the start of the piece) and the bpm it changes to. Always
// has at least one entry (0.0, cfg.bpm), even for a piece that never
// changes tempo at all -- a UI showing "current bpm" during playback can
// always just find the last entry at or before the current position,
// with no special-cased "no tempo change happened" branch.
struct TempoEvent {
    double atSeconds;
    double bpm;
};

struct PieceResult {
    std::string path;         // empty on failure
    std::vector<float> samples; // interleaved stereo, for a UI waveform preview
    int sampleRate = 48000;
    unsigned seed = 0; // same seed the piece was rendered from -- lets a caller
                        // derive further choices (e.g. playback mode) without
                        // needing its own independent RNG roll.
    PieceModuleInfo info;
    std::vector<TempoEvent> tempoTimeline;
    ProbabilityMarket nextProbabilityMarket;
};

using ProgressCallback = std::function<void(const PieceProgress &)>;

// Runs synchronously -- callers driving a GUI should invoke this from a
// worker thread and marshal onProgress/the final result back to the message
// thread themselves. The render itself is far faster than real-time (it's
// plain sample-by-sample math, no I/O until the final WAV write), so even an
// 80s piece renders in a small fraction of a second of CPU time.
inline PieceResult renderIntegratedPiece(const PieceConfig &cfg, const ProgressCallback &onProgress = nullptr) {
    const double sampleRate = 48000.0;

    // Where cfg.seed actually becomes consequence: from this line down,
    // nothing negotiates with it, every choice in this function only
    // ever inherits from it, never the other way around.
    std::mt19937 rng(cfg.seed);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    int wildReseedCount = 0;

    const CompositionIdentity identity = cfg.compositionIdentityOverride
        ? cfg.compositionIdentityOverrideValue
        : makeCompositionIdentity(cfg.seed);
    MusicalDirection musicalDirection(cfg.seed, cfg.musicalDirectionEnabled, sampleRate);
    MusicalRelations musicalRelations(cfg.seed,
        cfg.musicalDirectionEnabled && cfg.musicalRelationsEnabled, sampleRate);
    ModuleSelectionTrace selectionTrace(cfg.moduleSelection);
    const bool coreVoiceEnabled = selectionTrace.resolve(StudioModule::Core, true);
    const int pieceRootMidiNote = cfg.rootOverride ? cfg.rootMidiNote : identity.rootMidiNote;
    const bool rhythmicProtagonist = identity.focus == MaterialFocus::Rhythmic;
    const bool spectralProtagonist = identity.focus == MaterialFocus::Spectral;
    const bool harmonicProtagonist = identity.focus == MaterialFocus::Harmonic;
    const bool ensembleLead = identity.leadVoice == LeadVoiceModel::Ensemble;
    bool callResponseArrangement = identity.ensembleModel == EnsembleModel::CallResponse;
    bool counterpointArrangement = identity.ensembleModel == EnsembleModel::Counterpoint;
    bool polyphonicArrangement = identity.ensembleModel == EnsembleModel::Polyphonic;

    // Fase 80 -- Caillois game weights (Alea/Agon/Ilinx/Mimicry): a
    // "rule-making machine", formalizing the Caillois lens this project
    // already used interpretively (see the earlier addenda on Agon) into
    // a real mechanical bias. Four continuous weights, sampled uniformly
    // from the 4-simplex (they always sum to 1) via the standard "stick-
    // breaking" method -- three cut points sorted, the four gaps between
    // them are the weights -- so every piece gets its OWN bespoke blend
    // rather than a pick from a fixed list of named "games". Alea itself
    // gets no mechanism cluster: it IS the absence of thematic bias, the
    // baseline this engine already runs when every weight is roughly
    // equal. The other three nudge (never force -- gameBias always
    // leaves real headroom on both ends) a curated cluster of mechanisms
    // already emblematic of that game in the project's own vocabulary:
    // Agon (voices competing/coordinating) -> agon itself, Harmonaig,
    // sieve polyphony; Ilinx (vertigo/loss of control) -> Rungler,
    // Rossler collapse, thermal fade, self-feedback, stochastic mute,
    // Leibniz rectifier; Mimicry (simulacrum) -> apocryphal tear, self-
    // epitaph, foreign signal, parenthesis, elemento estranho.
    double gameWeightAlea, gameWeightAgon, gameWeightIlinx, gameWeightMimicry;
    if (cfg.gameWeightOverride) {
        // Fase 83 -- an album track walking its own trajectory across the
        // simplex (see AlbumRenderer.hpp) rather than rolling a fresh
        // bespoke blend; everything downstream (gameBias and every
        // mechanism it nudges) treats this identically either way.
        gameWeightAlea = cfg.gameWeightAleaOverride;
        gameWeightAgon = cfg.gameWeightAgonOverride;
        gameWeightIlinx = cfg.gameWeightIlinxOverride;
        gameWeightMimicry = cfg.gameWeightMimicryOverride;
    } else {
        double gameCuts[3] = { unit(rng), unit(rng), unit(rng) };
        std::sort(gameCuts, gameCuts + 3);
        gameWeightAlea = gameCuts[0];
        gameWeightAgon = gameCuts[1] - gameCuts[0];
        gameWeightIlinx = gameCuts[2] - gameCuts[1];
        gameWeightMimicry = 1.0 - gameCuts[2];
    }

    const GamePlan gamePlan = makeGamePlan(cfg.seed, identity,
        gameWeightAlea, gameWeightAgon, gameWeightIlinx, gameWeightMimicry);

    // Phase 119 -- the composition is no longer represented only by a
    // fixed call chain. These regions and passages are a first, deliberately
    // small mutable cartography: time, rhythm, harmony and space can read
    // one another through typed openings, and the RasgarGesture below can
    // split the temporal region while the piece is running.
    OpenModuleArchive moduleArchive(8192);
    CompositionalTopology topology(moduleArchive);
    const double temporalResistance = std::clamp(
        0.28 + 0.42 * gamePlan.memory - 0.18 * gamePlan.risk, 0.08, 0.9);
    const RegionId temporalRegion = topology.addRegion(
        RegionRole::Core, cfg.bpm / 180.0, gamePlan.energy,
        temporalResistance);
    const RegionId rhythmRegion = topology.addRegion(
        RegionRole::Rhythm, static_cast<double>(static_cast<int>(identity.rhythmModel)) / 3.0,
        rhythmicProtagonist ? 0.9 : gamePlan.energy,
        std::clamp(0.48 - 0.22 * gamePlan.freedom, 0.08, 0.8));
    const RegionId harmonyRegion = topology.addRegion(
        RegionRole::Harmony, static_cast<double>(pieceRootMidiNote - 57) / 12.0,
        harmonicProtagonist ? 0.9 : gamePlan.energy,
        std::clamp(0.52 + 0.22 * gamePlan.memory - 0.18 * gamePlan.freedom, 0.08, 0.9));
    const RegionId spaceRegion = topology.addRegion(
        RegionRole::Space, 0.0, 0.5,
        std::clamp(0.46 - 0.22 * gamePlan.hiddenInformation, 0.08, 0.85));
    auto passageModeForRule = [](GameRule rule) {
        switch (rule) {
            case GameRule::Preserve: return PassageMode::Preserve;
            case GameRule::Mutate: return PassageMode::Fold;
            case GameRule::Oppose: return PassageMode::Invert;
            case GameRule::Withhold: return PassageMode::Difference;
            case GameRule::Exchange: return PassageMode::Accumulate;
            case GameRule::Break: return PassageMode::Difference;
        }
        return PassageMode::Preserve;
    };
    const PassageMode gamePassageMode = passageModeForRule(gamePlan.rule);
    const std::uint32_t rhythmToHarmonyPassage = topology.connect(
        rhythmRegion, harmonyRegion, ModuleDomain::Rhythm, ModuleDomain::Harmony,
        gamePassageMode, 0.58 + 0.72 * gamePlan.freedom,
        0.58 - 0.42 * gamePlan.risk, false);
    const std::uint32_t harmonyToSpacePassage = topology.connect(
        harmonyRegion, spaceRegion, ModuleDomain::Harmony, ModuleDomain::Space,
        gamePassageMode, 0.52 + 0.64 * gamePlan.hiddenInformation,
        0.62 - 0.36 * gamePlan.freedom, true);
    RasgarGesture rasgarGesture(cfg.seed, identity, gamePlan, topology,
                               moduleArchive, temporalRegion, rhythmRegion);
    RasgarFrame rasgarFrame = rasgarGesture.frame();
    double topologyHarmonyBias = 0.0;
    double topologySpaceBias = 0.0;
    int topologyHarmonicShiftCount = 0;
    bool rasgarHarmonicShiftThisBar = false;
    // Strategy reorganizes ensemble relations. It never erases the identity
    // choice, but Counter must create an audible respondent and Negotiate
    // must distribute agency instead of leaving a nominal solo untouched.
    if (gamePlan.strategy == GameStrategy::Counter) callResponseArrangement = true;
    if (gamePlan.strategy == GameStrategy::Negotiate) {
        counterpointArrangement = counterpointArrangement || !polyphonicArrangement;
    }
    // Neutral (all four equal, 0.25) leaves the multiplier at 1.0 -- no
    // change from this mechanism's own baseline probability. A weight of
    // 1.0 multiplies by 2.8 (real emphasis, still capped at 0.97 so
    // nothing is ever a guarantee); a weight of 0.0 multiplies by 0.4
    // (suppressed, never silenced outright).
    auto gameBias = [](double base, double weight) {
        return std::clamp(base * (0.4 + 2.4 * weight), 0.0, 0.97);
    };

    // Probability market (fase 69, see ProbabilityMarket.hpp): the eight
    // most recent seed-gated mechanisms (fases 59-68, listed at each
    // market.price() call below) draw their probability from a persisted,
    // slowly-wandering "price" instead of a fixed constant. Scoped to one
    // directory per cfg.outDir, same as this piece's own WAV/txt output --
    // a fresh recordings folder starts a fresh market, an existing one
    // continues its own history.
    ProbabilityMarket market;
    std::string marketPath = cfg.outDir + "/.rasgo_market.txt";
    if (cfg.probabilityMarketOverride) market = cfg.probabilityMarketInput.continuation();
    else market.load(marketPath);

    // Diagnostic/audition hook, not part of the normal generative design --
    // lets a specific combination of the rare mechanisms below be forced
    // on or off, and their gap timers shrunk, purely so a short example
    // clip can demonstrate a chosen combination without waiting through a
    // full natural piece. Completely inert unless RASGO_AUDITION_MODE is
    // set in the environment; reads four extra env vars and does nothing
    // else otherwise, safe to leave compiled in.
    bool auditionMode = std::getenv("RASGO_AUDITION_MODE") != nullptr;
    auto auditionForce = [](const char *name, bool natural) -> bool {
        const char *v = std::getenv(name);
        if (!v) return natural;
        return std::string(v) == "1";
    };

    // "Elemento estranho", the literal version -- see ForeignSignal.hpp.
    // Whether a piece TRIES this is still seed-derived (reproducible: the
    // same seed always makes the same attempt), but the actual VALUE it
    // reads, when it succeeds, comes from the real machine at this exact
    // render, not from the seed -- this is the one place in the whole
    // engine where "same seed in" can honestly stop meaning "same piece
    // out". Rare by design (~1 piece in 4 even tries), and exposed
    // honestly in this piece's own metadata rather than folded in quietly.
    bool foreignSignalEnabled = auditionForce("RASGO_FORCE_FOREIGN", unit(rng) < gameBias(0.25, gameWeightMimicry));
    ForeignReading foreignReading;
    if (foreignSignalEnabled) foreignReading = readForeignSignal();

    // The arc is now a live Markov walk over structural roles (intro/rising/
    // peak/falling/coda) instead of a fixed-length list generated once and
    // scaled to fit a duration chosen beforehand -- see GenerativeArc.hpp
    // (phase 13). The walk decides duration by reaching its coda. Read-only
    // progress preview later counts that same walk without imposing a length.
    // Audition mode also caps the walk short -- a demo clip of a forced
    // mechanism should be quick to listen to, not a full natural piece.
    double effectiveMaxDuration = auditionMode ? std::min(cfg.maxDurationSeconds, 18.0) : cfg.maxDurationSeconds;
    GenerativeArc arc(cfg.seed, sampleRate, effectiveMaxDuration, cfg.energyBias);

    const int stepsPerBar = 16;
    const double secondsPerStep = 60.0 / cfg.bpm / 4.0;

    // No piece takes its scale from a user-set dial -- the seed picks one
    // out of the whole repertoire, the same way it picks everything else
    // about its character. Fase 83: this used to have an album-track
    // exception (keeping cfg.scale exactly as the theme specified, shared
    // across every track) -- removed. An album's cohesion now comes from
    // its game-weight trajectory (see AlbumRenderer.hpp), not a forced
    // shared key; each track picks its own scale as freely as a
    // standalone piece would. Fase 100 reintroduces a real override, but
    // for a different reason than the old album one -- inheriting a
    // SPECIFIC other piece's own scale for a mashup, not sharing one
    // fixed key across a whole set.
    Scale initialScale = cfg.scaleOverride
        ? cfg.scaleOverrideValue
        : identity.initialScale;

    StepSequencer sequencer;
    sequencer.configure(stepsPerBar, initialScale, pieceRootMidiNote, cfg.seed,
        static_cast<StepSequencer::RhythmModel>(static_cast<int>(identity.rhythmModel)),
        identity.scaleBlendProbability,
        static_cast<StepSequencer::HarmonicModel>(static_cast<int>(identity.harmonicLanguage)));
    sequencer.applyGameCharacter(gamePlan.memory, gamePlan.freedom, gamePlan.risk);

    // Sieve-generated experimental scale (see SieveScale.hpp): instead of
    // picking from the 45-scale researched catalog above, a rare fraction
    // of pieces (album tracks included as of fase 83, same as initialScale
    // above) get a pitch-class set constructed algorithmically via Xenakis
    // sieve theory -- genuinely uncatalogued, checked by construction
    // (rejection-sampled against the real catalog, not just assumed)
    // rather than another named lookup entry.
    bool generatedScaleEnabled = identity.spectral.generatedScale;
    market.record("generatedScale", generatedScaleEnabled);
    std::string generatedScaleDescription;
    if (generatedScaleEnabled) {
        SieveScale sieve = generateSieveScale(rng, cataloguedPitchClassSets());
        sequencer.setCustomPrimaryScale(sieve.table, sieve.description);
        generatedScaleDescription = sieve.description;
    }

    // Fase 94 -- acid bassline (see AcidBasslineVoice.hpp): a TB-303-style
    // all-in-one voice, the first combination in this engine of the
    // Polivoks filter's own self-oscillating character (fase 75) with a
    // dedicated fast filter envelope, accent, and slide -- a real
    // identity, not a generic bass patch. Pattern generated once per piece
    // (16 steps, real 303-style density -- mostly active with sparse
    // rests, occasional accent/slide/octave-jump per step), drawing notes
    // from the piece's own catalog scale table (not the rare generated-
    // sieve override above, a deliberate simplification) two octaves below
    // the root, the classic acid register.
    bool acidBasslineEnabled = selectionTrace.resolve(StudioModule::Acid, unit(rng) < 0.22);
    AcidBasslineVoice acidBass; acidBass.setSampleRate(sampleRate);
    acidBass.setBaseCutoff(220.0 + unit(rng) * 260.0);
    acidBass.setEnvDepth(1400.0 + unit(rng) * 2200.0);
    acidBass.setResonance(0.8 + unit(rng) * 0.18);
    acidBass.setDecayTimes(0.12 + unit(rng) * 0.18, 0.08 + unit(rng) * 0.14);
    std::array<bool, 16> acidActive{}, acidAccent{}, acidSlide{};
    std::array<int, 16> acidMidiNote{};
    {
        const ScaleTable &acidTable = sequencer.primaryScaleTable();
        for (int i = 0; i < 16; ++i) {
            acidActive[static_cast<size_t>(i)] = unit(rng) < 0.65;
            acidAccent[static_cast<size_t>(i)] = acidActive[static_cast<size_t>(i)] && unit(rng) < 0.25;
            acidSlide[static_cast<size_t>(i)] = acidActive[static_cast<size_t>(i)] && unit(rng) < 0.2;
            int degreeIdx = static_cast<int>(unit(rng) * acidTable.length) % acidTable.length;
            int octaveJitter = (unit(rng) < 0.3) ? 12 : 0;
            acidMidiNote[static_cast<size_t>(i)] = pieceRootMidiNote - 24 + acidTable.degrees[static_cast<size_t>(degreeIdx)] + octaveJitter;
        }
    }
    int acidBasslineAccentCount = 0;
    int acidBasslineSlideCount = 0;

    // Fase 95 -- envelope loop pulse (see EnvelopeLoopPulse.hpp): a
    // Polivoks EG "loop" self-cycling AD source, patched into itself the
    // way a real modular EG-as-clock idiom does, striking a short sine
    // blip at its own dedicated end-of-attack instant -- the only
    // mechanism in this whole engine that self-triggers from an envelope
    // completing its own cycle rather than any note/step/chaos-driven
    // logic. Runs on its own independent period, deliberately NOT locked
    // to the beat, tracking the piece's root note two octaves down (set
    // once at setup -- a deliberate simplification, not re-tracked through
    // any later tonal-centre modulation).
    bool egLoopPulseEnabled = selectionTrace.resolve(StudioModule::EnvelopePulse, unit(rng) < 0.2);
    EnvelopeLoopPulse egLoopPulse; egLoopPulse.setSampleRate(sampleRate);
    egLoopPulse.configure(0.003 + unit(rng) * 0.02, 0.12 + unit(rng) * 0.35);
    egLoopPulse.setFrequency(midiToFreq(pieceRootMidiNote - 24));
    int egLoopPulseCount = 0;

    // Fase 96 -- "nota de rodape" (footnote): a short, isolated 2-5 note
    // gesture, at most once per piece, placed at the weakest metrical
    // position it can find (fase 85's metricalStrengthForBar), pitched
    // entirely from the chromatic complement of the piece's own active
    // scale (the fase-78 plasma-channel technique, reused here for pitch
    // selection only) -- every note it plays is, by construction, outside
    // this piece's own harmonic vocabulary. Own dedicated reverb instances
    // (bigger room, more mix than anything else in the piece) so it reads
    // as spatially distinct too, not just tonally.
    bool footnoteEnabled = selectionTrace.resolve(StudioModule::Footnote, unit(rng) < 0.35);
    bool footnoteFired = false;
    bool footnoteArmed = false;
    int footnoteNotesRemaining = 0;
    int footnoteNoteGapCountdown = 0;
    int footnoteEventCount = 0;
    FootnotePluck footnotePluck; footnotePluck.setSampleRate(sampleRate);
    footnotePluck.setPartialRatio(unit(rng) < 0.5 ? (1.15 + unit(rng) * 0.2) : (1.65 + unit(rng) * 0.25));
    footnotePluck.setDecayTime(0.3 + unit(rng) * 0.5);
    SimpleReverb footnoteReverbL; footnoteReverbL.setSampleRate(sampleRate);
    SimpleReverb footnoteReverbR; footnoteReverbR.setSampleRate(sampleRate);
    {
        double footnoteRoomSize = 0.75 + unit(rng) * 0.25;
        double footnoteReverbMix = 0.45 + unit(rng) * 0.3;
        footnoteReverbL.setRoomSize(footnoteRoomSize);
        footnoteReverbL.setDamping(0.15);
        footnoteReverbL.setMix(footnoteReverbMix);
        footnoteReverbR.setRoomSize(footnoteRoomSize);
        footnoteReverbR.setDamping(0.15);
        footnoteReverbR.setMix(footnoteReverbMix);
    }
    std::array<int, 12> footnoteComplementPcs{};
    int footnoteComplementCount = 0;
    {
        std::set<int> footnoteUsedPcs;
        const ScaleTable &footnoteTable = sequencer.primaryScaleTable();
        for (int i = 0; i < footnoteTable.length; ++i) footnoteUsedPcs.insert(footnoteTable.degrees[static_cast<size_t>(i)] % 12);
        for (int p = 0; p < 12; ++p) {
            if (!footnoteUsedPcs.count(p)) footnoteComplementPcs[static_cast<size_t>(footnoteComplementCount++)] = p;
        }
        if (footnoteComplementCount == 0) {
            // Degenerate case (chromatic/near-chromatic scale leaves no
            // complement): fall back to a plain tritone from the root,
            // same fallback idiom the fase-78 plasma channel already uses.
            footnoteComplementPcs[0] = 6;
            footnoteComplementCount = 1;
        }
    }

    HenonMap henon; henon.setSampleRate(sampleRate); henon.setRate(35.0);
    henon.setInitial(0.05 + 0.1 * unit(rng), 0.05 + 0.1 * unit(rng));
    LorenzMap lorenz; lorenz.setSampleRate(sampleRate); lorenz.setRate(180.0);
    lorenz.setInitial(0.1 + 0.5 * unit(rng), 0.1 + 0.5 * unit(rng), 0.1 + 0.5 * unit(rng));
    LatoocarfianMap lato; lato.setSampleRate(sampleRate); lato.setRate(70.0);
    // A small, present-not-overpowering nudge from the foreign reading
    // above when one was actually taken -- the seed-derived starting point
    // stays the dominant term either way.
    double foreignNudge = (foreignSignalEnabled && foreignReading.available)
                        ? 0.05 * (foreignReading.normalized - 0.5) : 0.0;
    lato.setInitial(0.1 + 0.2 * unit(rng) + foreignNudge, 0.1 + 0.2 * unit(rng) + foreignNudge);

    // A real feedback loop, not just a metaphor for one -- the closed-loop
    // "no-input mixer"/Make Noise Maths-into-itself idea: the piece's own
    // recent output loudness modulates the Lorenz map's own rate, which
    // drives pitch drift, which affects the voice, which affects the
    // output loudness. Necessarily one sample late (this loop can't see
    // its own current-sample output before that sample exists), the same
    // causality-respecting lag every other cross-consumer value in this
    // loop already accepts. Seed-gated to about half of pieces -- a
    // self-referential character, not a universal behaviour.
    bool selfFeedbackEnabled = unit(rng) < gameBias(0.5, gameWeightIlinx);
    float outputFeedbackEnergy = 0.0f;
    const double lorenzBaseRate = 180.0;

    LatoocarfianMap ringCarrier; ringCarrier.setSampleRate(sampleRate);
    ringCarrier.setInitial(0.13 + 0.05 * unit(rng), 0.09 + 0.05 * unit(rng));
    RingModulator ringMod; ringMod.setBalance(0.95);

    // Five independently implemented chaotic maps added in phase 8 but not
    // initially wired into the main patch. Each gets a real, modest role
    // below rather than existing only as an unused implementation.
    StandardMap standardChaos; standardChaos.setSampleRate(sampleRate); standardChaos.setRate(0.8);
    standardChaos.setInitial(unit(rng) * 6.28, unit(rng) * 6.28);
    CuspMap cuspChaos; cuspChaos.setSampleRate(sampleRate); cuspChaos.setRate(1.1);
    cuspChaos.setParams(1.0, 1.0); cuspChaos.setInitial(unit(rng) * 0.5);
    QuadMap quadChaos; quadChaos.setSampleRate(sampleRate); quadChaos.setRate(40.0);
    quadChaos.setParams(-0.8 + unit(rng) * 0.3, 0.1, 0.1); quadChaos.setInitial(0.1 + unit(rng) * 0.2);
    LinCongMap lincongChaos; lincongChaos.setSampleRate(sampleRate); lincongChaos.setRate(0.5);
    lincongChaos.setParams(1103515245.0, 12345.0, 2147483648.0); lincongChaos.setInitial(unit(rng));
    FBSineMap fbsineChaos; fbsineChaos.setSampleRate(sampleRate); fbsineChaos.setRate(0.3);
    fbsineChaos.setParams(1.6, 0.9, 1.02, 0.05);
    fbsineChaos.setInitial(unit(rng) * 0.3, unit(rng) * 6.28);

    // Bastl Popcorn-style addressable stepping: rather than the melodic
    // read pointer always advancing 0,1,2...15 in lockstep with the master
    // clock (percussion/euclidean-cowbell/tom-cycle below stay on that
    // same linear clock, untouched), a dedicated chaos source's own 0..1
    // output instead picks WHICH scale-degree/gate index to read this step
    // -- the sequencer's own generated pattern becomes an addressable jump
    // table for a fraction of pieces, rather than a fixed lookup. Driven
    // once per STEP, not once per sample like every other chaos map in
    // this file -- setSampleRate(1.0) below makes each tick() call count
    // as one whole time unit, so setRate's meaning becomes "chaotic
    // iterations per step" instead of "per second of audio".
    bool chaosStepAddressEnabled = unit(rng) < market.price("chaosStepAddress", 0.15);
    market.record("chaosStepAddress", chaosStepAddressEnabled);
    StandardMap addressChaos;
    addressChaos.setSampleRate(1.0);
    addressChaos.setRate(0.3 + unit(rng) * 0.3);
    addressChaos.setParams(2.0 + unit(rng) * 1.5);
    addressChaos.setInitial(unit(rng) * 6.28, unit(rng) * 6.28);
    int chaosStepAddressCount = 0;

    // Fase 107 -- sequencer wild timing (task #30, user: "sequencer mais
    // imprevisivel/variado, mais inospito, mais musical"). The rhythmic/
    // melodic pattern (StepSequencer::regenerate) has always refreshed on
    // a perfectly regular clock -- once every bar, no exceptions, even
    // though the Turing Machine lock (see StepSequencer.hpp) already
    // varies HOW MUCH changes each time. That clock's own regularity is a
    // real, untouched source of the cross-piece sameness the user
    // reported (task #29) -- this changes WHEN it fires, not just what it
    // draws. A dedicated StandardMap (same setSampleRate(1.0) trick as
    // chaosStepAddress just above -- each tick() call is one step, not
    // one audio sample) ticks once per real step; regenerate() only fires
    // on a genuine zero-crossing of the chaotic value, gated by a real
    // minimum gap so it can never fire every single step (that would just
    // be noise, not music). A pattern can now hold steady for many bars
    // in a row (the map staying on one side, building real expectation)
    // or refresh twice within a single bar (a genuinely "inospito" event)
    // -- both are real properties of the underlying chaotic dynamics, not
    // an arbitrary probability roll. advanceChord() above stays on its
    // normal one-per-bar schedule regardless -- the harmonic rhythm keeps
    // its own steady anchor even while the surface pattern goes wild.
    bool sequencerWildTimingEnabled = unit(rng) < gamePlan.ruleChangeProbability(0.26);
    StandardMap sequencerRegenChaos;
    sequencerRegenChaos.setSampleRate(1.0);
    sequencerRegenChaos.setRate(0.15 + unit(rng) * 0.3);
    sequencerRegenChaos.setParams(1.8 + unit(rng) * 1.7);
    sequencerRegenChaos.setInitial(unit(rng) * 6.28, unit(rng) * 6.28);
    int sequencerRegenMinGapSteps = 2 + static_cast<int>(unit(rng) * 4.0);
    int sequencerRegenStepsSinceLast = 999; // large so the very first eligible step can always fire
    float sequencerRegenChaosPrevValue = 0.0f;
    bool sequencerRegenChaosHasPrev = false;
    int sequencerWildRegenCount = 0;
    double sequencerRegenDensity = 0.0;
    double sequencerRegenOctaveBias = 0.0;

    Oscillator osc1;
    osc1.setSampleRate(sampleRate);
    Oscillator::Waveform coreWaveform = Oscillator::Waveform::Saw;
    double coreOsc2Ratio = 1.503;
    float coreOsc1Gain = 1.0f;
    float coreOsc2Gain = 1.0f;
    float coreDiodeDrive = 2.5f;
    switch (identity.coreVoice) {
        case CoreVoiceModel::Subtractive:
            coreWaveform = Oscillator::Waveform::Saw;
            coreOsc2Ratio = 1.503;
            coreOsc2Gain = 0.75f;
            break;
        case CoreVoiceModel::Rounded:
            coreWaveform = Oscillator::Waveform::Sine;
            coreOsc2Ratio = 1.0;
            coreOsc2Gain = 0.42f;
            coreDiodeDrive = 1.25f;
            break;
        case CoreVoiceModel::Pulse:
            coreWaveform = Oscillator::Waveform::Pulse;
            coreOsc2Ratio = 0.5;
            coreOsc2Gain = 0.62f;
            coreDiodeDrive = 2.1f;
            break;
        case CoreVoiceModel::Inharmonic:
            coreWaveform = Oscillator::Waveform::Saw;
            coreOsc2Ratio = 1.41421356237;
            coreOsc2Gain = 0.85f;
            coreDiodeDrive = 3.4f;
            break;
        case CoreVoiceModel::Minimal:
            coreWaveform = Oscillator::Waveform::Sine;
            coreOsc2Ratio = 2.0;
            coreOsc2Gain = 0.08f;
            coreDiodeDrive = 1.0f;
            break;
    }
    osc1.setWaveform(coreWaveform);
    osc1.setPulseWidth(0.18 + identityUnit(cfg.seed, 60) * 0.62);
    DiodeMixer voiceDiodeMixer; // 2x-oversampled diodeMix -- see DiodeShaper.hpp

    // Lyra-8-style bidirectional cross-FM: rather than only an external
    // chaos source modulating both oscillators one-way, each oscillator's
    // own previous-sample output nudges the *other's* frequency this
    // sample -- a genuine two-way tug distinct from a shared modulation
    // source, capable of the same "oscillators fighting for dominance"
    // instability Rob Hordijk's Rungler-adjacent designs are known for.
    // 0 for about half of pieces (no cross-FM, today's one-way-only
    // behaviour); otherwise a modest depth so it destabilises tuning
    // without making the voice unusable.
    double crossFmDepth = (unit(rng) < 0.5) ? 0.0 : 0.4 + unit(rng) * 1.1;
    float lastOsc1Out = 0.0f, lastOsc2Out = 0.0f;

    // A seed-derived character colour for the whole piece -- an original
    // implementation informed by Pantala Labs' "Saturador de fita"/"Tube
    // Control" concept (a tape
    // saturator's rounded transients + gentle top-end loss + wow, versus a
    // valve stage's even-harmonic-biased asymmetric clip), plus a third,
    // more violent option: Fracture, a West Coast wavefolder (Buchla/Serge
    // lineage) deliberately pushed past clean periodic folding into a
    // jittered, tearing one -- see FractureFolder.hpp. None, Tape, Tube or
    // Fracture -- deliberately not layered together, and deliberately not
    // "always on": most pieces still get neither, since this is a colour
    // choice, not a universal quality upgrade.
    enum class VoiceColor { None, Tape, Tube, Fracture };
    VoiceColor voiceColor = VoiceColor::None;
    {
        double colorRoll = unit(rng);
        if (colorRoll < 0.25) voiceColor = VoiceColor::Tape;
        else if (colorRoll < 0.45) voiceColor = VoiceColor::Tube;
        else if (colorRoll < 0.58) voiceColor = VoiceColor::Fracture;
    }
    TapeSaturator tapeSat; tapeSat.setSampleRate(sampleRate);
    tapeSat.setDrive(0.3f + static_cast<float>(unit(rng)) * 0.3f);
    TubeSaturator tubeSat;
    tubeSat.setDrive(0.25f + static_cast<float>(unit(rng)) * 0.3f);
    FractureFolder fractureFold;
    fractureFold.seed(cfg.seed + 26000u);
    fractureFold.setFoldAmount(0.15 + unit(rng) * 0.35);
    fractureFold.setFractureAmount(0.3 + unit(rng) * 0.6);

    // osc2 used to be a fixed pulse wave with pulse-width modulation; now a
    // MorphOscillator driven by the same chaos value (latoVal) sweeps
    // continuously through sine/triangle/saw/square instead of just
    // widening a pulse -- a much richer timbral response to the same
    // modulation source.
    MorphOscillator osc2;
    osc2.setSampleRate(sampleRate);

    // Hocketing (Intellijel Shifty's idea): rather than always the same
    // voice sounding every note, some notes get handed to a different
    // timbre/register instead -- the classic effect of a melodic line
    // bouncing between instruments. Purely additive, not a rewrite of the
    // main voice: osc1/osc2 keep playing every note exactly as they always
    // have (with all their existing modulation intact), but on a hocket
    // turn one of these two extra oscillators *also* sounds that note,
    // panned/registered distinctly, so the ear catches a different
    // instrument taking the note. Off for about 60% of pieces -- most
    // pieces stay a single continuous line, as they always have.
    Oscillator hocketOsc1; hocketOsc1.setSampleRate(sampleRate);
    hocketOsc1.setWaveform(Oscillator::Waveform::Pulse);
    Oscillator hocketOsc2; hocketOsc2.setSampleRate(sampleRate);
    hocketOsc2.setWaveform(Oscillator::Waveform::Sine);
    bool hocketEnabled = selectionTrace.resolve(StudioModule::Hocket,
        callResponseArrangement || counterpointArrangement || identityUnit(cfg.seed, 72) < 0.16);
    int hocketOctaveShift1 = static_cast<int>(unit(rng) * 3.0) - 1; // -1, 0, or +1 octave
    int hocketOctaveShift2 = static_cast<int>(unit(rng) * 3.0) - 1;
    int hocketCycle = 0;
    float hocketEnv1 = 0.0f, hocketEnv2 = 0.0f;
    double hocketDecay1 = decayCoeff(0.12 + unit(rng) * 0.2, sampleRate);
    double hocketDecay2 = decayCoeff(0.12 + unit(rng) * 0.2, sampleRate);
    int hocketCount = 0;
    float lastHocket1Out = 0.0f, lastHocket2Out = 0.0f;

    // Polyphonic cascade -- a genuine Shifty-style shift register (Intellijel
    // Shifty's actual behaviour, which the additive hocketing above
    // deliberately simplified away from at fase 37): EVERY gated note pushes
    // through the register instead of only every third note taking an
    // isolated turn. hocketOsc1 always catches the newest note fresh;
    // whatever it was playing a moment ago shifts down into hocketOsc2,
    // continuing from wherever its envelope had already decayed to rather
    // than restarting -- so several recent notes genuinely overlap and ring
    // together, an emergent chord-like texture from a monophonic sequence,
    // "ghost" melodic lines crossing instead of one voice occasionally
    // borrowing another instrument's timbre. Only a fraction of pieces that
    // already have hocketing enabled get the cascade flavour instead of the
    // existing alternating-turn one -- mutually exclusive per piece, same
    // "one clean mode, not layered" discipline as modal shift/tonal
    // modulation and voice leading/degree-jumpiness elsewhere in this file.
    bool hocketCascadeEnabled = hocketEnabled && unit(rng) < market.price("hocketCascade", 0.35);
    market.record("hocketCascade", hocketCascadeEnabled);
    double cascadeFreq1 = 0.0;
    DiodeMixer cascadeDiodeMixer; // separate history from voiceDiodeMixer -- a different signal pair entirely

    // Extends the osc1<->osc2 cross-FM above into a sparse, one-way
    // "network" including the two hocket oscillators, rather than a dense
    // all-4-modulate-all-4 mesh (that reads as noise fast -- exactly the
    // "massaroca" risk raised when this was proposed). Each of the four
    // possible hocket->main-voice links gets its own independent, low
    // probability of being active this piece; the reverse direction (main
    // voice modulating the hocket oscillators back) is deliberately never
    // wired, so this can't create a new feedback loop on top of the
    // already-tested osc1<->osc2 one -- a transient hit can nudge the
    // sustained voice's pitch, but nothing nudges back.
    bool hocket1ToOsc1 = hocketEnabled && unit(rng) < 0.2;
    bool hocket1ToOsc2 = hocketEnabled && unit(rng) < 0.2;
    bool hocket2ToOsc1 = hocketEnabled && unit(rng) < 0.2;
    bool hocket2ToOsc2 = hocketEnabled && unit(rng) < 0.2;
    float hocketInfluenceDepth = static_cast<float>(0.3 + unit(rng) * 0.7);

    // A modal resonator layered under the subtractive voice -- additive/
    // resonant sound generation (the idea behind Mutable Instruments'
    // Rings) rather than oscillator-plus-filter, struck once per gated
    // step and left to ring on its own. Ratios/decays get a little jitter
    // from the seed so each piece's "struck object" has its own identity
    // (a slightly different bar/bell/plate) rather than one fixed timbre.
    ModalResonator resonator; resonator.setSampleRate(sampleRate);
    std::array<double, ModalResonator::kNumModes> modalRatios{}, modalDecays{}, modalGains{};
    for (int m = 0; m < ModalResonator::kNumModes; ++m) {
        modalRatios[static_cast<size_t>(m)] = (m + 1) * (1.0 + 0.02 * m) + (unit(rng) - 0.5) * 0.08;
        modalDecays[static_cast<size_t>(m)] = 1.3 / (m + 1.0) + unit(rng) * 0.2;
        modalGains[static_cast<size_t>(m)] = 1.0 / (m + 1.0);
    }
    resonator.setModes(modalRatios, modalDecays, modalGains);
    resonator.setBrightness(0.5f);

    // A second, independent ModalResonator reused for a genuinely
    // different purpose: not struck, blown. Same class, same safety
    // properties (every mode is a damped two-pole filter, r < 1 always --
    // continuous excitation settles to a bounded steady state, it can't
    // build up without limit), but fed every sample by BreathExciter
    // instead of one mallet impulse, and tuned toward long-open-tube
    // territory (near-harmonic ratios, longer decay, low fundamental)
    // instead of a bar/bell's stretched inharmonic ratios -- a low,
    // breathy voice request, "elemento de grave importante ao conjunto."
    // A genuinely different colour from everything else in the engine, but
    // deliberately rare (seed-gated) and deliberately gentle in the mix --
    // see the quality bar this was built under: allowed to be radical in
    // concept, never allowed to make the final render harder to listen to.
    // Voz de theremin: a glide-only sine body, never re-triggered like every
    // other voice here. Legacy routing can still shadow melody pitch; the
    // relational orchestra gives it independent pitch and a non-prescriptive
    // degree of temporal presence (continuous, latent or changing by history).
    // Same rarity class/plain-probability convention as the tuba voice
    // just below (both additive circuit-behavior textures, not part of
    // the narrative/simulacro family apocryphal tear or corte de montagem
    // belong to, so not gameBias'd by Mimicry).
    bool thereminLead = identity.leadVoice == LeadVoiceModel::Theremin;
    bool thereminEnabled = selectionTrace.resolve(StudioModule::Theremin, thereminLead || identityUnit(cfg.seed, 69) < 0.07);
    ThereminVoice theremin;
    theremin.setSampleRate(sampleRate);
    if (thereminEnabled) {
        double glideSeconds = 0.15 + unit(rng) * 0.35;
        double vibratoHz = 5.0 + unit(rng) * 2.0;
        float vibratoDepth = static_cast<float>(0.15 + unit(rng) * 0.5);
        theremin.configure(glideSeconds, vibratoHz, vibratoDepth, cfg.seed + 63000u);
        theremin.setGain(static_cast<float>((thereminLead ? 0.9 : 0.45) + unit(rng) * (thereminLead ? 0.35 : 0.3)));
    }

    // Fase 104 -- objeto de melodia/flauta (tarefa #31): the melody
    // object's own harmonic engine, running independently of
    // sequencer.chordProgressionEnabled() (only ~50% of pieces, a real
    // gap the project's own standardization audit found -- task #29), so
    // this always has real harmonic content to phrase over regardless.
    // Real 2-operator FM synthesis (Chowning/DX7-flute-style, a genuinely
    // new synthesis principle for this engine) plus one of six real,
    // documented harmonic-movement techniques (HarmonicWanderer.hpp),
    // seed-picked once per piece and advanced at genuine arc-section
    // boundaries -- same "passagens" convention as the rhythm archetype
    // (fase 67) and tonal modulation (fase 32/33).
    bool fmFluteLead = identity.leadVoice == LeadVoiceModel::FmFlute;
    bool fmFluteEnabled = selectionTrace.resolve(StudioModule::FmFlute, fmFluteLead || identityUnit(cfg.seed, 70) < fmFluteProbability(identity));
    FmFluteVoice fmFlute;
    fmFlute.setSampleRate(sampleRate);
    HarmonicWanderer harmonicWanderer;
    // Eixo B (task #31): the vertical/voicing structure under the flute's
    // phrases, independent of Eixo A's harmonic-movement choice above --
    // sorteado por conta propria, mais combinacoes reais entre as duas
    // dimensoes do que se fossem acopladas.
    MelodyVoicingBank voicingBank;
    voicingBank.setSampleRate(sampleRate);
    // Eixo C (task #31): how each new phrase note relates to the scale
    // degree it was drawn from -- independent of Eixo A (key centre) and
    // Eixo B (accompaniment voicing) above.
    DissonanceTreatment dissonanceTreatment;
    // Fase 125 -- intérprete melódico: adaptação local da inteligência de
    // frase do Antitotem para a matéria própria da flauta FM. Ele não escolhe
    // a escala nem substitui o Eixo C; interpreta a chegada e a articulação
    // depois que o motor harmônico escolheu o alvo.
    MelodicInterpreter fmFluteMelodicInterpreter(cfg.seed + 76000u);
    MelodicInterpretation fmFluteInterpretation;
    MaterialExpressionField materialExpressionField;
    SpatialExpressionField spatialExpressionField;
    MaterialExpressionPlan lastMaterialExpressionPlan;
    SpatialExpressionPlan lastSpatialExpressionPlan;
    ExpressiveInterpretation lastExpressiveInterpretation = ExpressiveInterpretation::neutral();
    float fmFluteBaseGain = 0.0f;
    const int fmFlutePhraseLength = 5 + static_cast<int>(cfg.seed % 5u);
    int fmFlutePhraseIndex = 0;
    int fmFluteNoteCount = 0;
    size_t fmFluteNoteSamplesRemaining = 0;
    size_t fmFluteNoteTotalSamples = 0;
    size_t fmFluteNoteSamplesElapsed = 0;
    bool fmFlutePitchTelemetryInitialized = false;
    int fmFlutePreviousTelemetryNote = 0;
    int fmFluteCurrentRepeatedNotes = 0;
    int fmFlutePitchChangeCount = 0;
    int fmFluteRepeatedPitchCount = 0;
    int fmFluteLongestRepeatedNotes = 0;
    int fmFlutePitchMin = 0;
    int fmFlutePitchMax = 0;
    std::vector<DissonanceNoteEvent> fmFlutePendingEvents;
    size_t fmFluteNextEventIdx = 0;
    int fmFlutePreviousOffset = 0;
    int fmFluteRootForNote = pieceRootMidiNote;
    if (fmFluteEnabled) {
        auto movement = static_cast<HarmonicMovement>(
            static_cast<int>(unit(rng) * static_cast<int>(HarmonicMovement::Count)) % static_cast<int>(HarmonicMovement::Count));
        harmonicWanderer.configure(movement, pieceRootMidiNote, initialScale, cfg.seed + 71000u);
        double modulatorRatio = 0.98 + unit(rng) * 0.06; // near 1:1, slight per-piece detune
        double peakIndex = 2.0 + unit(rng) * 3.0;
        double sustainIndex = 0.2 + unit(rng) * 0.4;
        double ampAttack = 0.03 + unit(rng) * 0.08;
        double ampRelease = 0.15 + unit(rng) * 0.3;
        fmFlute.configure(modulatorRatio, peakIndex, sustainIndex, ampAttack, ampRelease);
        fmFluteBaseGain = static_cast<float>((harmonicProtagonist ? 0.42 : 0.26) + unit(rng) * (harmonicProtagonist ? 0.18 : 0.12));
        fmFlute.setGain(fmFluteBaseGain);

        auto voicingStyle = static_cast<VoicingStyle>(
            static_cast<int>(unit(rng) * static_cast<int>(VoicingStyle::Count)) % static_cast<int>(VoicingStyle::Count));
        voicingBank.configure(voicingStyle, pieceRootMidiNote, cfg.seed + 73000u);
        voicingBank.setGain(static_cast<float>((harmonicProtagonist ? 0.14 : 0.07) + unit(rng) * (harmonicProtagonist ? 0.12 : 0.07)));
        voicingBank.retune(harmonicWanderer.current());

        auto dissonanceStyle = static_cast<DissonanceStyle>(
            static_cast<int>(unit(rng) * static_cast<int>(DissonanceStyle::Count)) % static_cast<int>(DissonanceStyle::Count));
        dissonanceTreatment.configure(dissonanceStyle, cfg.seed + 75000u);
    }

    // Fase 111 -- phasing (Steve Reich, "Piano Phase"/"Clapping Music"):
    // a second voice reads the exact same live sequencer pattern
    // (sequencer.noteAt()/gateAt(), the very same notes_/gate_ content the
    // main voice plays) but walks through it on its own clock, very
    // slightly faster or slower than the piece's own tempo. Both voices
    // start in unison; the phase relationship between them then drifts
    // continuously through every offset within the pattern and (for most
    // seeds, within a typical piece's length) back toward alignment --
    // Reich's own signature technique, built from two performers
    // deliberately disagreeing about tempo, not from any harmonic or
    // rhythmic device already in this engine. Deliberately distinct from
    // tempoDriftEnabled (fase 28), which moves the WHOLE piece's tempo
    // together -- here the main sequencer clock is untouched; only this
    // second, independent read-head runs at a different rate. Reads most
    // cleanly on pieces the sequencer's own Turing-Machine lockAmount_
    // (see StepSequencer.hpp) happens to keep fairly static bar to bar --
    // an emergent synergy, not a coupling this code forces.
    bool phasingEnabled = unit(rng) < 0.3;
    double phasingDelta = 0.0;
    if (phasingEnabled) {
        double magnitude = 0.006 + unit(rng) * 0.019; // 0.6%-2.5% off the main clock
        phasingDelta = (unit(rng) < 0.5 ? -1.0 : 1.0) * magnitude;
    }
    Oscillator phaseVoice; phaseVoice.setSampleRate(sampleRate);
    phaseVoice.setWaveform(Oscillator::Waveform::Sine);
    float phaseVoiceEnv = 0.0f;
    double phaseVoiceDecay = decayCoeff(0.25 + unit(rng) * 0.35, sampleRate);
    float phaseVoiceGain = static_cast<float>(0.16 + unit(rng) * 0.08);
    int phasingEventCount = 0;

    // Fase 112 -- microtonalidade/afinacao justa (task #3): reafina a voz
    // principal (baseFreq, ver mais abaixo -- ja compartilhado com walsh/
    // vosim/hard-sync-mestre, entao reafinar so este ponto ja reafina os
    // quatro) pra um sistema de altura genuinamente diferente de 12-TET.
    // Ver MicrotonalTuning.hpp pra citacoes/matematica completa: afinacao
    // justa 5-limit e pitagorica (com deriva real de comma acumulada nas
    // modulacoes tonais genuinas da peca), grade de quarto-de-tom 24-EDO,
    // e uma contribuicao original -- sieve de Xenakis (a mesma tecnica de
    // SieveScale.hpp) estendida pra uma divisao da oitava mais fina que
    // 12, gerando uma inflexao microtonal unica por peca. Distinto de
    // freeIntonationEnabled (deriva caotica nao-estruturada, "errar a
    // nota") -- aqui a afinacao inteira do centro tonal muda de forma
    // coerente e historicamente real, nao caotica.
    bool microtonalEnabled = identity.spectral.microtonal;
    MicrotonalTuning microtonalTuning;
    if (microtonalEnabled) {
        auto microtonalSystem = static_cast<TuningSystem>(
            static_cast<int>(unit(rng) * static_cast<int>(TuningSystem::Count)) % static_cast<int>(TuningSystem::Count));
        microtonalTuning.configure(microtonalSystem, cfg.seed + 77000u);
    }

    // Fase 113 -- objeto espectral (task #3, terceiro item do territorio
    // conceitual musical): pedido explicito do usuario de aprofundar
    // espectralismo de forma original -- nao "pegar espectro e virar
    // acorde", mas tratar as mesmas proporcoes numericas de uma serie de
    // parciais como fonte simultanea de altura E ritmo. Ver
    // SpectralObject.hpp pra citacoes/matematica completa dos tres eixos
    // (harmonicidade<->inarmonicidade real via Fletcher & Rossing, buracos
    // de sieve de Xenakis reaproveitando a mesma tecnica das fases 61/112,
    // ritmo espectral -- o parcial p soa p vezes mais rapido que o
    // parcial 1, mesma proporcao que define sua altura). Fundamental duas
    // oitavas abaixo da tonica da peca, mesmo registro grave/fundacional
    // da voz de tuba.
    bool spectralObjectEnabled = selectionTrace.resolve(StudioModule::Spectral, identity.spectral.spectralObject);
    SpectralObject spectralObject;
    int spectralObjectRhythmBars = 1 + static_cast<int>(unit(rng) * 3.0); // 1..3 compassos por ciclo do parcial 1
    if (spectralObjectEnabled) {
        spectralObject.setSampleRate(sampleRate);
        auto spectralSet = static_cast<SpectralObject::PartialSet>(
            static_cast<int>(unit(rng) * static_cast<int>(SpectralObject::PartialSet::Count)) % static_cast<int>(SpectralObject::PartialSet::Count));
        double spectralFundamentalHz = midiToFreq(pieceRootMidiNote - 24);
        spectralObject.configure(spectralSet, spectralFundamentalHz, cfg.seed + 79000u);
        spectralObject.setGain(static_cast<float>((spectralProtagonist ? 0.28 : 0.13) + unit(rng) * (spectralProtagonist ? 0.18 : 0.09)));
    }

    // Fase 116 -- rasgo espectral real (task #7, territorio conceitual de
    // ruptura e rasgo). Ver RasgoFracture.hpp pra citacoes/matematica
    // completa: um Fiber Bundle Model real (Pierce 1926, Daniels 1945)
    // com local load sharing (Harlow & Phoenix 1978) sobre os 6 parciais
    // do objeto espectral -- cada parcial e uma fibra com resistencia
    // real (distribuicao de Weibull), uma tensao ambiente cresce com a
    // proximidade do climax, e quando uma fibra rasga, a carga liberada
    // vai so pras vizinhas imediatas -- pode desencadear avalanches reais
    // de ruptura se propagando pelo feixe. So faz sentido junto do objeto
    // espectral (usa suas frequencias de parcial reais via
    // partialFrequency()).
    bool rasgoFractureEnabled = spectralObjectEnabled && unit(rng) < 0.4;
    RasgoFracture rasgoFracture;
    std::array<Oscillator, RasgoFracture::kNumFibers> rasgoTearOsc;
    std::array<Envelope, RasgoFracture::kNumFibers> rasgoTearEnv;
    double rasgoAmbientStressGain = 0.06 + unit(rng) * 0.06;
    int rasgoTotalBreaksReported = 0;
    if (rasgoFractureEnabled) {
        rasgoFracture.configure(cfg.seed + 85000u);
        for (int fi = 0; fi < RasgoFracture::kNumFibers; ++fi) {
            rasgoTearOsc[static_cast<size_t>(fi)].setSampleRate(sampleRate);
            rasgoTearOsc[static_cast<size_t>(fi)].setWaveform(Oscillator::Waveform::Noise);
            rasgoTearOsc[static_cast<size_t>(fi)].seedNoise(cfg.seed + 85100u + static_cast<unsigned>(fi));
            rasgoTearEnv[static_cast<size_t>(fi)].setSampleRate(sampleRate);
            // Ataque brusco/decaimento curto, mas com loop real ligado --
            // uma vez que uma fibra rasga, essa envelope nunca mais para
            // de se auto-retriggerar, um deshiar continuo e permanente
            // (a "cicatriz" do vocabulario do usuario: nunca volta ao
            // estado anterior). setLoopEnabled ja existe no motor desde a
            // fase do Polivoks EG.
            rasgoTearEnv[static_cast<size_t>(fi)].setADSR(0.002, 0.08 + unit(rng) * 0.15, 0.0, 0.02);
        }
    }

    // Fase 117 -- ruptura por excesso de coerencia (task #7, segundo
    // eixo de ruptura/rasgo -- distinto do rasgo espectral da fase 116,
    // a pedido explicito do usuario de manter ruptura e rasgo como
    // contribuicoes separadas). Ver CoherenceCollapse.hpp pra citacoes/
    // matematica completa: entropia real de Shannon sobre as ultimas 24
    // notas realmente tocadas -- quando a peca fica coerente/repetitiva
    // demais por barras consecutivas reais, ela sofre um colapso real:
    // um estalo audivel (ruido com glide descendente) mais uma
    // reorganizacao forcada e temporaria do sequencer (mais densa,
    // registro extremo) por algumas barras, reusando so a API publica ja
    // existente de StepSequencer (regenerate/setRegisterOffset), sem
    // tocar no motor interno da classe.
    bool coherenceCollapseEnabled = unit(rng) < gamePlan.ruleChangeProbability(0.18);
    CoherenceCollapse coherenceCollapse;
    Oscillator coherenceCrackOsc; coherenceCrackOsc.setSampleRate(sampleRate);
    coherenceCrackOsc.setWaveform(Oscillator::Waveform::Noise);
    Envelope coherenceCrackEnv; coherenceCrackEnv.setSampleRate(sampleRate);
    int coherenceReorgBarsRemaining = 0;
    int coherenceCollapseAudibleCount = 0;
    if (coherenceCollapseEnabled) {
        double entropyThreshold = 0.9 + unit(rng) * 0.6; // bits -- abaixo disso, "coerente demais" (maximo teorico e log2(12)~3.58)
        int sustainedBarsRequired = 3 + static_cast<int>(unit(rng) * 4.0); // 3..6 barras seguidas
        int cooldownBars = 8 + static_cast<int>(unit(rng) * 8.0); // 8..15 barras antes do proximo colapso possivel
        coherenceCollapse.configure(entropyThreshold, sustainedBarsRequired, cooldownBars, cfg.seed + 87000u);
        coherenceCrackEnv.setADSR(0.001, 0.35, 0.0, 0.15);
        coherenceCrackOsc.seedNoise(cfg.seed + 87100u);
    }

    // Fase 114 -- metrica como sistema de organizacao do tempo (task #3,
    // quarto item): pedido explicito do usuario de pensar metrica alem do
    // modalismo armenio -- "quantos ciclos estao acontecendo
    // simultaneamente?" em vez de "qual compasso usar?". Ver
    // PolymeterEnsemble.hpp pra citacoes/matematica completa: polimetria
    // real (quatro vozes com comprimentos de ciclo coprimos genuinos, nao
    // subdivisoes XOR de um unico relogio como o polyrhythm divider ja
    // existente) reencontrando-se num macroperiodo real, mais pulso nao
    // isocrono dentro de cada ciclo (Justin London -- intervalos livres
    // somando exatamente o comprimento do ciclo, distinto das celulas
    // fixas 2/3 do aksak). Registro uma oitava abaixo da tonica, distinto
    // do objeto espectral (duas oitavas abaixo) e da voz principal.
    double polymeterChance = rhythmicProtagonist
        ? std::max(0.58, gamePlan.ruleChangeProbability(0.42))
        : gamePlan.ruleChangeProbability(0.11);
    bool polymeterEnabled = identityUnit(cfg.seed, 83) < polymeterChance;
    PolymeterEnsemble polymeterEnsemble;
    if (polymeterEnabled) {
        polymeterEnsemble.setSampleRate(sampleRate);
        polymeterEnsemble.configure(pieceRootMidiNote - 12, cfg.seed + 81000u);
        polymeterEnsemble.setGain(static_cast<float>(rhythmicProtagonist
            ? (0.22 + unit(rng) * 0.16)
            : (0.10 + unit(rng) * 0.08)));
    }

    // Fase 115 -- jogo como campo de forcas: blefe e confianca (task #6,
    // primeiro item do territorio conceitual de jogo). Ver
    // BluffSignal.hpp pra citacoes/matematica completa: um sinal soa
    // identico toda vez que dispara (informacao genuinamente incompleta),
    // decidido internamente genuino ou blefe por uma taxa de veracidade
    // real fixada por peca; um sinal genuino e seguido, apos uma janela
    // real de expectativa, por um segundo gesto real de confirmacao; um
    // blefe nao e seguido por nada. Uma terceira voz "respondente" reage
    // a CADA sinal novo (genuino ou blefe -- ela nao sabe qual ainda) com
    // um ganho real proporcional a confianca ja acumulada ate aquele
    // momento, e se recusa a responder de vez depois de uma traicao real
    // (confianca que foi genuinamente construida e depois desabou).
    bool bluffSignalEnabled = unit(rng) < std::clamp(
        0.10 + 0.42 * gamePlan.hiddenInformation, 0.0, 0.72);
    BluffSignal bluffSignal;
    Oscillator bluffSignalOsc; bluffSignalOsc.setSampleRate(sampleRate);
    bluffSignalOsc.setWaveform(Oscillator::Waveform::Pulse);
    Envelope bluffSignalEnv; bluffSignalEnv.setSampleRate(sampleRate);
    Oscillator bluffConfirmOsc; bluffConfirmOsc.setSampleRate(sampleRate);
    bluffConfirmOsc.setWaveform(Oscillator::Waveform::Sine);
    Envelope bluffConfirmEnv; bluffConfirmEnv.setSampleRate(sampleRate);
    Oscillator bluffRespondentOsc; bluffRespondentOsc.setSampleRate(sampleRate);
    bluffRespondentOsc.setWaveform(Oscillator::Waveform::Saw);
    Envelope bluffRespondentEnv; bluffRespondentEnv.setSampleRate(sampleRate);
    double bluffTruthfulness = 0.7;
    double bluffSignalChancePerBar = 0.12 + unit(rng) * 0.18;
    int bluffResolveDelayBars = 2 + static_cast<int>(unit(rng) * 3.0);
    double bluffEmaAlpha = 0.3 + unit(rng) * 0.3;
    double bluffBetrayalThreshold = 0.25 + unit(rng) * 0.15;
    bool bluffPendingActive = false;
    size_t bluffResolveCountdownSamples = 0;
    float bluffRespondentGain = 0.5f;
    if (bluffSignalEnabled) {
        bluffTruthfulness = cfg.bluffTruthfulnessOverride ? cfg.bluffTruthfulnessOverrideValue : (0.25 + unit(rng) * 0.7);
        bluffSignal.configure(bluffTruthfulness, cfg.seed + 83000u);
        bluffSignalEnv.setADSR(0.004, 0.12, 0.0, 0.05);
        bluffConfirmEnv.setADSR(0.006, 0.25, 0.0, 0.1);
        bluffRespondentEnv.setADSR(0.008, 0.18, 0.0, 0.08);
        bluffSignalOsc.setFrequency(midiToFreq(pieceRootMidiNote + 31)); // duas oitavas + quinta, registro de "aviso"
        bluffConfirmOsc.setFrequency(midiToFreq(pieceRootMidiNote + 12)); // uma oitava, registro mais quente/grave
        bluffRespondentOsc.setFrequency(midiToFreq(pieceRootMidiNote + 24)); // duas oitavas, timbre distinto (saw)
    }

    bool tubaLead = identity.leadVoice == LeadVoiceModel::Tuba;
    bool tubaVoiceEnabled = selectionTrace.resolve(StudioModule::Tuba, tubaLead || identityUnit(cfg.seed, 71) < 0.10);
    ModalResonator tubaResonator; tubaResonator.setSampleRate(sampleRate);
    {
        std::array<double, ModalResonator::kNumModes> tubaRatios{}, tubaDecays{}, tubaGains{};
        for (int m = 0; m < ModalResonator::kNumModes; ++m) {
            tubaRatios[static_cast<size_t>(m)] = (m + 1) + (unit(rng) - 0.5) * 0.03;
            // Real bug found empirically (debug instrumentation comparing
            // this voice's own mean contribution against the piece's final
            // mixed output): with the long percussive-strike decay times
            // used elsewhere in this engine (seconds-long, correct for a
            // single mallet hit that never gets excited again before it
            // rings out), CONTINUOUS per-sample breath excitation pumps in
            // new energy faster than that slow decay can dissipate it --
            // the resonator was creeping toward its own safety ceiling
            // (fabs > 50) for the whole duration of a note, not settling
            // into a stable tone, and forcing the master limiter to
            // compress the ENTIRE mix hard to tame it (measured ~25x
            // louder, on average, than the final output of the whole
            // piece -- not a subjective "too quiet" complaint, a real
            // near-runaway). A continuously-excited resonator needs a much
            // shorter time constant than a struck one -- physically, a real
            // blown pipe loses energy to radiation/friction constantly,
            // reaching a stable steady-state loudness, not an ever-longer
            // ring. Shortened to the same order of magnitude a real
            // sustained wind tone settles at.
            tubaDecays[static_cast<size_t>(m)] = 0.06 / (m + 1.0) + unit(rng) * 0.015;
            tubaGains[static_cast<size_t>(m)] = 1.0 / ((m + 1.0) * (m + 1.0));
        }
        tubaResonator.setModes(tubaRatios, tubaDecays, tubaGains);
        tubaResonator.setBrightness(0.35f);
    }
    BreathExciter tubaBreath;
    tubaBreath.seed(cfg.seed + 28000u);
    tubaBreath.setSampleRate(sampleRate);
    tubaBreath.setFilterCutoff(500.0 + unit(rng) * 500.0);
    tubaBreath.setAttackRelease(0.25 + unit(rng) * 0.4, 0.6 + unit(rng) * 0.8);
    double tubaExciteAmount = 0.5 + unit(rng) * 0.4;
    double tubaMix = tubaVoiceEnabled
        ? ((tubaLead ? 0.58 : 0.22) + unit(rng) * (tubaLead ? 0.34 : 0.24)) : 0.0;
    int tubaOctaveDown = (unit(rng) < 0.5) ? 2 : 3;

    // Fases 177--180 -- Bass Behaviour Engine. It remains behind the same
    // expressive A/B switch: normal mode keeps the legacy acid pattern and
    // tuba root/breath behaviour byte-for-byte on their former path. In the
    // expressive branch, function -> phrase -> performance -> body is planned
    // at a fixed 16-step bass-cycle rate. Tuba and acid remain optional; a
    // restrained sine/second-harmonic core-sub body prevents their absence
    // from making every non-silent bass function inaudible.
    const bool bassBehaviourEnabled = cfg.expressiveActuationEnabled;
    BassPhraseHistory bassPhraseHistory;
    BassBodyPlan bassBodyPlan;
    BassBodyDirective bassPendingDirective;
    BassBodyDirective bassEarlyPendingDirective;
    BassBodyDirective bassTubaDirective;
    BassBodyDirective bassAcidDirective;
    BassBodyDirective bassCoreSubDirective;
    std::array<bool, BassFunctionPlan::kMaxSteps> bassKickMemory {
        true, false, false, false, true, false, false, false,
        true, false, false, false, true, false, false, false
    };
    std::array<bool, BassFunctionPlan::kMaxSteps> bassKickCapture{};
    PerformerState bassPerformer;
    bassPerformer.energy = std::clamp(gamePlan.energy, 0.0, 1.0);
    bassPerformer.effort = std::clamp(0.34 + gamePlan.energy * 0.38, 0.0, 1.0);
    bassPerformer.confidence = std::clamp(0.48 + gamePlan.memory * 0.34, 0.0, 1.0);
    bassPerformer.precision = std::clamp(0.50 + gamePlan.memory * 0.28, 0.0, 1.0);
    bassPerformer.tension = std::clamp(gamePlan.risk * 0.62, 0.0, 1.0);
    bassPerformer.risk = std::clamp(gamePlan.risk, 0.0, 1.0);
    bassPerformer.expressivity = std::clamp(0.42 + gamePlan.freedom * 0.42, 0.0, 1.0);
    BassExecutionFeedback bassExecutionFeedback(
        bassBehaviourEnabled && cfg.musicalDirectionEnabled
            && cfg.bassExecutionFeedbackEnabled, sampleRate);
    bool bassFeedbackPhraseBoundary = false;
    BassKickInteraction bassKickInteraction(cfg.expressiveActuationEnabled
        && cfg.musicalDirectionEnabled && cfg.bassKickInteractionEnabled, sampleRate);
    int bassPendingDelaySamples = -1;
    int bassEarlyPendingDelaySamples = -1;
    std::array<bool, BassBodyPlan::kMaxSteps> bassStepScheduledEarly{};
    int bassTubaGateSamples = 0;
    int bassCoreSubGateSamples = 0;
    int bassCycleCount = 0;
    int bassPlanCount = 0;
    int bassEventCount = 0;
    int bassTubaEventCount = 0;
    int bassAcidEventCount = 0;
    int bassCoreSubEventCount = 0;
    int bassGrooveApplicationCount = 0;
    int drumGrooveApplicationCount = 0;
    TemporalGrooveImprint drumGroove;
    GrooveDialogue grooveDialogue(cfg.seed,
        cfg.expressiveActuationEnabled && cfg.musicalDirectionEnabled
            && cfg.grooveDialogueEnabled, cfg.grooveSubdivisionsEnabled);
    int bassEarlyTimingClampCount = 0;
    int bassBodyRootMidi = pieceRootMidiNote;
    BassFunctionRole bassFinalRole = BassFunctionRole::Silence;
    BassBodyKind bassFinalBody = BassBodyKind::None;

    Oscillator bassCoreSubFundamental;
    bassCoreSubFundamental.setSampleRate(sampleRate);
    bassCoreSubFundamental.setWaveform(Oscillator::Waveform::Sine);
    Oscillator bassCoreSubHarmonic;
    bassCoreSubHarmonic.setSampleRate(sampleRate);
    bassCoreSubHarmonic.setWaveform(Oscillator::Waveform::Sine);
    Envelope bassCoreSubEnvelope;
    bassCoreSubEnvelope.setSampleRate(sampleRate);
    BassPhraseTimbre bassPhraseTimbre(cfg.seed,
        bassBehaviourEnabled && cfg.musicalDirectionEnabled
            && cfg.musicalRelationsEnabled && cfg.bassPhraseTimbreEnabled,
        sampleRate);

    // Fase 97 -- "Galo" (see GaloVoice.hpp): a stylized synthesized rooster
    // call, honestly labeled as synthesis, not a bioacoustic model. Reuses
    // ModalResonator (fase 43/78) and BreathExciter (the tuba voice's own
    // continuous exciter) with a real frequency-contour chirp -- rising
    // syllables, falling tail on the last one. Deliberately foreign to the
    // piece's own scale/register (a found-sound-like intrusion, same
    // "elemento estranho" spirit), fires at most once per piece.
    bool galoEnabled = selectionTrace.resolve(StudioModule::Galo, unit(rng) < 0.22);
    bool galoFired = false;
    GaloVoice galo; galo.setSampleRate(sampleRate);
    {
        std::array<double, ModalResonator::kNumModes> galoRatios{}, galoDecays{}, galoGains{};
        for (int m = 0; m < ModalResonator::kNumModes; ++m) {
            // More inharmonic than the tuba's near-integer ratios --
            // reads as a harsh, birdlike/nasal cluster rather than a
            // clean brass-like harmonic series.
            galoRatios[static_cast<size_t>(m)] = (m + 1) * (1.0 + 0.06 * (m % 2 == 0 ? 1.0 : -1.0)) + unit(rng) * 0.05;
            galoDecays[static_cast<size_t>(m)] = 0.05 / (m + 1.0) + unit(rng) * 0.01;
            galoGains[static_cast<size_t>(m)] = 1.0 / ((m + 1.0) * (m + 1.0));
        }
        galo.configure(cfg.seed + 71000u, galoRatios, galoDecays, galoGains);
    }
    double galoBaseFreq = 500.0 + unit(rng) * 400.0;
    double galoCallDuration = 1.1 + unit(rng) * 0.6;
    double galoFireChancePerBar = 0.015 + unit(rng) * 0.02;

    // Fase 98 -- voz de scat (see ScatVoice.hpp): real source-filter vowel-
    // formant synthesis, tuned to Peterson & Barney's documented real
    // vowel-formant values (unlike Atropelo, fase 90, whose formants are
    // deliberately shifted OUTSIDE real vowel space) -- wordless,
    // meaning-free scat syllables, not intelligible speech (that would
    // need a real voice recording this project doesn't have). Only active
    // alongside a genuine chord progression, same gating convention
    // Harmonaig already uses, so every syllable it sings genuinely
    // outlines this piece's own harmony.
    bool scatEnabled = selectionTrace.resolve(StudioModule::Scat, sequencer.chordProgressionEnabled()
        && (callResponseArrangement || identity.harmonicLanguage == HarmonicLanguage::Jazz
            || identityUnit(cfg.seed, 73) < 0.08), sequencer.chordProgressionEnabled(), "chord-progression");
    ScatVoice scat; scat.setSampleRate(sampleRate);
    scat.seed(cfg.seed + 81000u);
    double scatFireChancePerStep = 0.06 + unit(rng) * 0.08;
    int scatEventCount = 0;

    // Fase 99 -- "Coral" polifonico (see ChoirEnsemble.hpp): real
    // polyphony -- four ScatVoice instances in SATB-like registers, each
    // voice-led into its own chord tone (real vertical harmony, not
    // doubling), triggered together on the same shared vowel (reads as
    // ONE choir) but each carrying its own onset-timing/dynamics jitter
    // (the real "imperfect ensemble" a human choir has). Independent roll
    // from the scat solo voice above, same chord-progression gating.
    // Deliberately mutually possible alongside Harmonaig and the scat
    // solo voice -- a fired choir phrase happens on a genuine chord
    // change, same trigger point Harmonaig already uses, but only a
    // fraction of the time so it reads as the choir joining in, not glued
    // to every single chord.
    bool choirEnabled = selectionTrace.resolve(StudioModule::Choir, sequencer.chordProgressionEnabled()
        && (polyphonicArrangement || (ensembleLead && identityUnit(cfg.seed, 74) < 0.5)
            || identityUnit(cfg.seed, 75) < 0.05), sequencer.chordProgressionEnabled(), "chord-progression");
    ChoirEnsemble choir; choir.setSampleRate(sampleRate);
    choir.seed(cfg.seed + 91000u);
    choir.setRegisterOffsets({12, 5, -5, -12});
    double choirPhraseChance = 0.4 + unit(rng) * 0.3;
    int choirPhraseCount = 0;

    // Fase 101 -- "deja vu" (see DejaVuVoice.hpp): synthesized false
    // memory built from running statistics (pitch/brightness/density),
    // never a real quotation of anything that actually happened --
    // distinct from ApocryphalTear (fase 25), which replays a real, if
    // distorted, captured segment. Unlike the piece's other "single real
    // event" mechanisms (curto-circuito, galo), a piece can genuinely
    // have more than one deja vu -- the sensation itself recurs in real
    // life, so a minimum gap between occurrences stands in for "once per
    // piece" rarity instead of a hard one-shot.
    bool dejaVuEnabled = selectionTrace.resolve(StudioModule::DejaVu, unit(rng) < 0.25);
    DejaVuVoice dejaVu; dejaVu.setSampleRate(sampleRate);
    double dejaVuFireChancePerBar = 0.02 + unit(rng) * 0.03;
    int dejaVuBarsSinceLast = 999;
    int dejaVuMinGapBars = 6 + static_cast<int>(unit(rng) * 8.0);
    int dejaVuEventCount = 0;

    // Instruo and Ben "DivKid" Wilson's ochd: eight rate-related,
    // slowly-drifting LFOs
    // (see OchdBank.hpp) giving a few previously-static parameters -- the
    // resonator's brightness and the reverb's damping, both just set once
    // above/below and never touched again -- a slow, organic "breathing"
    // instead of a fixed value for the whole piece. Not every piece gets
    // this (some genuinely benefit from a static resonator/reverb
    // character), so it's a seed-derived choice like voiceColor/loFiDelay,
    // not a universal addition.
    bool ochdEnabled = unit(rng) < 0.6;
    OchdBank ochd; ochd.setSampleRate(sampleRate);
    ochd.configure(0.02 + unit(rng) * 0.04, cfg.seed + 18000u);
    float ochdBrightnessDepth = ochdEnabled ? static_cast<float>(0.1 + unit(rng) * 0.25) : 0.0f;
    float ochdDampingDepth = ochdEnabled ? static_cast<float>(0.05 + unit(rng) * 0.15) : 0.0f;

    // A granular wash of the piece's own immediate past (the idea behind
    // Mutable Instruments' Clouds, simplest granular-delay mode). Its mix
    // amount tracks the arc's density inversely below -- barely audible
    // while things are busy, more present as a section thins out, so
    // quieter/closing moments read as the piece dissolving into grains of
    // itself rather than just going quiet.
    GranularCloud granular; granular.setSampleRate(sampleRate, 1.5);
    // Noise ecology gets its own deterministic stream. Adding a texture
    // choice must not silently move later melodic, harmonic or drum rolls.
    std::mt19937 noiseVariantRng(cfg.seed ^ 0x6d2b79f5u);
    std::uniform_real_distribution<float> noiseVariantUnit(0.0f, 1.0f);
    granular.seed(cfg.seed + 9000u);
    granular.setDensity(5.0);
    granular.setGrainDuration(0.05, 0.16);
    granular.setSpread(0.7);
    granular.setPitchJitter(5.0);
    // Texture presence is a compositional decision. The processors still
    // advance when muted where their internal memory needs warming, but a
    // given piece is not obliged to carry every noise-like layer throughout
    // its whole form.
    bool granularTextureEnabled = selectionTrace.resolve(StudioModule::Granular, noiseVariantUnit(noiseVariantRng) < 0.82f);
    bool gendynTextureEnabled = selectionTrace.resolve(StudioModule::Gendyn, noiseVariantUnit(noiseVariantRng) < 0.68f);
    bool coloredNoiseBedEnabled = selectionTrace.resolve(StudioModule::NoiseBed, noiseVariantUnit(noiseVariantRng) < 0.72f);

    // GendynOscillator (Xenakis's Dynamic Stochastic Synthesis, see the
    // phase-12 note at the top of this file): a continuous, self-mutating
    // background texture, mixed in very quietly and more so as a section
    // thins out -- the same density-driven logic as the granular wash just
    // above, since both read as "the piece dissolving into its own texture"
    // during quieter passages, just via two structurally different
    // synthesis techniques.
    GendynOscillator gendyn; gendyn.setSampleRate(sampleRate);
    gendyn.seed(cfg.seed + 12000u);
    gendyn.configure(6 + static_cast<int>(unit(rng) * 6.0), 0.01, 0.08);
    gendyn.setStepScale(0.15f, 0.15f);
    gendyn.setDistribution(GendynOscillator::Distribution::Cauchy);

    // Frap Tools Sapel's dedicated noise colors -- a third, spectrally
    // distinct flavour of "dissolving into texture" alongside the granular
    // wash and Gendyn drone above: pink/red/blue noise each sit in a
    // different part of the spectrum than this engine's existing white-noise
    // uses (snare/clap/maracas), so this reads as a genuinely separate low
    // hiss/rumble/shimmer bed rather than more of the same. One color per
    // piece, seed-drawn, mixed in at the same density-inverse curve.
    ColoredNoise colorNoise;
    colorNoise.seed(cfg.seed + 15000u);
    double noiseColorRoll = unit(rng);
    ColoredNoise::Color chosenNoiseColor = noiseColorRoll < 0.34 ? ColoredNoise::Color::Pink
                                          : noiseColorRoll < 0.60 ? ColoredNoise::Color::Red
                                          : noiseColorRoll < 0.80 ? ColoredNoise::Color::Blue
                                                                  : ColoredNoise::Color::White;
    colorNoise.setColor(chosenNoiseColor);
    ColoredNoise::Color activeNoiseColor = chosenNoiseColor;
    int noiseColorChangeCount = 0;
    const auto noiseColorName = [](ColoredNoise::Color color) {
        return color == ColoredNoise::Color::Pink ? "pink"
             : color == ColoredNoise::Color::Red ? "red"
             : color == ColoredNoise::Color::Blue ? "blue" : "white";
    };

    // The last two software-side circuit ports that had never actually
    // been wired into the main patch (see the plan document's "em aberto"
    // list) -- both get a real, modest role rather than existing just to
    // say they're used.
    //
    // AtariPunkConsole (Forrest Mims' Stepped Tone Generator): a glitchy
    // stepped-tone texture, clocked a sub-octave below the current note so
    // it has some tonal relationship to the piece rather than reading as
    // pure noise. Mixed in following the same resonance-driven curve as
    // the ring modulator/AY8910 arpeggio -- inaudible in a quiet opening,
    // a distinct lo-fi "circuit-bent" texture at the climax.
    AtariPunkConsole atariPunk; atariPunk.setSampleRate(sampleRate);
    atariPunk.setPulseWidth(0.003);

    // Fase 109 -- Atari Punk Console wild clock (task #30, user
    // suggestion: "atari punk console (entre outros) pode seguir esse
    // tipo de musicalidade imprevisivel"). Same chaos-driven-EVENT idiom
    // as the sequencer's wild timing (fase 107) -- normally this voice's
    // clock tracks the melody's pitch smoothly (baseFreq*0.5, see below);
    // with this on, a real StandardMap zero-crossing occasionally forces
    // the clock to a real division/multiple of that (0.25x-2x) for a
    // short window before it settles back -- the actual, documented
    // behaviour of the real circuit this models (Forrest Mims' Stepped
    // Tone Generator, two 555 timers) when pushed hard enough that the
    // clock itself hiccups, not a smooth glide to a wrong pitch.
    const bool atariVoiceEnabled = selectionTrace.resolve(StudioModule::AtariPunk, true);
    bool atariPunkGlitchEnabled = unit(rng) < 0.3;
    if (!atariVoiceEnabled) atariPunkGlitchEnabled = false;
    StandardMap atariPunkGlitchChaos;
    atariPunkGlitchChaos.setSampleRate(1.0);
    atariPunkGlitchChaos.setRate(0.4 + unit(rng) * 0.5);
    atariPunkGlitchChaos.setParams(1.8 + unit(rng) * 1.7);
    atariPunkGlitchChaos.setInitial(unit(rng) * 6.28, unit(rng) * 6.28);
    float atariPunkGlitchPrevValue = 0.0f;
    bool atariPunkGlitchHasPrev = false;
    double atariPunkGlitchMult = 1.0;
    size_t atariPunkGlitchSamplesRemaining = 0;
    int atariPunkGlitchCount = 0;
    static const double kAtariPunkGlitchRatios[] = {0.25, 0.75, 1.5, 2.0, 0.5};

    // EnvelopeOscillator ("AD/AR as VCO"): an octave-up shimmer layered
    // very quietly under the main voice, its attack/release balance swept
    // by a chaos value already running (Latoocarfian) so the shimmer's
    // own timbre keeps drifting rather than sitting on one fixed shape.
    EnvelopeOscillator envOsc; envOsc.setSampleRate(sampleRate);

    // WalshOscillator (see WalshOscillator.hpp): a genuinely different
    // fourth synthesis principle for this engine -- additive combination
    // of orthogonal +-1 Walsh functions instead of sinusoids, interpolated
    // breakpoints, or a shape-morph crossfade. Seed-gated like the tuba
    // voice above (not every piece gets it), tracks the main voice's own
    // pitch (unison), and stays a background colour under the melody
    // rather than a second lead -- the same "radical in principle, gentle
    // in the mix" discipline as every other digitally-harsh module here.
    // Not one of Agon's four contesting voices (main/atari/gendyn/tuba is
    // a fixed, already-named quartet throughout this engine's own
    // metadata) -- density-driven instead, same bucket as colorNoise.
    bool walshOscEnabled = selectionTrace.resolve(StudioModule::Walsh, unit(rng) < market.price("walshOscillator", 0.3));
    market.record("walshOscillator", walshOscEnabled);
    WalshOscillator walshOsc; walshOsc.setSampleRate(sampleRate);
    {
        // 2-7 active sequencies, decaying weight with seed-jittered
        // amounts -- the same "which harmonics, how loud" role amplitude
        // weights play in ordinary additive synthesis, just over a
        // rectangular basis instead of a sinusoidal one.
        int walshVoices = 2 + static_cast<int>(unit(rng) * 6.0);
        std::vector<double> walshWeights(static_cast<size_t>(walshVoices));
        for (size_t k = 0; k < walshWeights.size(); ++k) {
            walshWeights[k] = (1.0 / (static_cast<double>(k) + 1.0)) * (0.6 + unit(rng) * 0.8);
        }
        walshOsc.configure(walshWeights);
    }

    // Fase 72 -- VOSIM (see VosimOscillator.hpp): a sixth, genuinely
    // distinct synthesis principle -- discrete decaying sin^2 pips
    // instead of a continuously-excited resonator (ModalResonator/tuba)
    // or any of the other five techniques already in this engine. Tracks
    // the main voice's own pitch (unison, like Walsh above) so it reads
    // as a ghostly formant-voice shadowing the melody, not an unrelated
    // texture -- the "voz narrante" idea from the same research batch,
    // minus the need for an actual speech recording this project doesn't
    // have. Same density-driven background-colour bucket as Walsh/Gendyn/
    // colorNoise, not one of Agon's four named voices.
    bool vosimEnabled = selectionTrace.resolve(StudioModule::Vosim, unit(rng) < 0.25);
    VosimOscillator vosim; vosim.setSampleRate(sampleRate);
    vosim.setFormant(350.0 + unit(rng) * 1400.0); // roughly vowel-formant territory
    vosim.setPulseCount(2 + static_cast<int>(unit(rng) * 5.0));
    vosim.setDecay(0.45 + unit(rng) * 0.4);

    // Fase 92 -- hard sync VCO (see HardSyncVoice.hpp): another genuinely
    // distinct synthesis principle -- forcibly resetting a "slave"
    // oscillator's phase every time a "master" locks it to the master's own
    // period, instead of any additive/breakpoint/formant/orthogonal-basis
    // technique already here. Master tracks the main voice's pitch in
    // unison (same convention as Walsh/VOSIM above) so the output's pitch
    // never fights the melody; only the slave/master ratio -- continuously
    // swept by an already-ticking chaos value, no new chaos state -- moves,
    // giving the real hard-sync sweep character. Same density-driven
    // background bucket as Walsh/VOSIM/Gendyn/colorNoise, not one of
    // Agon's four named voices.
    bool hardSyncEnabled = selectionTrace.resolve(StudioModule::HardSync, unit(rng) < 0.25);
    HardSyncVoice hardSync; hardSync.setSampleRate(sampleRate);
    hardSync.setSlaveWaveform(Oscillator::Waveform::Saw);
    double hardSyncRatioBase = 1.5 + unit(rng) * 2.5; // 1.5-4.0, classic sync-sweep territory
    double hardSyncRatioDepth = 0.4 + unit(rng) * 1.0;

    // Fase 73 -- batimento binaural caotico: hoje o estereo desta peca vem
    // so de pan (amplitude) e de caudas de delay/reverb diferentes -- nada
    // faz o TOM fundamental da voz principal divergir de verdade entre L e
    // R. Em vez de duplicar toda a cadeia mono ja validada (osc1/osc2/
    // diode mixer/filtro/resonator) pra dois canais, uma "sombra" binaural
    // aditiva -- dois osciladores senoidais simples, um por canal, cada um
    // so audivel no proprio canal -- roda em paralelo em uníssono com a
    // voz principal, com um pequeno desvio de frequencia entre L e R que o
    // proprio caos ja rodando (Latoocarfian) acelera e desacelera. O
    // cerebro do ouvinte tenta unificar as duas frequencias ligeiramente
    // diferentes e percebe uma pulsacao ritmica propria (o batimento
    // binaural de verdade), verificavel objetivamente no proprio WAV (a
    // divergencia real de frequencia entre os canais), diferente de
    // qualquer tecnica de largura estereo ja existente no motor.
    bool binauralEnabled = selectionTrace.resolve(StudioModule::Binaural, unit(rng) < 0.2);
    Oscillator binauralL; binauralL.setSampleRate(sampleRate); binauralL.setWaveform(Oscillator::Waveform::Sine);
    Oscillator binauralR; binauralR.setSampleRate(sampleRate); binauralR.setWaveform(Oscillator::Waveform::Sine);
    double binauralOffsetHz = 2.0 + unit(rng) * 13.0;

    // Fase 76 -- Vector Synthesis (Prophet VS-inspired 2D crossfade): the
    // last candidate from the curation, deliberately left for last since
    // it has more scope/risk than the others. Rather than retrofitting
    // the existing independently-mixed textures above (Gendyn/Walsh/
    // AtariPunk/colorNoise, each with its own enable flag and mix
    // envelope, already validated) into a shared crossfade -- real risk
    // of breaking already-tested behaviour for uncertain gain -- this is
    // a wholly separate, self-contained bank of four FRESH instances of
    // those same classes, dedicated only to this mechanism. Two already-
    // ticking chaos values (read-only reuse, no new state) act as the X/Y
    // position on a square; the four corners crossfade bilinearly instead
    // of each just fading in/out independently -- a genuinely different
    // modulation topology (continuous 2D position) from anything else in
    // this engine, at much lower risk than touching the existing blocks.
    bool vectorSynthEnabled = selectionTrace.resolve(StudioModule::Vector, unit(rng) < 0.2);
    GendynOscillator vsGendyn; vsGendyn.setSampleRate(sampleRate);
    vsGendyn.seed(cfg.seed + 37000u);
    vsGendyn.configure(6, 0.01, 0.08);
    vsGendyn.setStepScale(0.15f, 0.15f);
    vsGendyn.setDistribution(GendynOscillator::Distribution::Cauchy);
    WalshOscillator vsWalsh; vsWalsh.setSampleRate(sampleRate);
    vsWalsh.configure({1.0, 0.6, 0.35});
    AtariPunkConsole vsAtari; vsAtari.setSampleRate(sampleRate);
    vsAtari.setPulseWidth(0.003);
    ColoredNoise vsNoise; vsNoise.seed(cfg.seed + 38000u);
    vsNoise.setColor(ColoredNoise::Color::Pink);

    // Fase 77 -- Harmonaig-inspired chord pad (see CREDITS.md: Instruo
    // Harmonaig by Jason Lim, a real four-voice root/3rd/5th/7th chord
    // quantizer module). A dedicated, wholly separate bank of four fresh
    // oscillator+envelope voices (same low-risk pattern as Vector
    // Synthesis, fase 76) sounding the piece's own current chord as a
    // sustained pad -- root/third/fifth/seventh, matching the real
    // module's own voicing rather than a doubled triad, re-voiced every
    // time the harmony's own chord changes. Deliberately NOT built by
    // retuning osc1/osc2 (a fixed
    // cross-FM pair, not freely retunable without breaking that voice's
    // own character) or hocketOsc1/hocketOsc2 (deliberately sparse,
    // occasional voices, a different musical role). Additive layer, not a
    // replacement for the main melodic voice -- only rolled for pieces
    // that already have a real chord progression running (fixed or
    // Neo-Riemannian), since there is no "chord" to voice otherwise.
    bool harmonaigEnabled = selectionTrace.resolve(StudioModule::Harmonaig, sequencer.chordProgressionEnabled()
        && (polyphonicArrangement || ensembleLead || identityUnit(cfg.seed, 76) < 0.12),
        sequencer.chordProgressionEnabled(), "chord-progression");
    std::array<Oscillator, 4> harmonaigOsc;
    std::array<Envelope, 4> harmonaigEnv;
    for (size_t hv = 0; hv < 4; ++hv) {
        harmonaigOsc[hv].setSampleRate(sampleRate);
        harmonaigOsc[hv].setWaveform(Oscillator::Waveform::Sine);
        harmonaigEnv[hv].setSampleRate(sampleRate);
        // Slow attack/release -- a sustained pad, not a percussive hit;
        // each voice's own timing jittered a touch by seed so the four
        // voices don't all swell in lockstep.
        harmonaigEnv[hv].setADSR(0.4 + unit(rng) * 0.6, 0.2, 0.85, 1.2 + unit(rng) * 1.0);
    }
    // Seed-derived voicing, fixed for the whole piece (the real module's
    // own inversion/spread controls, here chosen once by the seed rather
    // than played live): which chord tone (0=root,1=third,2=fifth,3=seventh)
    // sits in the bass register (inversion), and whether the upper voices
    // spread across one octave (tight) or two (open).
    int harmonaigInversion = static_cast<int>(unit(rng) * 4.0) % 4;
    int harmonaigSpreadOctaves = (unit(rng) < 0.5) ? 1 : 2;
    int harmonaigChordCount = 0;

    // Fase 78 -- Xenakis sieve polyphony (2-4 independent channels,
    // elemental timbres): evolution of the fase-61 sieve scale
    // (SieveScale.hpp) from a single generated scale for the main melody
    // to several independent channels, each running its OWN sieve, its
    // own chaos-modulated clock, and its own distinct timbre -- wood/
    // breath, metal/glass (inharmonic ModalResonator), water (granular
    // texture, pitch-jitter standing in for a chirp), and a fourth
    // "plasma" channel whose own scale is the set complement (a real
    // NOT-sieve) of the union of the other three's pitch classes,
    // normally silent, triggered only when the three elemental channels'
    // currently-sounding pitch classes collide into "too consonant" an
    // interval set -- firing a tritone to break the predictability.
    // Contrapuntal texture emerges from statistical collision between
    // independently-clocked channels, not from written chords --
    // genuinely different in kind from the existing chord-progression/
    // Neo-Riemannian/Harmonaig harmonic layers, which all follow ONE
    // shared harmonic thread. Additive layer, own probability roll,
    // independent of Harmonaig (the two can coexist).
    bool sievePolyphonyEnabled = selectionTrace.resolve(StudioModule::Sieve, identity.spectral.sievePolyphony);

    SieveScale sieveWood = generateSieveScale(rng, cataloguedPitchClassSets());
    SieveScale sieveMetal = generateSieveScale(rng, cataloguedPitchClassSets());
    SieveScale sieveWater = generateSieveScale(rng, cataloguedPitchClassSets());

    // Plasma channel's own scale: the set complement of the union of the
    // other three's pitch classes -- a real NOT-sieve, not just a fourth
    // generated scale.
    ScaleTable sievePlasmaTable{};
    {
        std::set<int> unionPcs;
        for (int i = 0; i < sieveWood.table.length; ++i) unionPcs.insert(sieveWood.table.degrees[static_cast<size_t>(i)]);
        for (int i = 0; i < sieveMetal.table.length; ++i) unionPcs.insert(sieveMetal.table.degrees[static_cast<size_t>(i)]);
        for (int i = 0; i < sieveWater.table.length; ++i) unionPcs.insert(sieveWater.table.degrees[static_cast<size_t>(i)]);
        int fillIdx = 0;
        for (int p = 0; p < 12; ++p) {
            if (!unionPcs.count(p)) sievePlasmaTable.degrees[static_cast<size_t>(fillIdx++)] = p;
        }
        // Degenerate case: the union of three generated sieves happens to
        // cover all 12 pitch classes -- falls back to a plain tritone from
        // the root rather than an empty scale.
        if (fillIdx == 0) { sievePlasmaTable.degrees[0] = 6; fillIdx = 1; }
        for (int p = fillIdx; p < 12; ++p) sievePlasmaTable.degrees[static_cast<size_t>(p)] = sievePlasmaTable.degrees[static_cast<size_t>(fillIdx - 1)];
        sievePlasmaTable.length = fillIdx;
    }

    // Timbre engines, one wholly separate instance per channel (same low-
    // risk pattern as Vector Synthesis/Harmonaig): wood/breath reuses the
    // BreathExciter+ModalResonator continuous-excitation idea already
    // proven by the tuba voice (fase 43), tuned to near-harmonic ratios;
    // metal/glass is a ModalResonator alone with clearly inharmonic
    // "bell" ratios, excited by a single strike per note; water is a
    // plain sine tone fed continuously through a dedicated GranularCloud
    // instance, its own pitch-jitter standing in for a chirp -- an honest
    // approximation, not a new grain-chirp engine.
    BreathExciter sieveWoodBreath; sieveWoodBreath.seed(cfg.seed + 41000u);
    sieveWoodBreath.setSampleRate(sampleRate);
    sieveWoodBreath.setFilterCutoff(700.0);
    sieveWoodBreath.setAttackRelease(0.15, 0.4);
    ModalResonator sieveWoodBody; sieveWoodBody.setSampleRate(sampleRate);
    sieveWoodBody.setModes({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {1.4, 1.0, 0.7, 0.5, 0.35, 0.25}, {1.0, 0.5, 0.3, 0.2, 0.12, 0.08});
    sieveWoodBody.setBrightness(0.4f);

    ModalResonator sieveMetalBody; sieveMetalBody.setSampleRate(sampleRate);
    sieveMetalBody.setModes({1.0, 2.76, 4.07, 5.43, 6.86, 8.93}, {2.2, 1.6, 1.2, 0.9, 0.6, 0.4}, {1.0, 0.65, 0.45, 0.3, 0.2, 0.12});
    sieveMetalBody.setBrightness(0.75f);

    Oscillator sieveWaterTone; sieveWaterTone.setSampleRate(sampleRate); sieveWaterTone.setWaveform(Oscillator::Waveform::Sine);
    GranularCloud sieveWaterGrain; sieveWaterGrain.setSampleRate(sampleRate, 1.0);
    sieveWaterGrain.setDensity(24.0);
    sieveWaterGrain.setGrainDuration(0.02, 0.08);
    sieveWaterGrain.setSpread(0.4);
    sieveWaterGrain.setPitchJitter(7.0);
    sieveWaterGrain.setMix(1.0f);

    Oscillator sievePlasmaOsc; sievePlasmaOsc.setSampleRate(sampleRate); sievePlasmaOsc.setWaveform(Oscillator::Waveform::Pulse);
    Envelope sievePlasmaEnv; sievePlasmaEnv.setSampleRate(sampleRate);
    sievePlasmaEnv.setADSR(0.005, 0.05, 0.0, 0.25); // sharp, percussive interjection -- not a sustained voice

    // Per-channel independent clocks: each channel's own phase
    // accumulator, advanced every sample by its own base rate (seed-
    // derived, distinct per channel) further modulated by the piece's
    // own already-ticking Lorenz value (read-only, no new chaos state)
    // -- three DIFFERENT scalings of that one shared value so the
    // channels drift apart over time rather than moving in lockstep
    // despite sharing one chaos source.
    double sieveWoodPhase = 0.0, sieveMetalPhase = 0.0, sieveWaterPhase = 0.0;
    double sieveWoodRate = 0.6 + unit(rng) * 0.5;
    double sieveMetalRate = 0.35 + unit(rng) * 0.4;
    double sieveWaterRate = 0.9 + unit(rng) * 0.6;
    int sieveWoodPc = 0, sieveMetalPc = 0, sieveWaterPc = 0;
    int sieveWoodGateSamplesLeft = 0;
    int sievePolyphonyPlasmaCount = 0;
    int sieveWaterArticulationChangeCount = 0;
    double sieveWaterDensityMin = 0.0;
    double sieveWaterDensityMax = 0.0;
    double sieveWaterGrainDurationMin = 0.0;
    double sieveWaterGrainDurationMax = 0.0;
    double sieveWaterPitchJitterMin = 0.0;
    double sieveWaterPitchJitterMax = 0.0;
    std::mt19937 sieveWaterTimbreRng(cfg.seed + 41003u);
    std::uniform_real_distribution<double> sieveWaterTimbreUnit(0.0, 1.0);
    std::array<bool, 4> sievePitchTelemetryInitialized{};
    std::array<int, 4> sievePreviousPitch{};
    std::array<int, 4> sieveCurrentRepeatedNotes{};
    std::array<int, 4> sievePitchChangeCount{};
    std::array<int, 4> sieveRepeatedPitchCount{};
    std::array<int, 4> sieveLongestRepeatedNotes{};
    std::array<int, 4> sievePitchMin{};
    std::array<int, 4> sievePitchMax{};
    auto recordSievePitch = [&](std::size_t channel, int midi) {
        if (channel >= sievePitchTelemetryInitialized.size()) return;
        midi = std::clamp(midi, 0, 127);
        if (!sievePitchTelemetryInitialized[channel]) {
            sievePitchTelemetryInitialized[channel] = true;
            sievePreviousPitch[channel] = midi;
            sieveCurrentRepeatedNotes[channel] = 1;
            sievePitchMin[channel] = sievePitchMax[channel] = midi;
        } else if (sievePreviousPitch[channel] == midi) {
            ++sieveCurrentRepeatedNotes[channel];
            ++sieveRepeatedPitchCount[channel];
        } else {
            ++sievePitchChangeCount[channel];
            sieveCurrentRepeatedNotes[channel] = 1;
            sievePreviousPitch[channel] = midi;
        }
        sieveLongestRepeatedNotes[channel] = std::max(
            sieveLongestRepeatedNotes[channel], sieveCurrentRepeatedNotes[channel]);
        sievePitchMin[channel] = std::min(sievePitchMin[channel], midi);
        sievePitchMax[channel] = std::max(sievePitchMax[channel], midi);
    };

    // Fase 79 -- Bloom-inspired fractal melody (see CREDITS.md: Qu-Bit
    // Electronix Bloom, Andrew Ikenberry -- a "Trunk" base sequence
    // transformed recursively into "Branch" variations, navigated by a
    // "Path" parameter). Reimplemented independently as a genuine self-
    // similar structure: a short seed-derived Trunk of 5 semitone offsets,
    // and a Branch level built by literally re-applying the Trunk's own
    // pattern to itself (25 notes, each one the Trunk transposed by one of
    // the Trunk's own values) -- the same recursive-self-similarity idea
    // real fractal sequencers use, at a genuinely different structural
    // level from anything else in this engine (Turing-machine lock/jump
    // mutates a fixed-length pattern in place; chaos-addressed step
    // reading jumps around a fixed table; this literally regenerates a
    // longer structure FROM the short one's own shape). Path is a simple
    // 0/1 level index that can switch, with its own probability, each
    // time the currently-playing level completes a full loop -- walking
    // between "zoomed out" (Trunk) and "zoomed in" (Trunk-of-Trunk)
    // rather than growing without bound. Final pitches are snapped to the
    // nearest degree of the piece's own active scale, keeping the
    // fractal's raw semitone shape harmonically consistent with
    // everything else. Own fresh oscillator+envelope, own independent
    // clock -- additive layer, does not touch the main sequencer's
    // already-validated melody logic.
    bool bloomEnabled = selectionTrace.resolve(StudioModule::Bloom, unit(rng) < 0.22);
    std::array<int, 5> bloomTrunk{};
    for (int i = 0; i < 5; ++i) bloomTrunk[static_cast<size_t>(i)] = static_cast<int>(unit(rng) * 15.0) - 7; // -7..+7 semitones
    std::array<int, 25> bloomBranch{};
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5; ++j)
            bloomBranch[static_cast<size_t>(i * 5 + j)] =
                bloomTrunk[static_cast<size_t>(i)] + bloomTrunk[static_cast<size_t>(j)];
    int bloomPathLevel = unit(rng) < 0.5 ? 0 : 1;
    double bloomSwitchProbability = 0.2 + unit(rng) * 0.3;
    int bloomStepIndex = 0;
    double bloomPhase = 0.0;
    double bloomLoopSeconds = 3.0 + unit(rng) * 4.0; // a full Trunk (or Trunk-of-Trunk) cycle takes roughly this long regardless of level
    Oscillator bloomOsc; bloomOsc.setSampleRate(sampleRate); bloomOsc.setWaveform(Oscillator::Waveform::Saw);
    Envelope bloomEnv; bloomEnv.setSampleRate(sampleRate);
    bloomEnv.setADSR(0.02, 0.08, 0.3, 0.12);
    int bloomLevelSwitchCount = 0;

    // AY8910Chip (see the phase-12 note above): a chiptune-flavoured
    // arpeggio layer, tuned to the current chord tone whenever the
    // sequencer gates a new note, fading in with the arc's resonance
    // alongside the ring modulator and multiband bloom below rather than
    // running constantly.
    AY8910Chip chiptune; chiptune.setSampleRate(sampleRate);
    chiptune.setToneEnabled(0, true); chiptune.setToneEnabled(1, true); chiptune.setToneEnabled(2, true);
    chiptune.setVolume(0, 0.5f); chiptune.setVolume(1, 0.35f); chiptune.setVolume(2, 0.25f);
    chiptune.setEnvelopeShape(AY8910Chip::EnvelopeShape::DecayRepeat);
    chiptune.setEnvelopeFrequency(1.0 + unit(rng) * 2.0);

    MultibandFilter bands; bands.setSampleRate(sampleRate);
    bands.setBandFrequencies({300.0, 900.0, 2200.0, 4500.0});
    for (int b = 0; b < MultibandFilter::kNumBands; ++b) bands.setBandQ(b, 3.0);

    MoogFilter filter; filter.setSampleRate(sampleRate);
    Envelope ampEnv; ampEnv.setSampleRate(sampleRate);
    switch (identity.coreVoice) {
        case CoreVoiceModel::Subtractive: ampEnv.setADSR(0.008, 0.22, 0.52, secondsPerStep * 2.4); break;
        case CoreVoiceModel::Rounded: ampEnv.setADSR(0.08, 0.5, 0.72, secondsPerStep * 4.5); break;
        case CoreVoiceModel::Pulse: ampEnv.setADSR(0.002, 0.09, 0.24, secondsPerStep * 1.2); break;
        case CoreVoiceModel::Inharmonic: ampEnv.setADSR(0.015, 0.38, 0.38, secondsPerStep * 3.2); break;
        case CoreVoiceModel::Minimal: ampEnv.setADSR(0.18, 0.7, 0.82, secondsPerStep * 6.0); break;
    }

    // A second filter colour (a 2-pole Chamberlin state-variable filter;
    // the SallenKeyFilter class name is retained only for compatibility)
    // crossfaded against the ladder above -- see the phase-10 note at the
    // top of this file. GbmanMap drives the crossfade: it's one of the six
    // independently implemented chaotic maps added in phase 8 but not wired into
    // this patch until now, so this also finishes a piece of unfinished
    // business from that phase.
    SallenKeyFilter sallenFilter; sallenFilter.setSampleRate(sampleRate);
    SallenKeyFilter::Mode coreFilterMode = SallenKeyFilter::Mode::Lowpass;
    if (identity.coreVoice == CoreVoiceModel::Pulse) coreFilterMode = SallenKeyFilter::Mode::Highpass;
    else if (identity.coreVoice == CoreVoiceModel::Inharmonic) coreFilterMode = SallenKeyFilter::Mode::Bandpass;
    sallenFilter.setMode(coreFilterMode);
    float coreSecondFilterMax = identity.coreVoice == CoreVoiceModel::Subtractive ? 0.42f
                              : identity.coreVoice == CoreVoiceModel::Rounded ? 0.3f
                              : identity.coreVoice == CoreVoiceModel::Minimal ? 0.18f
                              : 0.9f;
    GbmanMap filterColorMap; filterColorMap.setSampleRate(sampleRate); filterColorMap.setRate(0.6);
    filterColorMap.setInitial(0.05 + 0.1 * unit(rng), 0.05 + 0.1 * unit(rng));

    // Fase 75 -- a THIRD filter colour (see PolivoksFilter.hpp): for a
    // fraction of pieces, the same GbmanMap-driven crossfade slot above
    // uses the Polivoks' harsh, genuinely self-oscillating character
    // instead of the clean Chamberlin SVF -- mutually exclusive per piece,
    // same "one clean mode, not layered" discipline as every other
    // alternate-colour choice in this file (voiceColor's Tape/Tube/
    // Fracture, drum archetypes, etc.).
    bool polivoksEnabled = unit(rng) < 0.25;
    PolivoksFilter polivoksFilter; polivoksFilter.setSampleRate(sampleRate);
    bool polivoksWasSelfOscillating = false;
    int polivoksSelfOscillationCount = 0;

    // Fase 93 -- Dual VCA, Polivoks-architecture inspired (see
    // PolivoksVCA.hpp): the real module's differential-pair transfer curve
    // (now an AS3330 in modern clones) instead of a clean idealized
    // multiply -- quiet passages stay close to linear, loud ones compress/
    // saturate against the pair's own thermal-voltage-like limit. Colours
    // the two hocketing voices' own gain stage (fase 37) specifically:
    // already a genuine "dual" pair (two independent oscillator+envelope
    // channels sharing one circuit identity, panned apart), the closest
    // real match in this engine to what a physical Dual VCA module colours.
    // Independent roll from hocketing itself -- only audible when both are
    // on, same as any other seed-gated colour choice layered on existing
    // structure.
    bool polivoksVcaEnabled = unit(rng) < 0.25;
    DualPolivoksVCA polivoksVca;
    polivoksVca.configure(1.5f + static_cast<float>(unit(rng)) * 2.0f);

    // The percussion layer this engine never had: a CGS22-style XOR clock
    // divider clocked once per sequencer step, driving three simple analog-
    // modeled drum voices. See PolyrhythmDivider.hpp / DrumVoices.hpp.
    //
    // Every parameter here is seed-derived -- which divider phase things
    // start on, which pair of the divider's four stages each voice's XOR
    // trigger watches, and each voice's own tuning -- so the percussion
    // pattern is a genuine consequence of the seed like everything else in
    // this engine, not a fixed backing track pasted under whatever the
    // melodic side happens to do.
    PolyrhythmDivider polyDivider;
    polyDivider.reset(static_cast<uint32_t>(rng() % 16u));

    // Cross-domain contamination: the piece's own recently-loud output
    // reaching into the CLOCK domain, not a continuous parameter -- a
    // structurally different target from self-feedback above (fase 23),
    // which nudges the Lorenz map's continuous rate from a SLOW (~1200-
    // sample) loudness average. This reads a much faster peak follower
    // and, when it spikes past a per-piece threshold with a minimum
    // cooldown elapsed, fires one EXTRA polyDivider.clock() pulse at the
    // next step boundary -- permanently shifting the phase of every
    // XOR-derived drum pattern downstream of that one shared counter, a
    // discrete state perturbation rather than a smooth parameter nudge.
    // Real audio-domain data crossing into the rhythmic/control domain,
    // the exact pairing the "em aberto" list asked for by name ("pico de
    // audio -> taxa do divisor de clock").
    bool crossDomainContagionEnabled = unit(rng) < market.price("crossDomainContagion", 0.25);
    market.record("crossDomainContagion", crossDomainContagionEnabled);
    float audioPeakFollower = 0.0f;
    float peakFollowerRelease = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.12)));
    double contagionThreshold = 0.55 + unit(rng) * 0.25;
    int contagionMinGapSteps = 8 + static_cast<int>(unit(rng) * 24.0);
    int stepsSinceContagion = contagionMinGapSteps;
    int crossDomainContagionCount = 0;

    static const int kStagePairs[6][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};
    std::vector<int> pairOrder = {0, 1, 2, 3, 4, 5};
    std::shuffle(pairOrder.begin(), pairOrder.end(), rng);
    int kickA = kStagePairs[pairOrder[0]][0], kickB = kStagePairs[pairOrder[0]][1];
    int snareA = kStagePairs[pairOrder[1]][0], snareB = kStagePairs[pairOrder[1]][1];
    int hihatA = kStagePairs[pairOrder[2]][0], hihatB = kStagePairs[pairOrder[2]][1];
    // The three stage-pairs kick/snare/hihat never used, now spoken for --
    // cowbell, clap and the tom/conga voice each get a genuinely different
    // slice of the same shared XOR clock instead of copying an existing
    // voice's trigger pattern.
    int cowbellA = kStagePairs[pairOrder[3]][0], cowbellB = kStagePairs[pairOrder[3]][1];
    int clapA = kStagePairs[pairOrder[4]][0], clapB = kStagePairs[pairOrder[4]][1];
    int tomA = kStagePairs[pairOrder[5]][0], tomB = kStagePairs[pairOrder[5]][1];

    // Beyond this session's user report ("os sons de percussão são sempre
    // os mesmos"): every voice below already varied its *pitch* per seed,
    // but the circuit's own character -- decay times, filter cutoff/
    // resonance, burst timing -- was a fixed constant, so a piece's kick/
    // snare/etc. always read as "the same drum, just retuned." Each voice
    // now also jitters those shape parameters per seed, around the same
    // centre values the fixed defaults used, so the character genuinely
    // differs piece to piece, not just the register.
    KickVoice kick; kick.setSampleRate(sampleRate);
    double kickBaseHz = 42.0 + unit(rng) * 24.0;
    double kickSweepHz = 55.0 + unit(rng) * 70.0;
    kick.setTuning(kickBaseHz, kickSweepHz);
    double kickPitchDecayBase = 0.05 + unit(rng) * 0.11;
    double kickAmpDecayBase = 0.15 + unit(rng) * 0.30;
    double kickAttackCurveBase = 0.78 + unit(rng) * 0.44;
    kick.setDecayTimes(kickPitchDecayBase, kickAmpDecayBase);
    kick.setAttackCurve(kickAttackCurveBase);
    SnareVoice snare; snare.setSampleRate(sampleRate);
    snare.seedNoise(static_cast<unsigned>(rng()));
    // Percussion noise identity: the snare/clap/maracas no longer all begin
    // from the same white-noise material. The choices remain bounded and
    // voice-specific: the snare keeps body/mesh legible, the clap avoids an
    // over-bright lead, and maracas are allowed to occupy the air band. This
    // is a timbral identity decision, not a new level boost.
    static constexpr std::array<ColoredNoise::Color, 3> snareNoiseChoices {
        ColoredNoise::Color::White, ColoredNoise::Color::Pink, ColoredNoise::Color::Blue };
    static constexpr std::array<const char *, 3> snareNoiseNames {"white", "pink", "blue"};
    const int snareNoiseChoice = static_cast<int>(noiseVariantUnit(noiseVariantRng) * snareNoiseChoices.size()) % static_cast<int>(snareNoiseChoices.size());
    snare.setNoiseColor(snareNoiseChoices[static_cast<std::size_t>(snareNoiseChoice)]);
    const char *snareNoiseColorName = snareNoiseNames[static_cast<std::size_t>(snareNoiseChoice)];
    double snareToneBaseHz = 160.0 + unit(rng) * 70.0;
    snare.setTone(snareToneBaseHz);
    double snareToneDecayBase = 0.06 + unit(rng) * 0.16;
    double snareNoiseDecayBase = 0.08 + unit(rng) * 0.20;
    double snareAttackCurveBase = 0.82 + unit(rng) * 0.36;
    // Derive the component balance from already-drawn snare decays rather
    // than consuming new RNG values. This keeps the rest of a seed's
    // composition path stable while giving shorter/longer bodies their own
    // bounded articulation.
    // Keep the low-frequency body legible even when a blue/pink noise choice
    // gives the mesh a stronger perceived edge. This is a local articulation
    // compensation, not a bus gain: body remains below the noise component's
    // raw gain range, but no longer starts at 0.68 where long concealed pieces
    // made the snare read as noise without a center.
    // Interrupting strategies ask the kit to cut through a busy exchange,
    // but that does not mean "more white noise": give the snare's body a
    // small contextual centre bias and let its rattle recede. Conceal keeps
    // a milder version of the same protection. This is deliberately local
    // to the snare articulation; no drum-bus or master gain is changed.
    const float snareContextBodyBias = gamePlan.strategy == GameStrategy::Interrupt ? 0.10f
        : gamePlan.strategy == GameStrategy::Conceal ? 0.06f : 0.0f;
    const float snareContextNoiseBias = gamePlan.strategy == GameStrategy::Interrupt ? -0.08f
        : gamePlan.strategy == GameStrategy::Conceal ? -0.04f : 0.0f;
    float snareBodyComponentGain = static_cast<float>(0.78
        + ((snareToneDecayBase - 0.06) / 0.16) * 0.12) + snareContextBodyBias;
    float snareNoiseComponentGain = static_cast<float>(1.05
        + ((snareNoiseDecayBase - 0.08) / 0.20) * 0.18) + snareContextNoiseBias;
    double snareNoiseCutoffBase = 1200.0 + unit(rng) * 1400.0;
    double snareNoiseResonanceBase = 0.15 + unit(rng) * 0.40;
    snare.setDecayTimes(snareToneDecayBase, snareNoiseDecayBase);
    snare.setNoiseFilter(snareNoiseCutoffBase, snareNoiseResonanceBase);
    snare.setAttackCurve(snareAttackCurveBase);

    // "Contagio do caos": most pieces still have perfectly uniform kick/
    // snare hits, decay times fixed once above like every other voice
    // parameter. In the rare pieces where this fires, each individual hit
    // instead gets its own small decay jitter straight from the chaos maps
    // already ticking every sample for pitch drift (lato for the kick,
    // standard for the snare -- different sources so the two drums don't
    // move in lockstep) -- the drums start "breathing" the same instability
    // the melody already has, instead of being the one perfectly
    // mechanical layer in an otherwise unstable piece.
    bool chaosContagionEnabled = unit(rng) < 0.15;
    double chaosContagionDepth = chaosContagionEnabled ? (0.2 + unit(rng) * 0.35) : 0.0;
    // A broader, quieter per-hit timbre drift than the rare chaos-contagion
    // mode above. It changes existing voice parameters only at trigger time,
    // so a kit breathes without becoming a continuously modulated blur.
    bool percussionTimbreVariationEnabled = unit(rng) < 0.72;
    double percussionTimbreDepth = 0.035 + unit(rng) * 0.075;
    // Keep the correction local to the snare's component balance. The white
    // mesh needs the stronger trim, while pink/blue can also become
    // noise-forward when per-hit timbre drift is active. This is not a bus or
    // master gain change: it preserves each colour's identity while keeping a
    // centre audible inside the rattle.
    const bool snareWhiteVariationCompensation = percussionTimbreVariationEnabled
        && snareNoiseChoice == 0;
    const bool snareColorBalanceCompensation = percussionTimbreVariationEnabled;
    if (snareWhiteVariationCompensation) {
        snareBodyComponentGain += 0.06f;
        snareNoiseComponentGain -= 0.12f;
    } else if (snareColorBalanceCompensation) {
        snareBodyComponentGain += 0.04f;
        snareNoiseComponentGain -= 0.08f;
    }
    snare.setComponentGains(snareBodyComponentGain, snareNoiseComponentGain);
    HiHatVoice hihat; hihat.setSampleRate(sampleRate);
    double hihatFundamentalBaseHz = 260.0 + unit(rng) * 180.0;
    hihat.setFundamental(hihatFundamentalBaseHz);
    double hihatDecay1Base = 0.025 + unit(rng) * 0.045;
    double hihatDecay2Base = 0.18 + unit(rng) * 0.37;
    double hihatCutoffBase = 5000.0 + unit(rng) * 5000.0;
    double hihatResonanceBase = 0.05 + unit(rng) * 0.30;
    hihat.setDecayTimes(hihatDecay1Base, hihatDecay2Base);
    hihat.setFilter(hihatCutoffBase, hihatResonanceBase);

    // Mutable Instruments' Branches: instead of every hihat clock edge
    // deterministically hitting the same (closed) voicing, a Bernoulli gate
    // routes each edge between closed and open per-seed-drawn odds -- the
    // classic "occasional open hat" pattern variation, done as a genuine
    // probabilistic router rather than a fixed every-Nth-hit rule. Mode
    // (coin-flip vs. strict alternation) and open-probability are both
    // seed-derived, same spirit as the divider's own XOR pair choices.
    BernoulliGate hihatBranch;
    hihatBranch.seed(cfg.seed + 20000u);
    hihatBranch.setMode(unit(rng) < 0.5 ? BernoulliGate::Mode::Bernoulli : BernoulliGate::Mode::Toggle);
    hihatBranch.setProbability(0.15 + unit(rng) * 0.35);

    CowbellVoice cowbell; cowbell.setSampleRate(sampleRate);
    double cowbellBaseFreq = 480.0 + unit(rng) * 160.0;
    cowbell.setFundamental(cowbellBaseFreq);
    double cowbellRatioBase = 1.2 + unit(rng) * 0.6;
    double cowbellDecayBase = 0.15 + unit(rng) * 0.40;
    double cowbellResonanceBase = 0.25 + unit(rng) * 0.50;
    cowbell.setRatio(cowbellRatioBase);
    cowbell.setDecay(cowbellDecayBase);
    cowbell.setResonance(cowbellResonanceBase);

    // Euclidean rhythm (Bjorklund/Toussaint): distribute k onsets as
    // evenly as possible across n steps -- real mathematics, not
    // generated audio math like the chaos maps, and genuinely different
    // in kind from the polyrhythm-divider clock the drums otherwise use.
    // Cowbell specifically because a cowbell/campana carrying a clave-like
    // pattern is itself standard practice in a lot of real percussion, not
    // an arbitrary choice of voice. Two of the k values below are named
    // honestly because the correspondence is exact and well-documented
    // (E(3,8)/E(5,8) repeated twice to fill this engine's 16-step bar are
    // literally the tresillo/cinquillo, verified by hand against the
    // algorithm's own periodicity, not asserted); the rest are just
    // "E(k,16)" with no cultural name attached, since claiming one without
    // that same verification would be exactly the kind of fabricated
    // correspondence this project has already retired scale names over.
    //
    // Kept deliberately on the sparse side (k <= 10 of 16, i.e. <=62.5%,
    // never above the cinquillo's own density) -- k=11/13 were in an
    // earlier version of this list and made cowbell fire on 69-81% of
    // every single sixteenth note, audibly busier than every other drum
    // voice despite sharing the exact same shared-clock mechanism and mix
    // weight (0.18, actually the lowest of the eight). Caught by ear, not
    // by the regression suite -- clipping/peak checks can't hear "this one
    // instrument won't stop talking."
    bool euclideanCowbellEnabled = unit(rng) < 0.25;
    std::vector<bool> euclideanPattern;
    std::string euclideanDescription;
    if (euclideanCowbellEnabled) {
        struct EuclideanChoice { int k; const char *name; };
        static const EuclideanChoice euclideanChoices[] = {
            {3, "E(3,16)"}, {4, "E(4,16)"}, {5, "E(5,16)"},
            {6, "E(6,16) -- tresillo cubano, dobrado (2x por compasso)"},
            {7, "E(7,16)"},
            {10, "E(10,16) -- cinquillo cubano, dobrado (2x por compasso)"},
        };
        int choiceIdx = static_cast<int>(unit(rng) * 6.0) % 6;
        int euclideanK = euclideanChoices[choiceIdx].k;
        euclideanDescription = euclideanChoices[choiceIdx].name;
        euclideanPattern.assign(static_cast<size_t>(stepsPerBar), false);
        int bucket = 0;
        for (int i = 0; i < stepsPerBar; ++i) {
            bucket += euclideanK;
            if (bucket >= stepsPerBar) { bucket -= stepsPerBar; euclideanPattern[static_cast<size_t>(i)] = true; }
        }
    }

    ClapVoice clap; clap.setSampleRate(sampleRate);
    clap.seedNoise(static_cast<unsigned>(rng()));
    static constexpr std::array<ColoredNoise::Color, 3> clapNoiseChoices {
        ColoredNoise::Color::White, ColoredNoise::Color::Pink, ColoredNoise::Color::Blue };
    static constexpr std::array<const char *, 3> clapNoiseNames {"white", "pink", "blue"};
    const int clapNoiseChoice = static_cast<int>(noiseVariantUnit(noiseVariantRng) * clapNoiseChoices.size()) % static_cast<int>(clapNoiseChoices.size());
    clap.setNoiseColor(clapNoiseChoices[static_cast<std::size_t>(clapNoiseChoice)]);
    const char *clapNoiseColorName = clapNoiseNames[static_cast<std::size_t>(clapNoiseChoice)];
    double clapDecayBase = 0.10 + unit(rng) * 0.30;
    double clapCutoffBase = 1000.0 + unit(rng) * 1400.0;
    double clapResonanceBase = 0.15 + unit(rng) * 0.35;
    double clapBurstBase = 0.003 + unit(rng) * 0.007;
    double clapGapBase = 0.005 + unit(rng) * 0.009;
    clap.setDecay(clapDecayBase);
    clap.setFilter(clapCutoffBase, clapResonanceBase);
    clap.setBurstTiming(clapBurstBase, clapGapBase);

    // Cymbal and maracas close out the TR-808 set, but the six XOR stage-
    // pairs above are already spoken for (kick/snare/hihat/cowbell/clap/
    // tom). Rather than extend PolyrhythmDivider's stage count just for
    // two more voices, these trigger off zero-crossings of the chaos maps
    // already running every sample for modulation -- a genuinely
    // different, non-clock-locked source of timing instead of another
    // slice of the same grid. Cymbal watches Henon at bar boundaries (a
    // crash is a rare, accent-like event, not a per-step one); maracas
    // watches Latoocarfian at step boundaries (a shaker rides closer to
    // the beat subdivision).
    CymbalVoice cymbal; cymbal.setSampleRate(sampleRate);
    double cymbalBaseFreq = 300.0 + unit(rng) * 140.0;
    cymbal.setFundamental(cymbalBaseFreq);
    double cymbalDecayBase = 0.7 + unit(rng) * 1.5;
    cymbal.setDecay(cymbalDecayBase);
    double cymbalCutoffBase = 0.0, cymbalResonanceBase = 0.0;
    cymbal.setFilter(cymbalCutoffBase = 2800.0 + unit(rng) * 3200.0,
                     cymbalResonanceBase = 0.05 + unit(rng) * 0.25);
    MaracasVoice maracas; maracas.setSampleRate(sampleRate);
    maracas.seedNoise(static_cast<unsigned>(rng()));
    static constexpr std::array<ColoredNoise::Color, 3> maracasNoiseChoices {
        ColoredNoise::Color::Blue, ColoredNoise::Color::White, ColoredNoise::Color::Pink };
    static constexpr std::array<const char *, 3> maracasNoiseNames {"blue", "white", "pink"};
    const int maracasNoiseChoice = static_cast<int>(noiseVariantUnit(noiseVariantRng) * maracasNoiseChoices.size()) % static_cast<int>(maracasNoiseChoices.size());
    maracas.setNoiseColor(maracasNoiseChoices[static_cast<std::size_t>(maracasNoiseChoice)]);
    const char *maracasNoiseColorName = maracasNoiseNames[static_cast<std::size_t>(maracasNoiseChoice)];
    double maracasDecayBase = 0.03 + unit(rng) * 0.09;
    double maracasCutoffBase = 2800.0 + unit(rng) * 3200.0;
    double maracasResonanceBase = 0.05 + unit(rng) * 0.25;
    maracas.setDecay(maracasDecayBase);
    maracas.setFilter(maracasCutoffBase, maracasResonanceBase);

    // Individual effects per percussion voice -- the other half of the
    // user's question that led to the humanization above ("um oscilador
    // com efeitos para cada peca de percussao"). Each of the 8 voices gets
    // its own independent roll of the same tape/tube saturator colour
    // already used on the main melodic voice (VoiceColor above), applied
    // to that voice alone before it joins the shared drums bus -- a real
    // per-instrument character, not just the shared gain multiplier
    // (kickGain etc.) already there. Most voices still land on "none".
    enum class PercColor { None, Tape, Tube };
    struct PercVoiceFX {
        PercColor color = PercColor::None;
        TapeSaturator tape;
        TubeSaturator tube;
        PolivoksVCA vca;
        float blend = 0.0f;
        float vcaBlend = 0.0f;
        bool characterVca = false;
    };
    auto makePercFx = [&]() {
        PercVoiceFX fx;
        fx.tape.setSampleRate(sampleRate);
        fx.tape.setDrive(0.3f + static_cast<float>(unit(rng)) * 0.4f);
        fx.tube.setDrive(0.25f + static_cast<float>(unit(rng)) * 0.4f);
        fx.characterVca = unit(rng) < 0.18;
        fx.vcaBlend = 0.14f + static_cast<float>(unit(rng)) * 0.22f;
        fx.vca.configure(0.75f + static_cast<float>(unit(rng)) * 1.45f);
        double roll = unit(rng);
        fx.color = roll < 0.2 ? PercColor::Tape : roll < 0.35 ? PercColor::Tube : PercColor::None;
        fx.blend = 0.3f + static_cast<float>(unit(rng)) * 0.35f;
        return fx;
    };
    PercVoiceFX kickFx = makePercFx(), snareFx = makePercFx(), hihatFx = makePercFx(),
                cowbellFx = makePercFx(), clapFx = makePercFx(), cymbalFx = makePercFx(),
                maracasFx = makePercFx();
    std::array<PercVoiceFX, 3> tomsFx = {makePercFx(), makePercFx(), makePercFx()};
    auto applyPercFx = [](PercVoiceFX &fx, float dry) {
        float shaped = dry;
        if (fx.characterVca) {
            const float vcaWet = fx.vca.process(dry, 1.0f);
            shaped = dry * (1.0f - fx.vcaBlend) + vcaWet * fx.vcaBlend;
        }
        if (fx.color == PercColor::None) return shaped;
        float wet = fx.color == PercColor::Tape ? fx.tape.process(shaped) : fx.tube.process(shaped);
        return shaped * (1.0f - fx.blend) + wet * fx.blend;
    };
    int percFxColoredCount = 0;
    int percFxCharacterVcaCount = 0;
    for (const PercVoiceFX *fx : {&kickFx, &snareFx, &hihatFx, &cowbellFx, &clapFx, &cymbalFx, &maracasFx,
                                   &tomsFx[0], &tomsFx[1], &tomsFx[2]}) {
        if (fx->color != PercColor::None) ++percFxColoredCount;
        if (fx->characterVca) ++percFxCharacterVcaCount;
    }

    // Per-piece drum-kit balance, seed-derived rather than one fixed mix
    // for every piece: each voice's weight gets its own random multiplier
    // (0.75-1.35x) so one piece might land hihat-forward and clap-shy while
    // another leans on kick/toms instead -- a different-feeling kit each
    // time, same underlying eight voices. `drumsPresence` is a second,
    // independent roll for how far forward the whole kit sits in the mix
    // overall (0.55-0.8x for a supporting kit, 0.88-1.2x for a rhythmic
    // protagonist) -- the supporting range was raised after the stem audit
    // showed that the old 0.38-0.66x roll still left the kit buried in dense
    // pieces. It remains below the protagonist range and is still seed-varied.
    auto drumVoiceGain = [&]() { return static_cast<float>(0.75 + unit(rng) * 0.6); };
    // Kick/snare are the two voices that actually carry the beat's
    // legibility -- a user listening pass reported not being able to hear
    // them clearly, traced to this multiplier occasionally rolling low
    // for exactly the two voices where that matters most. Given a higher
    // floor (0.95-1.4x) than the decorative voices (0.75-1.35x, unchanged
    // -- those are fine to sometimes sit further back).
    auto anchorVoiceGain = [&]() { return static_cast<float>(0.95 + unit(rng) * 0.45); };
    float kickGain = anchorVoiceGain(), snareGain = anchorVoiceGain(), hihatGain = drumVoiceGain(),
          cowbellGain = drumVoiceGain(), clapGain = drumVoiceGain(), tomsGain = drumVoiceGain(),
          cymbalGain = drumVoiceGain();
    // Maracas occupy the air band and their Euclidean pattern is intentionally
    // denser (7--13 of 16 steps). Give this decorative desk its own narrower
    // presence range so a busy shaker does not become the apparent lead.
    float maracasGain = static_cast<float>(0.60 + unit(rng) * 0.45);
    float drumsPresence = rhythmicProtagonist
        ? static_cast<float>(0.88 + unit(rng) * 0.32)
        : static_cast<float>(0.55 + unit(rng) * 0.25);
    if (gamePlan.strategy == GameStrategy::Conceal) drumsPresence *= 0.58f;
    if (gamePlan.strategy == GameStrategy::Interrupt) drumsPresence *= 1.18f;
    if (gamePlan.strategy == GameStrategy::Counter) drumsPresence *= 0.88f;
    // Concealment may place the kit behind the scene, but it must not erase
    // the instrument's rhythmic identity. The pre-strategy supporting range
    // is 0.55..0.80; after the old 0.58 conceal multiplier it became
    // 0.319..0.464, which made long pieces report a nearly inaudible kit.
    // Keep a recessed but perceptible floor. Protagonist pieces are already
    // above it and are unaffected.
    constexpr float kDrumsSupportPresenceFloor = 0.42f;
    const bool drumsPresenceFloorApplied = !rhythmicProtagonist
        && drumsPresence < kDrumsSupportPresenceFloor;
    drumsPresence = std::max(drumsPresence, kDrumsSupportPresenceFloor);
    double drumsEnergySum = 0.0;
    std::size_t drumsMeasuredFrames = 0;
    std::size_t drumsAudibleFrames = 0;
    float drumsPeak = 0.0f;
    // Diagnostic order: kick, snare, hihat, cowbell, clap, toms, cymbal,
    // maracas. These are post-presence contributions, so the measurements
    // answer the practical question "which desk is actually audible?" rather
    // than merely reporting the raw voice before arrangement gain.
    std::array<double, 8> drumVoiceEnergySum{};
    std::array<float, 8> drumVoicePeak{};
    double snareBodyEnergySum = 0.0, snareNoiseEnergySum = 0.0;
    float snareBodyPeak = 0.0f, snareNoisePeak = 0.0f;

    // Fase 85 stage 2 -- real per-hit dynamics (see RhythmGenerator.hpp's
    // hitVelocity/hitVelocityByDensity): each voice's *last-triggered* hit
    // carries its own velocity multiplier, updated at the moment it fires
    // and held (like a real drum machine's per-hit velocity, not just its
    // attack transient) until the next hit overwrites it. Centered near
    // 1.0 so the seed-derived per-voice gains above still set the overall
    // balance; this modulates around that baseline.
    float kickVelocity = 1.0f, snareVelocity = 1.0f, hihatVelocity = 1.0f,
          cowbellVelocity = 1.0f, clapVelocity = 1.0f, cymbalVelocity = 1.0f,
          maracasVelocity = 1.0f;
    float snareBodyHitGain = 1.0f, snareNoiseHitGain = 1.0f;
    std::array<float, 3> tomVelocity = {1.0f, 1.0f, 1.0f};

    // Fase 85 stage 2 -- gesture (C): a slow, already-running LFO channel
    // (the ochd bank -- see lfoOut[] below) sampled once per 4-bar phrase
    // rather than continuously, standing in for the way a real player
    // pushes or drags a whole phrase rather than wobbling note to note.
    // Reused, not a new oscillator -- same discipline the rest of this
    // project follows ("estender o que ja foi validado vem antes de
    // inventar de novo"). Read where lfoOut is available further down;
    // held here so the drum-trigger logic (which runs earlier in the
    // per-step block) can see the current phrase's value.
    double gestureBias = 0.0;
    constexpr int gesturePhraseBars = 4;
    int gestureMaxSamples = static_cast<int>(0.012 * sampleRate); // up to ~12ms push/pull

    // Fase 85 stage 2 -- repetition with difference (E, Deleuze -- see
    // Manifesto.md's own discussion of Agon): rather than mutating the
    // archetype's defining kick/snare pattern (which would risk the
    // identity a section change is supposed to establish), each real bar
    // repeat gets a small, bounded chance of adding ONE extra quiet ghost
    // hihat at a freshly-chosen step -- audible variation on every real
    // repeat of the cycle, never identical twice, without ever touching
    // what makes an archetype recognizable as itself.
    bool ghostHihatArmedThisBar = false;
    int ghostHihatStep = -1;
    int ghostHihatCount = 0;
    std::mt19937 ghostRng(cfg.seed + 45000u);
    int barCounter = 0;
    double lastLfoOut1 = 0.0;

    // Fase 85 stage 3 -- H: orchestration. Which of the 8 percussion voices
    // are actually "in" this section, decided at the same genuine
    // arc-section boundary fase 67 already uses to change archetype --
    // real arrangement (tacet/entrance), never a mid-bar dropout that
    // would read as a bug. Kick and hihat are the two anchor voices real
    // drumming rarely drops at the same time as everything else, so they
    // stay weighted toward "in" even in sparse sections; how many of the
    // remaining six join scales with the arc's own density (state.density
    // below) -- a full kit near the climax, a leaner one in a calm
    // section. Index order: 0 kick, 1 snare, 2 hihat, 3 cowbell, 4 clap,
    // 5 toms, 6 cymbal, 7 maracas.
    std::array<bool, 8> voiceActive{true, true, true, true, true, true, true, true};
    std::mt19937 orchestrationRng(cfg.seed + 55000u);
    int orchestrationChangeCount = 0;
    int orchestralMemorySectionCount = 0;
    int orchestralHighConflictCount = 0;
    int orchestralRepeatedLeaderCount = 0;
    int orchestralRoleResponseCount = 0;
    OrganismStem orchestralLastLeader = OrganismStem::Other;

    // Fase 85 stage 3 -- G: glitch coupling. When Curto-Circuito (fase 84)
    // finishes its hard-gate dropout, the ensemble gets a real rhythmic
    // aftermath -- the full kit forced back in for a couple of bars,
    // rather than the rupture being purely an amplitude/spectral event.
    // Distinct from Agon's own full-kit override below: this one is
    // triggered by a rupture, that one by a melodic power conflict --
    // same idiom (force full orchestration), different real cause.
    int postGlitchFullKitBars = 0;
    int postGlitchFullKitEvents = 0;

    // Fase 85 stage 3 -- cymbal/maracas migrate off chaos zero-crossing
    // triggering (henonVal/latoVal below) onto the same generalized
    // Euclidean generator built in stage 1 -- real, checkable onset
    // patterns instead of an unpitched chaos wobble crossing zero. Cymbal
    // stays sparse (a crash is a rare, accent-like event); maracas denser
    // (a shaker rides close to the subdivision) -- same real distinction
    // the old chaos-based version already respected, now expressed as a
    // real k value instead of an emergent side effect of an unrelated
    // chaos map's own period.
    EuclideanPattern generatedCymbalPattern = generateEuclideanPattern(rng, 1, 3);
    EuclideanPattern generatedMaracasPattern = generateEuclideanPattern(rng, 7, 13);

    // Fase 66 -- bateria como instrumento de verdade, nao um pano de fundo
    // fixo. Pedido do usuario apos escutar um album inteiro: a bateria
    // nunca falta (antes drumsPresence ficava constante em 0.4-0.7 pela
    // peça inteira) e nunca
    // varia de intensidade dentro da propria peca -- o oposto de "criar
    // suas proprias narrativas". Tres mudancas:
    //
    // 1) drumsEnabled: uma fracao real das pecas (12%) nao tem bateria
    //    nenhuma -- um carater legitimo, mesmo padrao de tuba/ochd/etc,
    //    nao um bug.
    // 2) A presenca da bateria agora acompanha a densidade do proprio arco
    //    (ja lida a cada sample pra granular/gendyn/colorNoise/walsh) --
    //    quase ausente numa secao rarefeita, plena no climax, em vez de
    //    uma unica constante pra peca inteira. Em momentos muito quietos
    //    (densidade < 0.12) a bateria some por completo -- nao precisa de
    //    um mecanismo de "tacet" a parte, o proprio arco ja narra isso.
    // 3) Falhas reais por batida: pra uma fracao das pecas com bateria
    //    (35%), cada gatilho tem uma chance real de simplesmente nao soar
    //    -- o padrao ritmico (XOR ou euclidiano) continua o mesmo, mas a
    //    bateria "erra" de vez em quando, uma imperfeicao real (humana ou
    //    de maquina com defeito) em vez de sempre tocar cada nota do
    //    padrao com precisao de robo.
    // The documented character is approximately 12% of pieces without a
    // kit: keep the threshold at 0.88 rather than silently making absence
    // the dominant fourth of the catalogue.
    const bool automaticDrums = rhythmicProtagonist || identityUnit(cfg.seed, 80) < 0.88;
    bool drumsEnabled = selectionTrace.resolve(StudioModule::Drums,
        automaticDrums || cfg.moduleSelection.hasIncludedDrum());
    for (unsigned idx = 0; idx < 8; ++idx)
        selectionTrace.resolve(static_cast<StudioModule>(static_cast<unsigned>(StudioModule::Kick) + idx),
            true, drumsEnabled, "drums");
    market.record("drumsEnabled", drumsEnabled);
    double hitFailureRate = (drumsEnabled && unit(rng) < 0.35) ? (0.03 + unit(rng) * 0.09) : 0.0;
    std::mt19937 hitFailureRng(cfg.seed + 34000u);
    std::uniform_real_distribution<double> hitFailureUnit(0.0, 1.0);
    auto hitSucceeds = [&]() { return hitFailureRate <= 0.0 || hitFailureUnit(hitFailureRng) >= hitFailureRate; };
    int drumHitFailureCount = 0;

    // Fase 67 -- real rhythmic identity (see DrumArchetypes.hpp): a
    // fraction of pieces with drums swap the XOR clock's kick/snare/hihat
    // triggers for an explicit archetype pattern (Marcha/Soul/Metal/Jazz/
    // Samba-inspirado), re-drawn at every genuinely new arc section --
    // "passagens" between feels within the same piece, not just between
    // pieces. Cowbell/clap/tom/cymbal/maracas keep their existing systems
    // untouched.
    // Base price raised from 0.35 to 0.6 (fase 102, task #29): a real
    // pattern-audit across 40 seeds found drumArchetypeName empty (falling
    // back to the shared pre-fase-67 XOR-clock generator, the SAME
    // underlying rhythmic logic for every one of those pieces) in 80% of
    // renders -- exactly the cross-seed rhythmic sameness the user was
    // hearing, since the whole point of an archetype is to give a piece
    // its own distinct rhythmic identity instead of sharing the generic
    // fallback. When an archetype WAS drawn, the 8 outcomes in that same
    // audit spread across 8 different named archetypes -- no dominance
    // once the mechanism actually engages, so raising how OFTEN it
    // engages is the direct fix, not redesigning the catalog itself.
    bool drumArchetypeEnabled = drumsEnabled
        && (rhythmicProtagonist || identityUnit(cfg.seed, 81) < 0.72);
    market.record("drumArchetype", drumArchetypeEnabled);
    std::mt19937 archetypeRng(cfg.seed + 35000u);
    // Fase 82 -- Xenakis rhythmic sieve (see RhythmSieve.hpp): one extra
    // slot alongside the fixed catalog of named archetypes, holding a
    // genuinely generated, uncataloged onset pattern -- the same
    // "generated vs. catalog" relationship the pitch sieve (fase 61) has
    // with the 45-scale repertoire. Generated once per piece (not
    // re-rolled per section, so a piece that lands on this slot keeps
    // the same generated rhythm throughout, the same way a section
    // landing on "Marcha" always means the real Marcha pattern).
    RhythmSieve generatedRhythm = generateRhythmSieve(rng);
    // Fase 85 stage 1 -- two more generated slots alongside the fixed
    // catalog and the Xenakis sieve: a generalized aksak grouping and a
    // generalized Euclidean onset pattern (see RhythmGenerator.hpp), each
    // drawn once per piece just like the sieve above.
    AksakPattern generatedAksak = generateAksakPattern(rng);
    EuclideanPattern generatedEuclid = generateEuclideanPattern(rng);
    // Independent second Euclidean pattern for the snare -- re-rolled if it
    // happens to land identical to the kick pattern just drawn (a snare
    // that doubles the kick exactly wouldn't read as a second voice).
    EuclideanPattern generatedEuclidSnare = generateEuclideanPattern(rng);
    for (int guard = 0; guard < 10 && generatedEuclidSnare.onset == generatedEuclid.onset; ++guard) {
        generatedEuclidSnare = generateEuclideanPattern(rng);
    }
    // Phase 119: generated rhythm families previously forced hihat=true on
    // every step. Different kick/snare theories therefore kept the same
    // bright sixteenth-note cell -- exactly the recurrent high pattern the
    // comparative listening exposed. The hihat now owns a third generated
    // pattern and can be recreated after a temporal tear.
    EuclideanPattern generatedEuclidHihat = generateEuclideanPattern(rng, 4, 12);
    int rhythmSieveSlotIndex = static_cast<int>(drumArchetypes().size());
    int rhythmAksakSlotIndex = rhythmSieveSlotIndex + 1;
    int rhythmEuclidSlotIndex = rhythmSieveSlotIndex + 2;
    std::array<int, 3> recentArchetypeHistory{{-1, -1, -1}};
    auto rememberArchetype = [&](int index) {
        recentArchetypeHistory[2] = recentArchetypeHistory[1];
        recentArchetypeHistory[1] = recentArchetypeHistory[0];
        recentArchetypeHistory[0] = index;
    };
    auto chooseArchetype = [&](bool generatedOnly) {
        const int first = generatedOnly ? rhythmSieveSlotIndex : 0;
        const int last = rhythmEuclidSlotIndex;
        std::vector<int> candidates;
        std::vector<int> freshCandidates;
        for (int index = first; index <= last; ++index) {
            candidates.push_back(index);
            bool recentlyHeard = false;
            for (int recent : recentArchetypeHistory) recentlyHeard |= recent == index;
            if (!recentlyHeard) freshCandidates.push_back(index);
        }
        const auto &pool = freshCandidates.empty() ? candidates : freshCandidates;
        std::uniform_int_distribution<size_t> pick(0, pool.size() - 1);
        return pool[pick(archetypeRng)];
    };
    int currentArchetypeIndex = drumArchetypeEnabled ? chooseArchetype(false) : 0;
    if (drumArchetypeEnabled) rememberArchetype(currentArchetypeIndex);
    int archetypeChangeCount = 0;
    auto currentArchetypeName = [&]() -> std::string {
        if (currentArchetypeIndex == rhythmSieveSlotIndex) return generatedRhythm.description;
        if (currentArchetypeIndex == rhythmAksakSlotIndex) return generatedAksak.description;
        if (currentArchetypeIndex == rhythmEuclidSlotIndex) return generatedEuclid.description;
        return drumArchetypes()[static_cast<size_t>(currentArchetypeIndex)].name;
    };

    // Fase 89 -- real meter change: a fraction of pieces get the
    // capability at all (most stay in the default 16-step bar the whole
    // time, this project's own "capability, not a rule imposed on every
    // piece" discipline -- fase 48/61/85 already follow the same shape).
    // Only the three GENERATED rhythm slots above (sieve/aksak/euclid,
    // fase 85 stage 1) are eligible once a section's bar length differs
    // from 16 -- the fixed named-archetype catalog was hand-drawn
    // specifically for a 16-step bar; stretching or compressing it into
    // another length would misrepresent the archetype itself, the same
    // "recorte de escopo" already used in fases 78/82. groupSizeForBarLength
    // feeds the generalized metrical model (RhythmGenerator.hpp) with
    // whichever real grouping that length is actually felt in -- four
    // groups of three for 12, four of five for 20, two of seven for 14.
    bool meterVarianceEnabled = rhythmicProtagonist || identityUnit(cfg.seed, 82) < 0.12;
    int currentBarLength = 16;
    int currentGroupSize = 4;
    int meterChangeCount = 0;
    std::mt19937 meterRng(cfg.seed + 66000u);
    auto groupSizeForBarLength = [](int barLength) -> int {
        switch (barLength) {
            case 12: return 3;
            case 14: return 7;
            case 20: return 5;
            default: return 4;
        }
    };

    // Same idea extended to the rest of the final mix balance: the voice/
    // delay blend and the three circuit-texture caps (chiptune arpeggio,
    // Atari Punk Console, envelope-oscillator shimmer) were each one fixed
    // number for every piece. Now each is its own seed-derived range --
    // still centered near the old fixed value (so the overall balance
    // stays recognizable), but no two pieces land on exactly the same mix.
    bool nonCoreLead = identity.leadVoice != LeadVoiceModel::Core;
    float voiceMixLevel = nonCoreLead
        ? static_cast<float>(0.34 + unit(rng) * 0.18)
        : static_cast<float>(0.68 + unit(rng) * 0.24);
    if (rhythmicProtagonist) voiceMixLevel *= 0.72f;
    if (spectralProtagonist) voiceMixLevel *= 0.68f;
    float delayMixLevel = static_cast<float>(0.26 + unit(rng) * 0.18);
    if (gamePlan.strategy == GameStrategy::Conceal) voiceMixLevel *= 0.62f;
    if (gamePlan.strategy == GameStrategy::Counter) voiceMixLevel *= 0.80f;
    if (gamePlan.strategy == GameStrategy::Follow) voiceMixLevel *= 1.08f;
    if (gamePlan.strategy == GameStrategy::Transform) delayMixLevel *= 1.22f;
    float chiptuneCap = static_cast<float>(0.08 + unit(rng) * 0.10);
    float atariCap = static_cast<float>(0.04 + unit(rng) * 0.06);
    float envOscCap = static_cast<float>(0.03 + unit(rng) * 0.06);

    // Tempo/cadence variation within a single piece: most pieces still keep
    // a constant bpm (what you set on TEMPO is what you get, honestly --
    // the default, not a bug). About a third get the *capability*, but even
    // then a real tempo change is a discrete, rare event at a genuine arc
    // transition (same gate tonal modulation/modal shift already use below)
    // -- the piece jumps to a new bpm and holds it, rather than continuously
    // wobbling bar to bar. Both how deep a jump can be (tempoDriftDepth) and
    // how often a transition actually triggers one (tempoChangeChance) are
    // their own seed-derived ranges, not one fixed formula every piece
    // follows the same way.
    bool tempoDriftEnabled = unit(rng) < gamePlan.ruleChangeProbability(0.24);
    double tempoDriftDepth = tempoDriftEnabled ? (0.10 + unit(rng) * 0.20) : 0.0;
    double tempoChangeChance = tempoDriftEnabled ? (0.15 + unit(rng) * 0.35) : 0.0;

    // Fase 114, Eixo C -- modulacao metrica real (Elliott Carter): quando
    // este flag esta ligado, o mecanismo de troca de andamento acima
    // (ja existente) troca COMO o fator de mudanca e escolhido -- em vez
    // de um multiplicador continuo arbitrario, uma razao real de tercina/
    // quintina pequena (3:2, 4:3, 5:4 e as inversas), a mesma familia de
    // proporcoes que a pratica real de modulacao metrica usa (uma
    // subdivisao do pulso antigo vira o novo pulso, uma razao musical
    // perceptivel em vez de um salto arbitrario de bpm). Mutuamente
    // exclusivo por peca com a deriva continua original -- mesma
    // disciplina de "um modo limpo, nao empilhado" que todo par de modos
    // alternativos deste motor ja segue.
    bool metricModulationEnabled = tempoDriftEnabled && unit(rng) < 0.4;
    static const double kMetricModulationRatios[] = {
        1.5, 2.0 / 3.0, 4.0 / 3.0, 0.75, 1.25, 0.8, 5.0 / 3.0, 0.6
    };
    int metricModulationEventCount = 0;

    double currentBpm = cfg.bpm;
    int tempoChangeCount = 0;
    std::vector<TempoEvent> tempoTimeline;
    tempoTimeline.push_back({0.0, cfg.bpm});

    // Ritardando at the coda: real compositional practice (a gradual
    // slowing signalling arrival/closure, not a jump) applied at the one
    // structural moment it's actually named for, rather than folded into
    // the generic per-transition tempo-change roll above. Only meaningful
    // on pieces that already have the tempo-change capability at all, and
    // even then only about half of them -- some pieces still end at full
    // tempo on purpose, a hard stop rather than an easing-off.
    bool codaRitardandoEnabled = tempoDriftEnabled && unit(rng) < 0.5;
    double ritardandoTargetFactor = 0.55 + unit(rng) * 0.25;
    double ritardandoTauSeconds = 4.0 + unit(rng) * 8.0;
    bool ritardandoArmed = false;
    double ritardandoStartBpm = 0.0;
    double ritardandoStartSeconds = 0.0;

    // Fase 68 -- accelerando/stringendo: the ritardando's pair at the other
    // end of the arc, requested as an open item since fase 28. Same real
    // exponential glide (not a jump), armed exactly once when the climax
    // (ArcRole::Peak) is first entered, easing the tempo UP toward a
    // faster target instead of down toward a slower one. Left armed
    // through Falling too (a stringendo that's already built up momentum
    // doesn't undo itself the moment the climax technically ends) --
    // ritardando gets final say once the coda genuinely begins, same
    // priority order the discrete tempo-jump logic already respects.
    bool accelerandoEnabled = tempoDriftEnabled && unit(rng) < market.price("accelerando", 0.4);
    market.record("accelerando", accelerandoEnabled);
    double accelerandoTargetFactor = 1.15 + unit(rng) * 0.35;
    double accelerandoTauSeconds = 3.0 + unit(rng) * 6.0;
    bool accelerandoArmed = false;
    bool peakEnteredOnce = false;
    double accelerandoStartBpm = 0.0;
    double accelerandoStartSeconds = 0.0;

    // Real tonal-centre modulation across a piece -- the scale/root aren't
    // one fixed pitch collection for the whole render anymore. Most pieces
    // still stay in one key throughout (a genuine choice a composer can
    // make, not a limitation), but when enabled, each *new* arc section
    // gets its own chance to actually modulate (checked below, at bar
    // boundaries, alongside the existing per-section regenerate() call).
    // The interval is weighted away from the "obvious" circle-of-fifths
    // move (a plain fourth/fifth) toward less expected distances -- a
    // minor second, a tritone, a major seventh -- so a modulation reads as
    // a genuine surprise/new direction rather than a predictable cadence.
    // Roughly 60% of the time the scale itself changes too, not just the
    // root, so the new tonal area can have a genuinely different colour,
    // not just the same mode transposed.
    bool tonalModulationEnabled = unit(rng) < 0.45;
    int modulationCount = 0;
    std::string lastModulationSectionName;
    static constexpr int kModulationIntervals[9] = {1, 2, 3, 4, 6, 8, 9, 10, 11};
    static constexpr double kModulationWeights[9] = {1.4, 1.0, 1.2, 1.2, 1.6, 1.2, 1.0, 1.0, 1.4};

    // Modal shift: a cheaper, more frequent sibling to the tonal
    // modulation above -- same root, only the scale/mode changes, so the
    // tonal centre never actually moves, just recolours. Checked at the
    // same section-transition points, but only when a tonal modulation
    // didn't already fire there (the two are mutually exclusive per
    // transition, not stacked) -- a piece can have either capability, both,
    // or neither, independently seed-derived.
    bool modalShiftEnabled = unit(rng) < 0.55;
    int modalShiftCount = 0;

    // Free intonation: the escala/StepSequencer machinery above still
    // decides which note gets chosen (the "spine" this project inherited
    // from Kosmische stays intact) -- but the actual pitch heard can be
    // pulled meaningfully away from that quantized note by an independent
    // chaotic drift, well beyond the subtle Lorenz-driven wobble
    // pitchDriftSemitones already had. Deliberately rarer than most
    // character choices (~25%) since it's a real perceptual departure from
    // "in scale," not a subtle colour -- most pieces stay quantized to
    // their scale exactly as they always have. Standard/Chirikov map
    // chosen as the drift source specifically because it's not already
    // driving pitch anywhere else (it modulates the ring-mod carrier),
    // avoiding correlating this with the Lorenz drift already present.
    bool freeIntonationEnabled = unit(rng) < 0.25;
    double freeIntonationDepth = freeIntonationEnabled ? (2.0 + unit(rng) * 6.0) : 0.0;

    // Rungler: Rob Hordijk's Benjolin feedback core, two internal
    // oscillators cross-feeding an 8-bit shift register (see Rungler.hpp)
    // -- a qualitatively different instability from every chaos map above.
    // Those are continuous attractors, organic drift; this is a stepped,
    // digital, lurching thing, closer kin to the DIY Nonlinearcircuits
    // tradition than to Lorenz/Henon. Runs at two audio-adjacent-but-slow
    // rates (a handful of Hz), its own stepped output added as one more
    // pitch-drift term, same spot free intonation and hocket influence
    // already add theirs.
    bool runglerEnabled = unit(rng) < gameBias(0.3, gameWeightIlinx);
    Rungler rungler;
    rungler.setSampleRate(sampleRate);
    rungler.setRates(2.0 + unit(rng) * 6.0, 2.0 + unit(rng) * 9.0);
    rungler.setFeedbackDepth(runglerEnabled ? (0.4 + unit(rng) * 0.6) : 0.0);
    rungler.reset(static_cast<uint8_t>(rng() & 0xFFu));
    double runglerDepth = runglerEnabled ? (1.0 + unit(rng) * 3.0) : 0.0;

    // Leibniz Rectifier: dynamic rectification (Nonlinearcircuits) feeding
    // an 8-bit code with a few of its own bits deliberately flipped (Xaoc
    // Devices' Leibniz system) -- see LeibnizRectifier.hpp. Independent of
    // voiceColor above (this is a rupture that comes and goes with the
    // piece's own intensity, not a fixed colour chosen once), its presence
    // in the mix tracks the arc's resonance the same way the ring
    // modulator below already does, closer to inaudible in a quiet
    // opening, most present near a climax -- so the piece's most torn
    // sound and its most intense moment are never a coincidence.
    bool leibnizEnabled = unit(rng) < gameBias(0.3, gameWeightIlinx);
    LeibnizRectifier leibnizRect;
    leibnizRect.seed(cfg.seed + 27000u);
    leibnizRect.setRectifyAmount(0.4 + unit(rng) * 0.5);
    leibnizRect.setAsymmetry(unit(rng) * 2.0 - 1.0);
    leibnizRect.setBitCorruption(leibnizEnabled ? (0.15 + unit(rng) * 0.5) : 0.0);
    double leibnizMix = leibnizEnabled ? (0.35 + unit(rng) * 0.4) : 0.0;

    // "Arquivo apocrifo": most pieces never do this (a deliberate rarity,
    // not every piece rewriting its own history) -- about 40% get the
    // chance to interrupt themselves once (rarely twice) with a reversed,
    // bit-crushed quote of their own much-earlier self, read straight out
    // of this same render's own output buffer. See ApocryphalTear.hpp.
    bool apocryphalEnabled = unit(rng) < gameBias(0.4, gameWeightMimicry);
    ApocryphalTear apocryphal;
    apocryphal.seed(cfg.seed + 21000u);
    apocryphal.setSampleRate(sampleRate);
    if (apocryphalEnabled) {
        int maxOccurrences = unit(rng) < 0.25 ? 2 : 1;
        apocryphal.configure(0.045, maxOccurrences);
    }

    // "Corte de montagem": a real jump cut, same rarity class as
    // Curto-Circuito/elemento estranho -- a genuinely distant, unprocessed
    // quote spliced in and back out via true zero-crossing hard cuts, not
    // a fade. Same family as apocryphal tear above (a distant self-
    // citation), so biased by the same Mimicry weight. See
    // MontageEditCut.hpp for exactly how it differs from apocryphal
    // tear/Parenteses/Curto-Circuito.
    bool montageCutEnabled = unit(rng) < gameBias(0.16, gameWeightMimicry);
    MontageEditCut montageCut;
    montageCut.seed(cfg.seed + 62000u);
    montageCut.setSampleRate(sampleRate);
    if (montageCutEnabled) montageCut.configure(0.02, 1);

    // "Atropelo": began as "voz narrante" and grew, through design
    // conversation with the user, into something else entirely --
    // competing almost-voices that interrupt each other for real (a hard,
    // click-free cutoff, not a mix cession), remember an unfinished
    // phrase and may probabilistically resume it later picking up the
    // SAME intonation contour where it was cut, may genuinely forget it
    // instead if the memory pool fills up, and specifically target
    // stochastic mute's own silence windows as a place to intrude rather
    // than firing at any random moment. Same family as apocryphal tear/
    // corte de montagem (self-referential, simulacro-adjacent), biased by
    // the same Mimicry weight.
    bool atropeloEnabled = unit(rng) < gameBias(0.14, gameWeightMimicry);
    AtropeloEnsemble atropelo;
    atropelo.setSampleRate(sampleRate);
    if (atropeloEnabled) atropelo.seed(cfg.seed + 91000u);
    double atropeloFireChancePerBar = 0.04 + unit(rng) * 0.03;
    int atropeloSilenceInvasionCount = 0;
    int atropeloNestingCount = 0;

    // "Garantia de limite": a peca precisa, de algum jeito seu, se
    // arriscar de verdade -- nao um mecanismo especifico obrigatorio, mas
    // a exigencia de que ao menos um entre os que realmente levam a peca a
    // um limite real (arquivo apocrifo, entonacao livre escapando da
    // escala, modulacao tonal, self-feedback de audio) esteja presente.
    // Os quatro sorteios acima sao independentes -- a maioria das pecas ja
    // acerta pelo menos um por acaso, mas nada garantia isso: uma peca
    // podia, em tese, sair mansa demais, sem nunca se testar. So entra em
    // acao quando os quatro ja sortearam "nao"; nesse caso, um deles e
    // forcado a ligar, escolhido pela propria seed -- a rede de seguranca
    // decide QUE algo aconteca, nunca QUAL.
    bool intensityGuaranteeTriggered = false;
    if (!apocryphalEnabled && !freeIntonationEnabled && !tonalModulationEnabled && !selfFeedbackEnabled) {
        intensityGuaranteeTriggered = true;
        int forced = static_cast<int>(unit(rng) * 4.0) % 4;
        switch (forced) {
            case 0:
                apocryphalEnabled = true;
                apocryphal.configure(0.045, unit(rng) < 0.25 ? 2 : 1);
                break;
            case 1:
                freeIntonationEnabled = true;
                freeIntonationDepth = 2.0 + unit(rng) * 6.0;
                break;
            case 2:
                tonalModulationEnabled = true;
                break;
            case 3:
            default:
                selfFeedbackEnabled = true;
                break;
        }
    }

    // Fase 84 -- "Curto-Circuito": a real hard-gate dropout, not a fade
    // (the point is a genuine discontinuity, the same signature a
    // waveform/spectrogram analysis of a real reference track showed as
    // its most audible structural event), paired with a second effect at
    // the exact same instant -- a curated handful of mechanisms the seed
    // had already decided to leave OFF for this piece get a real, elevated
    // second chance to force their way in anyway. Not "reshuffling what's
    // already on the table" (that's what the Caillois weights already
    // do to probabilities before anything fires) -- this is mechanisms
    // that already lost their own roll crashing the piece uninvited,
    // literally the same "forced activation" idiom the fase-40 intensity
    // guarantee above already established (flip the flag, configure the
    // object inline, same as apocryphal/freeIntonation/tonalModulation/
    // selfFeedback there), just invoked by a mid-piece rupture instead of
    // a start-of-piece safety net. Biased by Ilinx (vertigo/loss of
    // control over the machine's own protocol) -- the most direct
    // Caillois fit of any mechanism in the engine.
    bool curtoCircuitoEnabled = unit(rng) < gameBias(0.18, gameWeightIlinx);
    bool curtoCircuitoFired = false;
    int curtoCircuitoGatecrashCount = 0;

    // Fase 101 -- "arquivo apocrifo aprofundado": leitura apocrifa. A
    // real, explicitly-labeled counterfactual passage in the self-epitaph
    // -- not a lie inserted into the factual record (that stays untouched
    // everywhere else), but a separate, clearly-marked account of a real
    // near-miss this piece's own generative process had. Benjamin's
    // "arquivo dos vencidos" (fase 101 design conversation): the losing
    // rolls of a few single-real-event mechanisms (curto-circuito, galo,
    // nota de rodape), not just the winners, captured without consuming
    // any extra unit(rng) draws (the roll already happens regardless;
    // this only keeps the value the existing check already discarded).
    struct ApocryphalNearMiss { std::string mechanism; double closeness; };
    std::vector<ApocryphalNearMiss> apocryphalNearMisses;
    // Named, not just counted -- the txt score/JSON report exactly which
    // mechanisms crashed the piece, the same honesty standard the
    // probability market and the foreign-signal mechanism already hold
    // themselves to.
    std::string curtoCircuitoGatecrashedNames;
    double curtoCircuitoFireChancePerBar = 0.02 + unit(rng) * 0.02;
    int curtoCircuitoRampSamples = static_cast<int>(0.003 * sampleRate);
    int curtoCircuitoHoldSamples = 0;
    int curtoCircuitoSamplesIntoEvent = -1;
    double curtoCircuitoGateEnv = 1.0;

    // "Static pop" -- two Noise Cornucopia-inspired ideas not yet implemented at that stage
    // (grainy noise, noise-derived random gates) combined into one new
    // voice: a noise-derived trigger (genuinely different timing
    // statistics from both the XOR clock divider and the chaos-map
    // zero-crossings already driving every other percussion voice) firing
    // a short burst of grainy, discretized noise (not the smooth/filtered
    // noise this engine already uses for the snare/clap/maracas). Present
    // in only about 40% of pieces -- a seed-derived character choice, same
    // pattern as voiceColor/loFiDelay above, not a universal addition.
    bool staticPopEnabled = unit(rng) < 0.4;
    NoiseGateGenerator staticPopGate; staticPopGate.setSampleRate(sampleRate);
    staticPopGate.setNoiseRate(35.0 + unit(rng) * 40.0);
    staticPopGate.setDivider(3 + static_cast<int>(unit(rng) * 3.0)); // /8 .. /64
    staticPopGate.seed(cfg.seed + 23000u);
    GrainyNoise staticPopNoise;
    staticPopNoise.seed(cfg.seed + 24000u);
    staticPopNoise.setGraininess(0.3f + static_cast<float>(unit(rng)) * 0.4f);
    float staticPopEnv = 0.0f;
    float staticPopDecay = static_cast<float>(decayCoeff(0.025, sampleRate)); // short pop, ~25ms

    // Holds the previous sample's LinCongMap value -- the gated-step block
    // below (which decides the AY8910 arpeggio's glitch) runs earlier in
    // the loop than this sample's own chaos ticks, so it reads a one-
    // sample-old value; harmless (imperceptible at audio rate), same
    // pattern as stepCV being computed once and read by several later
    // consumers elsewhere in this loop.
    float lincongVal = 0.0f;

    // Tom/conga: TomVoice is KickVoice retuned (see DrumVoices.hpp). Three
    // instances, one shared trigger stage-pair -- each hit cycles to the
    // next one (low/mid/hi) round-robin, since a single trigger source
    // budget (six XOR stage-pairs, already spent on kick/snare/hihat/
    // cowbell/clap) doesn't stretch to three fully independent voices.
    std::array<TomVoice, 3> toms;
    std::array<double, 3> tomAttackCurveBase{};
    static const double tomBases[3] = {90.0, 140.0, 210.0};
    for (int i = 0; i < 3; ++i) {
        toms[static_cast<size_t>(i)].setSampleRate(sampleRate);
        toms[static_cast<size_t>(i)].setTuning(tomBases[i] + (unit(rng) - 0.5) * 12.0, 30.0 + unit(rng) * 20.0);
        // Longer than the kick -- toms sustain more -- and now jittered per
        // seed too, same as every other voice above.
        toms[static_cast<size_t>(i)].setDecayTimes(0.10 + unit(rng) * 0.14, 0.28 + unit(rng) * 0.30);
        tomAttackCurveBase[static_cast<size_t>(i)] = 0.76 + unit(rng) * 0.48;
        toms[static_cast<size_t>(i)].setAttackCurve(tomAttackCurveBase[static_cast<size_t>(i)]);
    }
    int tomCycle = 0;

    // Rhythmic humanization: off for about half of pieces (a perfectly
    // quantized grid is a valid character choice too, not a flaw). When
    // on, every percussion trigger below fires a small random number of
    // samples *after* its nominal grid position instead of exactly on it
    // -- delay-only (no lookahead scheduling exists to land a hit early),
    // same "imprecisão de contagem" a human player's timing actually has,
    // not a mechanical grid.
    bool rhythmHumanizeEnabled = unit(rng) < 0.5;
    int humanizeMaxSamples = rhythmHumanizeEnabled
        ? static_cast<int>((0.003 + unit(rng) * 0.009) * sampleRate)
        : 0;
    int relationalRhythmDelaySamples = 0;
    double relationalRhythmVelocityScale = 1.0;
    int humanizeApplicationCount = 0;
    int swingApplicationCount = 0;
    int lastHumanizationSamples = 0;
    auto rollHumanizeDelay = [&](bool structuredGroove = false) {
        // Preserve RNG consumption, but the new primary-kit studies use
        // voice/phrase placement rather than stacking independent jitter.
        const int rolledSamples = humanizeMaxSamples > 0
            ? static_cast<int>(unit(rng) * humanizeMaxSamples) : 0;
        if (!structuredGroove && rhythmHumanizeEnabled && humanizeMaxSamples > 0)
            ++humanizeApplicationCount;
        lastHumanizationSamples = structuredGroove ? 0 : rolledSamples;
        return lastHumanizationSamples + relationalRhythmDelaySamples;
    };

    // Fase 71 -- "Heisenberg": rhythmic precision and pitch precision get
    // coupled INVERSELY for a fraction of pieces, instead of the two
    // independent rolls above (rhythmHumanizeEnabled/freeIntonationEnabled).
    // A single seed-derived "balance" decides both at once: near 0, the
    // rhythm locks down close to a mechanical grid while the pitch is
    // allowed to wander widely; near 1, the pitch locks close to the
    // scale while the rhythm is allowed to fracture loosely -- the piece
    // is never allowed BOTH kinds of precision simultaneously, an uncertainty
    // trade-off rather than two independent character rolls. Overrides
    // whatever the independent rolls above (and the intensity-guarantee
    // fallback, if it fired) already set -- deliberately the last word for
    // pieces where this fires, same "one clean mode, not layered"
    // discipline as every other alternate-mode pair in this file.
    bool heisenbergEnabled = unit(rng) < 0.25;
    double heisenbergBalance = 0.0;
    if (heisenbergEnabled) {
        heisenbergBalance = unit(rng);
        rhythmHumanizeEnabled = true;
        humanizeMaxSamples = static_cast<int>((0.0005 + heisenbergBalance * 0.020) * sampleRate);
        freeIntonationEnabled = true;
        freeIntonationDepth = 0.3 + (1.0 - heisenbergBalance) * 9.7;
    }
    int kickDelay = -1, snareDelay = -1, hihatDelay = -1, cowbellDelay = -1,
        clapDelay = -1, cymbalDelay = -1, maracasDelay = -1;
    bool hihatDelayedOpen = false;
    GrooveRepeat snareRepeat, hihatRepeat;
    std::array<int, 3> tomDelay = {-1, -1, -1};

    ExpressiveEventLedger expressiveEventLedger(cfg.expressiveEventLedgerEnabled);
    // Phase 174: the expressive layer consults the seed's actual performer
    // population. These are capability actors, not a second module catalog:
    // absent voices remain absent and infrastructure never enters the set.
    PerformanceActorRegistry performanceActors;
    if (drumsEnabled) {
        performanceActors.add({0, PerformanceGestureFamily::Hit,
                               AgencyFamily::Rhythm,
                               PerformanceCapability::Impact
                                   | PerformanceCapability::DynamicTrajectory
                                   | PerformanceCapability::ReleaseShape,
                               true, false, 0.78, 0.62, 1.0});
    }
    if (granularTextureEnabled || gendynTextureEnabled || coloredNoiseBedEnabled) {
        performanceActors.add({1, PerformanceGestureFamily::Texture,
                               AgencyFamily::Texture,
                               PerformanceCapability::SpectralTrajectory
                                   | PerformanceCapability::DynamicTrajectory
                                   | PerformanceCapability::OnsetShape,
                               true, false, 0.46, 0.48, 0.72});
    }
    if (fmFluteEnabled || thereminEnabled || tubaVoiceEnabled) {
        performanceActors.add({2, PerformanceGestureFamily::Tone,
                               AgencyFamily::Instrument,
                               PerformanceCapability::PitchTrajectory
                                   | PerformanceCapability::DynamicTrajectory
                                   | PerformanceCapability::Transition
                                   | PerformanceCapability::ReleaseShape,
                               true, false, 0.64, 0.50, 0.68});
    }
    PerformanceSelectionRequest expressiveHitRequest;
    expressiveHitRequest.constrainFamily = true;
    expressiveHitRequest.family = PerformanceGestureFamily::Hit;
    expressiveHitRequest.maximumActors = 1;
    PerformanceSelectionRequest expressiveTextureRequest;
    expressiveTextureRequest.constrainFamily = true;
    expressiveTextureRequest.family = PerformanceGestureFamily::Texture;
    expressiveTextureRequest.maximumActors = 1;
    RasgarTimingMemory expressiveTimingMemory;
    RasgarAccentField expressiveAccentField;
    RasgarPercussionArticulationAdapter expressiveArticulationAdapter;
    int expressiveActuationEventCount = 0;
    double expressiveKickDecayScale = 1.0;
    double expressiveSnareDecayScale = 1.0;
    double expressiveHihatDecayScale = 1.0;
    double expressiveCowbellDecayScale = 1.0;
    double expressiveKickPitchSemitones = 0.0;
    double expressiveSnarePitchSemitones = 0.0;
    double expressiveHihatPitchSemitones = 0.0;
    double expressiveCowbellPitchSemitones = 0.0;
    double expressiveKickTimbreAmount = 0.0;
    double expressiveSnareTimbreAmount = 0.0;
    double expressiveHihatTimbreAmount = 0.0;
    double expressiveCowbellTimbreAmount = 0.0;
    double expressiveKickGate = 1.0;
    double expressiveSnareGate = 1.0;
    double expressiveHihatGate = 1.0;
    double expressiveCowbellGate = 1.0;
    RasgarGateWindow expressiveKickGateWindow;
    RasgarGateWindow expressiveSnareGateWindow;
    RasgarGateWindow expressiveHihatGateWindow;
    RasgarGateWindow expressiveCowbellGateWindow;
    for (int domain = 0; domain < static_cast<int>(ExpressiveDomain::Count); ++domain) {
        expressiveEventLedger.markAvailable(static_cast<ExpressiveDomain>(domain));
    }
    auto observePercussionPlan = [&](ExpressiveEventSource source,
                                     std::uint64_t nominalSample,
                                     int step,
                                     int totalDelay,
                                     double velocity,
                                     int humanizationSamples,
                                     int swingSamples,
                                     int phraseGestureSamples,
                                     int rasgarGestureSamples,
                                     int relationSamples,
                                     double stepDurationSeconds,
                                     double density,
                                     bool ghost = false,
                                     bool ratchet = false) {
        ExpressiveEventIntent intent;
        intent.source = source;
        intent.scale = ghost || ratchet
            ? ExpressiveScale::Microevent : ExpressiveScale::Step;
        intent.nominalSample = nominalSample;
        intent.step = step;
        intent.bar = barCounter;
        intent.section = std::max(0, static_cast<int>(arc.history().size()) - 1);
        intent.velocity = velocity;
        intent.durationSeconds = stepDurationSeconds;
        intent.density = density;
        intent.accent = velocity;
        intent.domains = expressiveBit(ExpressiveDomain::Timing)
            | expressiveBit(ExpressiveDomain::Energy)
            | expressiveBit(ExpressiveDomain::Duration);
        intent.causes = expressiveBit(ExpressiveCause::Grid)
            | expressiveBit(ExpressiveCause::MetricAccent)
            | expressiveBit(ExpressiveCause::Density);
        if (humanizationSamples != 0) {
            intent.causes |= expressiveBit(ExpressiveCause::Humanization);
        }
        if (swingSamples != 0) {
            intent.causes |= expressiveBit(ExpressiveCause::Swing);
        }
        if (phraseGestureSamples != 0) {
            intent.causes |= expressiveBit(ExpressiveCause::PhraseGesture);
        }
        if (rasgarGestureSamples != 0) {
            intent.causes |= expressiveBit(ExpressiveCause::RasgarGesture);
        }
        if (relationSamples != 0) {
            intent.causes |= expressiveBit(ExpressiveCause::Relation);
        }
        if (percussionTimbreVariationEnabled) {
            intent.domains |= expressiveBit(ExpressiveDomain::Material);
            intent.causes |= expressiveBit(ExpressiveCause::TimbreVariation);
        }
        intent.ghost = ghost;
        intent.ratchet = ratchet;

        ExpressiveEventPlan plan;
        plan.performedSample = nominalSample
            + static_cast<std::uint64_t>(std::max(0, totalDelay));
        plan.timing.humanizationSamples = humanizationSamples;
        plan.timing.swingSamples = swingSamples;
        plan.timing.phraseGestureSamples = phraseGestureSamples;
        plan.timing.rasgarGestureSamples = rasgarGestureSamples;
        plan.timing.relationSamples = relationSamples;
        plan.performedVelocity = velocity;
        plan.performedDurationSeconds = intent.durationSeconds;
        plan.materialAmount = percussionTimbreVariationEnabled
            ? percussionTimbreDepth : 0.0;
        expressiveEventLedger.observe(intent, plan);
    };

    auto observeExpressiveEvent = [&](ExpressiveEventSource source,
                                      ExpressiveScale scale,
                                      std::uint64_t nominalSample,
                                      int step,
                                      double pitchMidi,
                                      double velocity,
                                      double durationSeconds,
                                      double density,
                                      double tension,
                                      double entropy,
                                      ExpressiveMask domains,
                                      ExpressiveMask causes,
                                      double materialAmount = 0.0,
                                      double spatialAmount = 0.0,
                                      bool ghost = false,
                                      bool ratchet = false) {
        ExpressiveEventIntent intent;
        intent.source = source;
        intent.scale = scale;
        intent.nominalSample = nominalSample;
        intent.step = step;
        intent.bar = barCounter;
        intent.section = std::max(0, static_cast<int>(arc.history().size()) - 1);
        intent.pitchMidi = pitchMidi;
        intent.velocity = velocity;
        intent.durationSeconds = durationSeconds;
        intent.density = density;
        intent.accent = std::clamp(velocity, 0.0, 1.0);
        intent.tension = tension;
        intent.entropy = entropy;
        intent.domains = domains;
        intent.causes = causes;
        intent.ghost = ghost;
        intent.ratchet = ratchet;

        ExpressiveEventPlan plan;
        plan.performedSample = nominalSample;
        plan.performedPitchMidi = pitchMidi;
        plan.performedVelocity = velocity;
        plan.performedDurationSeconds = durationSeconds;
        plan.materialAmount = materialAmount;
        plan.spatialAmount = spatialAmount;
        expressiveEventLedger.observe(intent, plan);
    };

    auto applyExpressivePercussion = [&](RasgarTimingVoice voice,
                                         float &velocity,
                                         int &delay,
                                         int step,
                                         double resonance,
                                         double density,
                                         int stepSamples) {
        if (!cfg.expressiveActuationEnabled
            || performanceActors.select(cfg.seed,
                                        static_cast<std::uint32_t>(barCounter),
                                        expressiveHitRequest).empty()) return;
        expressiveTimingMemory.beginSection(voice, barCounter, gestureBias * 0.5, 0.2);
        RasgarAccentInput input;
        input.metric = metricalStrengthForBar(step, currentBarLength, currentGroupSize);
        input.structural = std::clamp(resonance, 0.0, 1.0);
        input.relational = std::clamp(
            std::abs(static_cast<double>(relationalRhythmDelaySamples))
                / std::max(1.0, static_cast<double>(stepSamples)),
            0.0, 1.0);
        input.phrase = std::clamp(std::abs(gestureBias), 0.0, 1.0);
        input.density = density;
        const HitGesturePlan hitGesture = PerformanceGesturePlanner::planHit(
            PerformerState{}, input.metric, density,
            std::clamp(0.24 + std::abs(gestureBias) * 0.34, 0.0, 1.0));
        input.phrase = std::clamp(input.phrase * 0.84
                                  + hitGesture.residue * 0.16, 0.0, 1.0);
        const RasgarAccentPlan accent = expressiveAccentField.evaluate(
            input, velocity, static_cast<double>(stepSamples) / sampleRate,
            0.25);
        const RasgarPercussionArticulation articulation =
            expressiveArticulationAdapter.translate(
                voice == RasgarTimingVoice::Kick ? RasgarPercussionVoice::Kick
                    : voice == RasgarTimingVoice::Snare ? RasgarPercussionVoice::Snare
                    : voice == RasgarTimingVoice::Hihat ? RasgarPercussionVoice::Hihat
                    : RasgarPercussionVoice::Cowbell,
                accent, 0.82 + hitGesture.integrity * 0.18);
        switch (voice) {
            case RasgarTimingVoice::Kick: expressiveKickDecayScale = articulation.decayScale; break;
            case RasgarTimingVoice::Snare: expressiveSnareDecayScale = articulation.decayScale; break;
            case RasgarTimingVoice::Hihat: expressiveHihatDecayScale = articulation.decayScale; break;
            case RasgarTimingVoice::Cowbell: expressiveCowbellDecayScale = articulation.decayScale; break;
            case RasgarTimingVoice::Count: break;
        }
        switch (voice) {
            case RasgarTimingVoice::Kick: expressiveKickPitchSemitones = articulation.microPitchSemitones; break;
            case RasgarTimingVoice::Snare: expressiveSnarePitchSemitones = articulation.microPitchSemitones; break;
            case RasgarTimingVoice::Hihat: expressiveHihatPitchSemitones = articulation.microPitchSemitones; break;
            case RasgarTimingVoice::Cowbell: expressiveCowbellPitchSemitones = articulation.microPitchSemitones; break;
            case RasgarTimingVoice::Count: break;
        }
        switch (voice) {
            case RasgarTimingVoice::Kick: expressiveKickTimbreAmount = articulation.timbreAmount; break;
            case RasgarTimingVoice::Snare: expressiveSnareTimbreAmount = articulation.timbreAmount; break;
            case RasgarTimingVoice::Hihat: expressiveHihatTimbreAmount = articulation.timbreAmount; break;
            case RasgarTimingVoice::Cowbell: expressiveCowbellTimbreAmount = articulation.timbreAmount; break;
            case RasgarTimingVoice::Count: break;
        }
        switch (voice) {
            case RasgarTimingVoice::Kick: expressiveKickGate = articulation.gate; break;
            case RasgarTimingVoice::Snare: expressiveSnareGate = articulation.gate; break;
            case RasgarTimingVoice::Hihat: expressiveHihatGate = articulation.gate; break;
            case RasgarTimingVoice::Cowbell: expressiveCowbellGate = articulation.gate; break;
            case RasgarTimingVoice::Count: break;
        }
        velocity = static_cast<float>(accent.velocity);
        const double budgetSamples = (cfg.wildMode ? 0.012 : 0.006) * sampleRate;
        const double requested = (input.metric - 0.5) * budgetSamples;
        const int residual = static_cast<int>(std::lround(
            expressiveTimingMemory.observe(voice, requested, budgetSamples)));
        delay = std::max(0, delay + residual);
        ++expressiveActuationEventCount;
    };

    // Three more per-piece drum-dynamics mechanisms, all reusing voices
    // and patterns already built rather than new DSP math -- direct
    // response to "as outras vozes tambem passam por procedimentos
    // especiais?" (only kick/snare had chaos contagion until now).

    // 1. Hihat joins chaos contagion (same mechanism as kick/snare above,
    // its own chaos source so the three voices don't move in lockstep).
    // 2. Toms occasionally ratchet -- a real drummer's fill/roll, not
    // every hit, 1-3 fast extra retriggers of the SAME tom voice.
    bool tomRatchetEnabled = unit(rng) < 0.2;
    std::array<int, 3> tomRatchetRemaining = {0, 0, 0};
    std::array<int, 3> tomRatchetCountdown = {0, 0, 0};
    int tomRatchetGapSamples = static_cast<int>((0.035 + unit(rng) * 0.05) * sampleRate);
    int tomRatchetCount = 0;
    // 3. Cymbal occasionally rings out much longer (crash decaying into a
    // sustain vs. its usual tighter hit) -- a rare per-hit choice, not a
    // per-piece constant, so the same piece can have both short accents
    // and one long-ringing crash.
    bool cymbalSustainEnabled = unit(rng) < 0.2;
    int cymbalSustainCount = 0;

    DelayLine delay; delay.setSampleRate(sampleRate);
    delay.setFeedback(0.35);
    // Roughly 40% of pieces get the PT2399-style lo-fi delay character
    // (see DelayLine::setLoFi) instead of the clean digital repeat -- a
    // seed-derived choice, not a universal "upgrade," so pieces keep
    // sounding genuinely different from each other rather than all
    // converging on one "improved" delay colour.
    bool loFiDelay = unit(rng) < 0.4;
    delay.setLoFi(loFiDelay);

    SimpleReverb reverb; reverb.setSampleRate(sampleRate);
    reverb.setRoomSize(0.6); reverb.setDamping(0.4);

    // Stereo output, phase 16: everything upstream of this point stays a
    // single mono signal (the voice/filter/percussion mix) -- real content
    // difference between channels comes from here down, not from
    // amplitude-panning an identical mono source. A second delay/reverb
    // tap, seed-derived slightly out of sync with the left ones (delay
    // time ratio, room size), decorrelates the two channels' time-based
    // tails the way a real stereo rig would (mono dry signal, wide
    // time-effects), instead of a fake "stereo" that's just the same
    // waveform copied to both channels.
    DelayLine delayR; delayR.setSampleRate(sampleRate);
    delayR.setFeedback(0.35);
    delayR.setLoFi(loFiDelay); // same character choice as the left delay above
    double delayTimeRRatio = 1.0 + (unit(rng) - 0.5) * 0.14; // +-7%

    SimpleReverb reverbR; reverbR.setSampleRate(sampleRate);
    const double reverbRRoomSize = std::clamp(
        0.6 + (unit(rng) - 0.5) * 0.2, 0.2, 0.9);
    reverbR.setRoomSize(reverbRRoomSize);
    reverbR.setDamping(0.4);

    // A slow pan LFO automates the stereo position of the shared mono
    // core over the piece -- seed-derived rate/shape, same "breathing"
    // range as LfoBank's four LFOs (0.03-0.3Hz) so it reads as another
    // slow modulation layer, not a jarring ping-pong effect.
    Lfo panLfo;
    panLfo.setSampleRate(sampleRate);
    panLfo.setShape(unit(rng) < 0.5 ? Lfo::Shape::Sine : Lfo::Shape::Triangle);
    panLfo.setFrequency(0.03 + unit(rng) * 0.17);
    panLfo.setDepth(0.7f); // never pans hard left/right -- stays a drift, not a wipe
    panLfo.seed(cfg.seed + 21000u);

    ClockedRandomSource stepRandom;
    stepRandom.setSampleRate(sampleRate);
    stepRandom.setClockFrequency(0.4);

    // The four LFOs kosmische.sc always had but its UI never exposed --
    // shape and rate per-LFO chosen from the seed (0.03-0.3Hz: slow enough
    // to read as "breathing" underneath the audio-rate chaos above, not as
    // audible modulation of its own). Depths are fixed per target below,
    // at the point each one gets used, same as the other modulation sources
    // in this render loop.
    LfoBank lfoBank; lfoBank.setSampleRate(sampleRate);
    static const Lfo::Shape kLfoShapes[4] = {
        Lfo::Shape::Sine, Lfo::Shape::Triangle, Lfo::Shape::Pulse, Lfo::Shape::SampleHold
    };
    for (int i = 0; i < LfoBank::kNumLfos; ++i) {
        auto &l = lfoBank.lfo(i);
        l.setShape(kLfoShapes[static_cast<int>(unit(rng) * 4.0) % 4]);
        l.setFrequency(0.03 + unit(rng) * 0.27);
        l.setDepth(1.0f);
        l.seed(cfg.seed + 5000u + static_cast<unsigned>(i));
    }

    // Section memory for percussion: reuse the existing LFO/S&H primitive,
    // but latch it only when the arc enters a new named section. The held
    // value colours a passage without becoming another continuous wobble.
    Lfo percussionSectionHoldSource;
    percussionSectionHoldSource.setSampleRate(sampleRate);
    percussionSectionHoldSource.setShape(Lfo::Shape::SampleHold);
    percussionSectionHoldSource.setFrequency(0.035 + unit(rng) * 0.08);
    percussionSectionHoldSource.setDepth(1.0f);
    percussionSectionHoldSource.seed(cfg.seed + 5400u);
    float percussionSectionHold = 0.0f;

    // Buchla sketched a "Dual Control Voltage Integrator" (Model 155) --
    // "Need circuit" handwritten right on the schematic, apparently never
    // built -- a lag/slew processor that turns a stepped CV's hard jumps
    // into glides. Chaining one after the stepped random source it was
    // filed alongside (ClockedRandomSource, modeling the Random Voltage
    // Source it also never shipped as a real module) turns delay
    // time/reverb mix's instant per-clock jumps into portamento-like
    // swoops instead.
    LagProcessor delayTimeLag; delayTimeLag.setSampleRate(sampleRate); delayTimeLag.setTimes(180.0, 180.0);
    LagProcessor reverbMixLag; reverbMixLag.setSampleRate(sampleRate); reverbMixLag.setTimes(180.0, 180.0);

    // And the "Dual Signal Leveler" (Model 172) on the same page --
    // "We don't make. Why?" -- an automatic gain unit rather than a hard
    // clipper. Applied right before the final tanh() safety net, evening
    // out the arc's deliberate swing from a quiet "abertura" to a loud
    // "colapso" instead of leaving that entirely to soft clipping.
    SignalLeveler leveler; leveler.setSampleRate(sampleRate);
    // Chaos-driven modulation (Henon/Lorenz/Latoocarfian, now joined by the
    // resonator/granular layers) can occasionally wander into a louder
    // trajectory than a given seed's regression test happened to sample --
    // that is what chaos means. followMs is kept short and the gain floor
    // low so the leveler can duck a sudden loud passage fast and hard
    // rather than relying on tanh() alone to keep it merely bounded
    // (bounded is not the same as not distorted).
    leveler.setTargetLevel(0.22f); leveler.setTimes(15.0, 150.0, 25.0); leveler.setGainRange(0.12f, 1.3f);

    // Final-stage audio hygiene (replaces a blanket tanh() that ran on
    // every sample regardless of whether it was needed): a DC blocker per
    // channel (diode-mixed voices clip asymmetrically by design, which can
    // leave a real DC bias nothing downstream otherwise removes) and a
    // lookahead limiter that stays transparent -- gain 1.0, no processing
    // -- whenever the signal is comfortably under its ceiling, only
    // engaging smoothly near an actual peak. Deliberately not a loudness-
    // maximizing brickwall: the point is technical cleanliness (no clicks,
    // no unnecessary coloration), not flattening the dynamic range that's
    // this engine's whole character.
    DcBlocker dcBlockL; dcBlockL.setSampleRate(sampleRate);
    DcBlocker dcBlockR; dcBlockR.setSampleRate(sampleRate);
    LinkedOverloadProtector overloadProtector;
    // The upper knee deliberately leaves reconstruction headroom: highly
    // discontinuous/near-Nyquist material can intersample-overshoot far above
    // its sample peak. The final true-peak limiter then needs only the last
    // few dB, rather than acting as an emergency mixer.
    overloadProtector.setThresholdAndLimit(0.68f, 0.82f);
    LookaheadLimiter limiter; limiter.setSampleRate(sampleRate);
    limiter.setCeiling(0.89125094f); // -1 dBTP technical ceiling

    size_t samplesPerStep = static_cast<size_t>(secondsPerStep * sampleRate);
    size_t samplesPerBar = samplesPerStep * static_cast<size_t>(stepsPerBar);
    // Boundary-scheduled rather than a fixed i % samplesPerStep check, since
    // tempoDriftEnabled can change samplesPerStep/samplesPerBar from one bar
    // to the next -- a plain modulo against a moving denominator wouldn't
    // land on consistent boundaries.
    size_t nextStepBoundary = 0;
    size_t nextBarBoundary = 0;
    size_t stepCounter = 0;

    // Fase 111 -- phasing's own independent read-head clock: same idea as
    // samplesPerStep/nextStepBoundary above, a second, separate schedule
    // rather than a fixed ratio applied to the primary one, since
    // phaseSamplesPerStep needs recomputing at every bar boundary too
    // (right alongside samplesPerStep, whether or not tempoDriftEnabled
    // is what moved it that bar -- see the bar-boundary block below).
    size_t phaseSamplesPerStep = phasingEnabled
        ? std::max<size_t>(1, static_cast<size_t>(static_cast<double>(samplesPerStep) / (1.0 + phasingDelta)))
        : samplesPerStep;
    size_t phaseNextStepBoundary = 0;
    size_t phaseStepCounter = 0;
    int phaseGlobalStep = -1;

    std::vector<float> outSamples;
    // A reservation hint, not a cap -- typical pieces land in the same
    // 60-90s ballpark they always have, but the walk can run shorter or
    // longer (up to cfg.maxDurationSeconds); the vector just reallocates
    // past this if a given seed's walk runs long.
    outSamples.reserve(static_cast<size_t>(90.0 * sampleRate) * 2);

    int globalStep = -1;
    int currentMidiNote = pieceRootMidiNote;
    unsigned directedMelodyNoteIndex = 0;
    bool melodicPitchTelemetryInitialized = false;
    int melodicPreviousTelemetryNote = currentMidiNote;
    int melodicCurrentRepeatedSteps = 0;
    int melodicPitchChangeCount = 0;
    int melodicRepeatedPitchCount = 0;
    int melodicLongestRepeatedSteps = 0;
    int melodicPitchMin = currentMidiNote;
    int melodicPitchMax = currentMidiNote;
    bool codaNoteOffSent = false;

    // "Morte termica": how a piece dies, not just that it ends. Coda
    // already releases the melodic voice above (codaNoteOffSent); this
    // shapes the *whole mix's* final moments instead of leaving only that
    // one voice's release to carry the ending -- the arc's own coda role,
    // read from the outside via arc.inCoda(), same hook the melodic
    // release already uses. Two seed-decided characters: most of the time
    // a slow fade, the sound suffocating and losing harmonics like a
    // machine running out of power; more rarely (thermalAbruptDeath) a
    // fast, clean cutoff instead -- never a literal instant jump to zero
    // (that would click), just a much steeper exponential, still smooth
    // sample to sample.
    bool thermalFadeEnabled = unit(rng) < gameBias(0.4, gameWeightIlinx);
    bool thermalAbruptDeath = thermalFadeEnabled && unit(rng) < 0.25;
    double thermalFadeTauSeconds = thermalAbruptDeath ? (0.15 + unit(rng) * 0.3) : (2.5 + unit(rng) * 4.0);
    double thermalStaticDepth = thermalFadeEnabled ? (0.03 + unit(rng) * 0.05) : 0.0;
    double thermalLpMin = 200.0 + unit(rng) * 300.0;
    size_t thermalSamplesIntoCoda = 0;
    float thermalLpStateL = 0.0f, thermalLpStateR = 0.0f;
    std::mt19937 thermalNoiseRng(cfg.seed + 29000u);
    std::uniform_real_distribution<double> thermalUnit(-1.0, 1.0);

    // Rossler attractor: a spiral that expands slowly on its own, then
    // erupts once it swings past a threshold, instead of an attractor that
    // just wanders continuously like Lorenz/Henon already do. Ticks every
    // sample like the other chaos sources (small, mostly-inaudible
    // contribution to the filter cutoff below), but its real job is the
    // rare moment rossler.justErupted() fires -- only acted on while a
    // section is still minimal (state.density low), so it reads as "a calm
    // passage suddenly destabilizing," never as random filter noise
    // stacked on an already-busy climax.
    bool rosslerEnabled = unit(rng) < gameBias(0.3, gameWeightIlinx);
    RosslerMap rossler; rossler.setSampleRate(sampleRate);
    rossler.setRate(30.0 + unit(rng) * 50.0);
    rossler.setInitial(0.5 + unit(rng) * 1.5, 0.5 + unit(rng) * 1.5, 0.5 + unit(rng) * 1.5);
    double rosslerKickSeconds = 0.15 + unit(rng) * 0.35;
    float rosslerKickDecayCoeff = static_cast<float>(std::exp(-1.0 / (rosslerKickSeconds * sampleRate)));
    double rosslerKickDepthHz = 1800.0 + unit(rng) * 3200.0;
    float rosslerKickEnv = 0.0f;

    // Fase 74 -- envelope de "asfixia" por sidechain: a modulacao do
    // filtro principal ate aqui e so caos/LFO-driven (henonVal/lfoOut[2]
    // acima) e o chute pontual do Rossler -- nada reage a um transiente de
    // PERCUSSAO real. Aqui, cada batida do bumbo fecha o filtro quase
    // instantaneamente (like a real sidechain/ducking compressor, tecnica
    // padrao em techno/EDM) e o deixa reabrir devagar -- um alvo diferente
    // de qualquer coisa que ja modula o filtro, gatilho real de audio em
    // vez de caos sintetico.
    bool asphyxiaEnabled = unit(rng) < 0.25;
    double asphyxiaDepth = 0.5 + unit(rng) * 0.4; // fecha 50-90% do caminho ate zero
    double asphyxiaRecoverySeconds = 0.08 + unit(rng) * 0.25;
    float asphyxiaRecoveryCoeff = static_cast<float>(std::exp(-1.0 / (asphyxiaRecoverySeconds * sampleRate)));
    float asphyxiaEnv = 1.0f; // 1 = filtro na abertura normal, some rumo a (1-depth) no impacto
    int asphyxiaCount = 0;

    // Whether this specific piece leaves behind a plain-text epitaph in
    // its own WAV file (see WavWriter.hpp) -- most do, some don't, same
    // "seed decides" spirit as everything else, even though this one has
    // zero audible consequence either way.
    bool selfEpitaphEnabled = unit(rng) < gameBias(0.7, gameWeightMimicry);

    // Stochastic mute: a real, sample-accurate silence, not a fade --
    // "corte cirurgico," John Cage/Autechre-style disruptive silence
    // rather than the whole-mix thermal fade above. Deliberately rare
    // (long minimum gap between occurrences) and deliberately brief (a
    // few milliseconds), or this reads as the recording being broken
    // instead of the piece tearing itself on purpose. Originally keyed off
    // cuspVal crossing a threshold (the research this was based on
    // suggested a Cusp Map trigger); measured empirically instead of
    // assumed, that map's actual excursion range turned out to depend
    // heavily on initial conditions -- some seeds converge toward a narrow
    // band nowhere near the nominal [-1,1] the class promises, so neither
    // a fixed cutoff nor "above its own recent average" reliably fired at
    // all. Replaced with a plain per-sample probability on its own
    // dedicated RNG (muteRng) -- less "sourced from a specific chaos map,"
    // but the one design that's actually guaranteed to deliver the rare,
    // real silence this is supposed to be, regardless of which seed rolls
    // it on.
    bool stochasticMuteEnabled = unit(rng) < gameBias(0.25, gameWeightIlinx);
    std::mt19937 muteRng(cfg.seed + 30000u);
    std::uniform_real_distribution<double> muteUnit(0.0, 1.0);
    int muteLenMinSamples = static_cast<int>(0.003 * sampleRate);
    int muteLenMaxSamples = static_cast<int>(0.018 * sampleRate);
    size_t muteMinGapSamples = static_cast<size_t>((10.0 + unit(rng) * 15.0) * sampleRate);
    // Expected additional wait past the minimum gap before the per-sample
    // roll below succeeds: roughly 1/perSampleProbability seconds -- tuned
    // for a handful of seconds, so the exact moment still wanders instead
    // of always landing right at the minimum gap boundary.
    double mutePerSampleProbability = 1.0 / (sampleRate * (2.0 + unit(rng) * 4.0));
    float muteSlewCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (0.0015 * sampleRate)));
    int muteRemaining = 0;
    size_t samplesSinceLastMute = 0;
    float muteGateSmoothed = 1.0f;

    // Agon: merit-based conflict, the counterweight to everything else in
    // this file being Alea (a sorteio, used once, applied). Four existing
    // voices (main subtractive voice, Atari Punk Console, Gendyn texture,
    // tuba) each get a seed-derived "power" extreme -- deliberately four
    // mix points that never share a filter/envelope stage with each other
    // (the modal resonator was the first candidate, but it mixes into the
    // main voice *before* that shared filter stage, so scaling it
    // independently there would double up with whatever the main voice's
    // own multiplier already did to the combined signal downstream).
    // Power isn't fixed from the first sample -- it solidifies from a
    // neutral baseline toward that
    // extreme as the piece accumulates lived time (arc.elapsedSeconds()),
    // same biography framing the rest of this project already uses: a
    // conflict early in a piece is close and uncertain, the same conflict
    // late in the same piece is decided with real force. A conflict only
    // triggers when the melody itself has gone stale (low recent pitch
    // variance, tracked as a cheap running EWMA rather than a full
    // history buffer) -- Agon answers predictability, it doesn't compete
    // for its own sake. Whichever voice currently holds the most power
    // wins a temporary, real gain advantage; the others are pulled back.
    // The LookaheadLimiter already downstream is the actual arbiter --
    // it's what stops the "winner" from just clipping outright.
    bool agonEnabled = auditionForce("RASGO_FORCE_AGON", unit(rng) < gameBias(0.3, gameWeightAgon));
    double agonPowerExtreme[4];
    for (int agonK = 0; agonK < 4; ++agonK) agonPowerExtreme[agonK] = 0.15 + unit(rng) * 0.8;
    double agonSolidifyTauSeconds = 20.0 + unit(rng) * 40.0;
    // Calibrated empirically, not guessed: an isolated probe logging the
    // EWMA below across real pieces showed it settles around 50-140
    // (semitones^2) once a melody is actually moving -- register-offset
    // jumps between sections dominate it far more than note-to-note
    // jumpiness does. The original 0.6-2.1 band assumed near-total pitch
    // stasis and was two orders of magnitude too tight to ever trigger.
    double agonStalenessThreshold = 15.0 + unit(rng) * 35.0;
    double agonConflictChance = 0.1 + unit(rng) * 0.3;
    size_t agonMinGapSamples = static_cast<size_t>((15.0 + unit(rng) * 20.0) * sampleRate);
    if (auditionMode) {
        // A forced Agon still needs the melody to read as "stale" before it
        // will actually fire -- relax that condition too in audition mode,
        // otherwise forcing the flag on wouldn't reliably produce a
        // conflict inside a short demo clip.
        agonMinGapSamples = static_cast<size_t>(2.0 * sampleRate);
        agonStalenessThreshold = 1000.0;
    }
    double agonConflictTauSeconds = 2.0 + unit(rng) * 3.0;
    size_t agonSamplesSinceConflict = 0;
    int agonConflictSamplesRemaining = 0;
    int agonConflictWinner = -1;
    int agonRusgaCount = 0;
    float agonConflictEnv = 0.0f;

    // Fase 85 stage 3 -- a voice actually sounds if the current section's
    // orchestration includes it, OR a real cross-mechanism encounter is
    // overriding orchestration entirely: an Agon rusga in progress, or the
    // couple of bars right after Curto-Circuito's rupture (F/G above).
    auto voiceOn = [&](int idx) {
        return drumsEnabled && musicalDirection.mayStart(OrganismStem::Drums)
            && cfg.moduleSelection.choose(
                static_cast<StudioModule>(static_cast<unsigned>(StudioModule::Kick) + idx),
                voiceActive[static_cast<size_t>(idx)]
            || agonConflictSamplesRemaining > 0
            || postGlitchFullKitBars > 0);
    };
    // How much each voice cedes during a conflict isn't a flat winner-take-all
    // split -- a voice whose own solidified power sits close to the winner's
    // barely cedes ground, one far behind cedes closer to the old full 0.5.
    // Filled in whenever a conflict triggers (relative[k] = power[k]/best,
    // so the winner's own slot is always 1.0 and unused -- see the boost
    // branch below instead).
    double agonRelativePower[4] = { 1.0, 1.0, 1.0, 1.0 };
    float agonMainMult = 1.0f, agonAtariMult = 1.0f, agonResonatorMult = 1.0f, agonTubaMult = 1.0f;
    double agonPitchMean = pieceRootMidiNote;
    double agonPitchVar = 0.0;

    // "Elemento estranho" -- deliberately NOT another well-mannered,
    // smoothly-integrated mechanism like everything else in this file.
    // Every other risk mechanism here (Rungler, Rossler, Agon, stochastic
    // mute) is gated by some legible condition -- staleness, a threshold
    // crossed, a chaos map's own state. This one isn't: no staleness
    // check, no narrative motivation, just a rare, unmotivated gap+roll,
    // because genuine foreignness in life doesn't usually announce a
    // reason first. While it's active, the main voice and the Atari
    // texture are compared sample-by-sample -- whichever is louder passes
    // through, phase-modulated by the one that lost -- and that
    // comparison is intentionally left raw and discontinuous, never
    // smoothed into coherence the way Agon's own conflict is. Only the
    // event's entry/exit is smoothed (a fast ~15ms gate), so it arrives
    // and leaves without clicking, not so it eases in gently -- it should
    // still sound like it doesn't quite belong to the rest of the piece.
    bool estranhoEnabled = auditionForce("RASGO_FORCE_ESTRANHO", unit(rng) < gameBias(0.15, gameWeightMimicry));
    double estranhoChance = 0.05 + unit(rng) * 0.15;
    size_t estranhoMinGapSamples = static_cast<size_t>((25.0 + unit(rng) * 45.0) * sampleRate);
    if (auditionMode) estranhoMinGapSamples = static_cast<size_t>(2.0 * sampleRate);
    size_t estranhoSamplesSinceLast = 0;
    int estranhoSamplesRemaining = 0;
    double estranhoDurationSeconds = 0.4 + unit(rng) * 1.4;
    float estranhoGateEnv = 0.0f;
    int estranhoCount = 0;

    // Real Q1.15 fixed-point overflow (see FixedPointOverflow.hpp) --
    // "virgule fixe" territory, a different numeric class than the plain
    // floats everything else in this engine works in. Same rare
    // gap+roll shape as elemento estranho, same smooth-edges/raw-interior
    // envelope discipline; a genuinely different mechanism, not a
    // reskin -- this one wraps a value's sign via integer masking rather
    // than comparing two signals against each other.
    bool fixedOverflowEnabled = auditionForce("RASGO_FORCE_FIXEDOVERFLOW", unit(rng) < 0.15);
    double fixedOverflowChance = 0.05 + unit(rng) * 0.15;
    size_t fixedOverflowMinGapSamples = static_cast<size_t>((20.0 + unit(rng) * 40.0) * sampleRate);
    if (auditionMode) fixedOverflowMinGapSamples = static_cast<size_t>(2.0 * sampleRate);
    size_t fixedOverflowSamplesSinceLast = 0;
    int fixedOverflowSamplesRemaining = 0;
    double fixedOverflowDurationSeconds = 0.3 + unit(rng) * 1.0;
    // 1.3..2.5 -- enough drive to wrap a few times per cycle (audibly
    // "broken") without driving so hard the result dissolves into pure
    // aliased noise with no trace of the original signal left.
    double fixedOverflowDrive = 1.3 + unit(rng) * 1.2;
    float fixedOverflowGateEnv = 0.0f;
    int fixedOverflowCount = 0;
    FixedPointOverflow fixedOverflowProcessor;
    fixedOverflowProcessor.setDrive(fixedOverflowDrive);

    // Real spectral-domain processing (see SpectralGhost.hpp) -- a
    // genuinely different CLASS of processing than everything else in
    // this engine, which is all time-domain. Fed continuously (so its
    // internal analysis buffers stay warm and its ~23ms latency never
    // shows up as a gap), but only mixed into the signal during a rare,
    // sustained window -- longer than elemento estranho's brief burst
    // (seconds, not milliseconds), since a spectral texture needs real
    // time to actually read as a ghostly, unpitched blur rather than a
    // click.
    bool spectralGhostEnabled = auditionForce("RASGO_FORCE_SPECTRALGHOST", identity.spectral.spectralGhost);
    double spectralGhostChance = 0.04 + unit(rng) * 0.1;
    size_t spectralGhostMinGapSamples = static_cast<size_t>((30.0 + unit(rng) * 50.0) * sampleRate);
    if (auditionMode) spectralGhostMinGapSamples = static_cast<size_t>(2.0 * sampleRate);
    size_t spectralGhostSamplesSinceLast = 0;
    int spectralGhostSamplesRemaining = 0;
    double spectralGhostDurationSeconds = 2.0 + unit(rng) * 4.0;
    float spectralGhostGateEnv = 0.0f;
    int spectralGhostCount = 0;
    SpectralGhost spectralGhostProcessor;
    spectralGhostProcessor.seedRng(cfg.seed + 29000u);
    spectralGhostProcessor.setDriftAmount(0.1 + unit(rng) * 0.4);

    // Real sample-rate corruption (see SampleHoldGlitch.hpp) -- decimation,
    // holding stale values for an irregular, chaos-jittered number of
    // samples instead of updating every one. Same rare gap+roll shape as
    // the rest of this "estranho" family; unlike fixed-point overflow
    // (amplitude wraparound) or spectral ghost (frequency domain), this
    // one corrupts the TIME axis specifically -- a genuinely different
    // failure mode, not a re-skin of an existing one.
    bool sampleHoldGlitchEnabled = auditionForce("RASGO_FORCE_SAMPLEHOLD", unit(rng) < 0.15);
    double sampleHoldGlitchChance = 0.05 + unit(rng) * 0.15;
    size_t sampleHoldGlitchMinGapSamples = static_cast<size_t>((20.0 + unit(rng) * 40.0) * sampleRate);
    if (auditionMode) sampleHoldGlitchMinGapSamples = static_cast<size_t>(2.0 * sampleRate);
    size_t sampleHoldGlitchSamplesSinceLast = 0;
    int sampleHoldGlitchSamplesRemaining = 0;
    double sampleHoldGlitchDurationSeconds = 0.3 + unit(rng) * 1.2;
    float sampleHoldGlitchGateEnv = 0.0f;
    int sampleHoldGlitchCount = 0;
    SampleHoldGlitch sampleHoldGlitchProcessor;
    sampleHoldGlitchProcessor.seed(cfg.seed + 30000u);
    // 2..40 samples at 44.1kHz is ~0.05-0.9ms per hold -- audible as a
    // grainy, stuttering artifact riding the signal, not a full dropout.
    sampleHoldGlitchProcessor.setHoldRange(2, 2 + static_cast<int>(unit(rng) * 60.0));

    // Fase 70 -- "Parenteses" (see ParenthesisGlitch.hpp): a real reroute
    // of the master output tap to a captured, stuttering loop for a short
    // window, bar-scale rather than sample-scale (SampleHoldGlitch above)
    // or a long-distance self-quotation (ApocryphalTear) -- the piece's
    // own generative engine keeps computing underneath, unaware anything
    // is happening, so it resumes exactly on schedule when the window
    // closes. Excluded from the coda -- that section already has its own
    // dedicated choreographed ending, not another mechanism competing
    // with it.
    bool parenthesisEnabled = unit(rng) < gameBias(0.2, gameWeightMimicry);
    ParenthesisGlitch parenthesis;
    parenthesis.setSampleRate(sampleRate);
    int parenthesisMinGapBars = 8 + static_cast<int>(unit(rng) * 16.0);
    int parenthesisBarsSinceLast = parenthesisMinGapBars;
    double parenthesisChance = 0.06 + unit(rng) * 0.06;
    int parenthesisCount = 0;
    std::string parenthesisLastMode;
    double parenthesisLastDurationSeconds = 0.0;
    double parenthesisLastLoopSeconds = 0.0;

    if (hocketEnabled || phasingEnabled || polymeterEnabled
        || metricModulationEnabled || sequencerWildTimingEnabled
        || atariPunkGlitchEnabled) {
        performanceActors.add({3, PerformanceGestureFamily::Pattern,
                               AgencyFamily::Polyphony,
                               PerformanceCapability::PatternTiming
                                   | PerformanceCapability::Density
                                   | PerformanceCapability::Transition,
                               true, false, 0.52, 0.40, 0.56});
    }
    if (rasgoFractureEnabled || coherenceCollapseEnabled
        || fixedOverflowEnabled || spectralGhostEnabled
        || sampleHoldGlitchEnabled || parenthesisEnabled
        || curtoCircuitoEnabled || montageCutEnabled || atropeloEnabled) {
        performanceActors.add({4, PerformanceGestureFamily::Rupture,
                               AgencyFamily::Rupture,
                               PerformanceCapability::OnsetShape
                                   | PerformanceCapability::SpectralTrajectory
                                   | PerformanceCapability::FailureRecovery,
                               true, false, 0.44, 0.32, 0.44});
    }
    if (cfg.relationalOrchestraEnabled || cfg.relationalChamberEnabled) {
        performanceActors.add({5, PerformanceGestureFamily::Relation,
                               AgencyFamily::Instrument,
                               PerformanceCapability::Transition
                                   | PerformanceCapability::DynamicTrajectory
                                   | PerformanceCapability::SpatialTrajectory,
                               true, false, 0.48, 0.38, 0.48});
    }

    // Phase 120 -- an enabled module is not automatically an audible agent.
    // Families take turns occupying foreground/interlocutor/background roles;
    // the core yields real room instead of every distinctive source receiving
    // another permanently cautious cap under the same invariant lead.
    // Core-sub is an Instrument desk even without Acid/Tuba/FM/Theremin.
    // Its availability must reach both family eligibility and stem direction;
    // otherwise valid bass events are multiplied by an inactive family's zero.
    const bool bassFallbackEnabled =
        bassBehaviourEnabled && !tubaVoiceEnabled && !acidBasslineEnabled;
    AgencyActiveSet agencyActive{};
    agencyActive[static_cast<std::size_t>(AgencyFamily::Core)] = coreVoiceEnabled;
    agencyActive[static_cast<std::size_t>(AgencyFamily::Texture)] = true;
    agencyActive[static_cast<std::size_t>(AgencyFamily::Instrument)] =
        acidBasslineEnabled || thereminEnabled || fmFluteEnabled
        || atropeloEnabled || tubaVoiceEnabled || galoEnabled
        || footnoteEnabled || bassFallbackEnabled;
    agencyActive[static_cast<std::size_t>(AgencyFamily::Voice)] =
        scatEnabled || choirEnabled || dejaVuEnabled || bluffSignalEnabled;
    agencyActive[static_cast<std::size_t>(AgencyFamily::Polyphony)] =
        harmonaigEnabled || sievePolyphonyEnabled || bloomEnabled
        || hocketEnabled || phasingEnabled;
    agencyActive[static_cast<std::size_t>(AgencyFamily::Rhythm)] =
        drumsEnabled || polymeterEnabled;
    agencyActive[static_cast<std::size_t>(AgencyFamily::Rupture)] =
        rasgoFractureEnabled || coherenceCollapseEnabled
        || fixedOverflowEnabled || spectralGhostEnabled
        || sampleHoldGlitchEnabled || parenthesisEnabled
        || curtoCircuitoEnabled;
    AgencyActiveSet agencyForegroundEligible = agencyActive;
    agencyForegroundEligible[static_cast<std::size_t>(AgencyFamily::Instrument)] =
        acidBasslineEnabled || thereminEnabled || fmFluteEnabled
        || atropeloEnabled || tubaVoiceEnabled || bassFallbackEnabled;
    agencyForegroundEligible[static_cast<std::size_t>(AgencyFamily::Voice)] =
        scatEnabled || choirEnabled || bluffSignalEnabled;
    // Rare gestures retain their full event gain but never reserve an entire
    // section that may pass before the event happens. Their form is an
    // interruption, not a stable orchestral desk.
    agencyForegroundEligible[static_cast<std::size_t>(AgencyFamily::Rupture)] = false;
    MaterialAgency materialAgency(
        cfg.seed, identity, gamePlan, agencyActive,
        agencyForegroundEligible, sampleRate, cfg.materialAgencyEnabled);
    const std::string agencyInitialForeground = materialAgency.foregroundLabel();

    OrganismStemActiveSet audibilityActive{};
    auto setAudibilityActive = [&](OrganismStem stem, bool active) {
        audibilityActive[static_cast<std::size_t>(stem)] = active;
    };
    setAudibilityActive(OrganismStem::Core, coreVoiceEnabled);
    setAudibilityActive(OrganismStem::Texture, true);
    setAudibilityActive(OrganismStem::Theremin, thereminEnabled);
    setAudibilityActive(OrganismStem::FmFlute, fmFluteEnabled);
    setAudibilityActive(OrganismStem::Tuba, tubaVoiceEnabled);
    setAudibilityActive(OrganismStem::Acid, acidBasslineEnabled);
    setAudibilityActive(OrganismStem::BassFallback, bassFallbackEnabled);
    setAudibilityActive(OrganismStem::Galo, galoEnabled);
    setAudibilityActive(OrganismStem::Scat, scatEnabled);
    setAudibilityActive(OrganismStem::Choir, choirEnabled);
    setAudibilityActive(OrganismStem::DejaVu, dejaVuEnabled);
    setAudibilityActive(OrganismStem::Drums, drumsEnabled);
    setAudibilityActive(OrganismStem::Harmonaig, harmonaigEnabled);
    setAudibilityActive(OrganismStem::Sieve, sievePolyphonyEnabled);
    setAudibilityActive(OrganismStem::Bloom, bloomEnabled);
    setAudibilityActive(OrganismStem::Hocket, hocketEnabled || phasingEnabled);
    setAudibilityActive(OrganismStem::Spectral, spectralObjectEnabled);
    setAudibilityActive(OrganismStem::Polymeter, polymeterEnabled);
    setAudibilityActive(OrganismStem::Bluff, bluffSignalEnabled);
    setAudibilityActive(OrganismStem::Footnote, footnoteEnabled);
    setAudibilityActive(OrganismStem::Rupture,
        agencyActive[static_cast<std::size_t>(AgencyFamily::Rupture)]);
    // Residual and output static are accounting desks, not autonomous
    // organisms whose level this stage is allowed to reinterpret.
    setAudibilityActive(OrganismStem::Other, false);
    setAudibilityActive(OrganismStem::ThermalStatic, false);
    OrganismAudibility organismAudibility(
        cfg.seed, cfg.organismAudibilityEnabled, audibilityActive,
        identity, gamePlan, sampleRate);
    musicalDirection.setAvailable(audibilityActive);
    musicalRelations.setAvailable(drumsEnabled, bassBehaviourEnabled, fmFluteEnabled);
    const auto directedKitInterpretation = [&](unsigned voice, double velocity) {
        if (musicalRelations.enabled()) {
            musicalRelations.recordApplied(MusicalBody::Kit);
            return KitInterpretation::interpret(musicalRelations.kitMaterial(voice),
                musicalRelations.kitArticulation(voice), velocity);
        }
        return KitInterpretation::interpret(musicalDirection.plan().material,
            musicalDirection.plan().articulation, velocity);
    };
    const auto directedGain = [&](OrganismStem stem) {
        if (stem == OrganismStem::Core && !coreVoiceEnabled) return 0.0;
        return organismAudibility.gain(stem) * musicalDirection.gain(stem);
    };

    AgencyContributionMeter agencyMeter;
    agencyMeter.setBarObservationEnabled(cfg.musicalDirectionEnabled);
    OrganismStemAuditor organismStemAudit;
    organismStemAudit.prepare(sampleRate, cfg.organismStemAuditEnabled,
                              loFiDelay, 0.6, reverbRRoomSize);
    NarrativeAttentionEngine::ProfileSet narrativeAttentionProfiles{};
    for (std::size_t i = 0; i < narrativeAttentionProfiles.size(); ++i) {
        narrativeAttentionProfiles[i] = baseOrchestralProfile(
            static_cast<OrganismStem>(i), identity);
    }
    const bool narrativeAttentionActive = cfg.narrativeAttentionEnabled
        && cfg.organismStemAuditEnabled;
    NarrativeAttentionEngine narrativeAttention(
        cfg.seed + 131000u, narrativeAttentionActive);
    NarrativeAttentionDecision narrativeAttentionDecision;
    const bool narrativeMixActive = cfg.narrativeMixEnabled
        && narrativeAttentionActive && cfg.organismAudibilityEnabled;
    NarrativeMixDirector narrativeMix(narrativeMixActive);
    NarrativeMixDirective narrativeMixDirective;
    OrchestralSectionMemory orchestralSectionMemory;
    orchestralSectionMemory.beginSection();

    // Phase 121 -- first real intermodular orchestra. It only exists when
    // all three participants can genuinely act: an audible drum engine, a
    // theremin body and the shared harmonic field. No second probability
    // roll: once participants exist, their relation must have a real chance
    // to live. The PieceConfig switch is solely the same-seed contrafactual.
    const bool relationalOrchestraActive = cfg.relationalOrchestraEnabled
        && drumsEnabled && thereminEnabled;
    IntermodularOrchestra relationalOrchestra(
        cfg.seed, identity, gamePlan, sampleRate, relationalOrchestraActive);
    RegionId thereminRelationRegion = 0;
    std::uint32_t rhythmToThereminRelationPassage = 0;
    std::uint32_t thereminToHarmonyRelationPassage = 0;
    std::uint32_t harmonyToRhythmRelationPassage = 0;
    bool relationalThereminPitchTelemetryInitialized = false;
    int relationalThereminPreviousTelemetryMidi = 0;
    int relationalThereminCurrentRepeatedBars = 0;
    int relationalThereminPitchChangeCount = 0;
    int relationalThereminRepeatedPitchCount = 0;
    int relationalThereminLongestRepeatedBars = 0;
    int relationalThereminPitchMin = 0;
    int relationalThereminPitchMax = 0;
    if (relationalOrchestraActive) {
        thereminRelationRegion = topology.addRegion(
            RegionRole::Voice, static_cast<double>(pieceRootMidiNote + 12) / 127.0,
            thereminLead ? 0.86 : 0.56, relationalOrchestra.sensitivity());
        rhythmToThereminRelationPassage = topology.connect(
            rhythmRegion, thereminRelationRegion,
            ModuleDomain::Rhythm, ModuleDomain::Gesture,
            PassageMode::Preserve, 0.82 + 0.34 * gamePlan.freedom,
            relationalOrchestra.sensitivity() * 0.45, false);
        thereminToHarmonyRelationPassage = topology.connect(
            thereminRelationRegion, harmonyRegion,
            ModuleDomain::Pitch, ModuleDomain::Harmony,
            gamePassageMode, 0.76 + 0.36 * gamePlan.risk,
            relationalOrchestra.resistance() * 0.42, false);
        harmonyToRhythmRelationPassage = topology.connect(
            harmonyRegion, rhythmRegion,
            ModuleDomain::Harmony, ModuleDomain::Rhythm,
            PassageMode::Difference, 0.72 + 0.30 * gamePlan.memory,
            0.18 + 0.38 * gamePlan.hiddenInformation, false);
    }
    OrchestraDecision relationalDecision;
    RhythmObservation relationalRhythmObservation;
    int relationalAppliedHarmonyShiftCount = 0;
    double relationalThereminPresenceSmoothed =
        relationalOrchestra.currentThereminPresence();
    const double relationalThereminPresenceSmoothing = 1.0 - std::exp(
        -1.0 / (sampleRate * 0.18));

    const bool relationalChamberActive = cfg.relationalChamberEnabled
        && cfg.organismStemAuditEnabled && fmFluteEnabled && tubaVoiceEnabled;
    const bool relationalChamberAuditReady = cfg.organismStemAuditEnabled;
    RelationalChamber relationalChamber(cfg.seed + 127000u,
                                        relationalChamberActive);
    ChamberDecision relationalChamberDecision;
    int relationalChamberConsequenceCount = 0;
    int relationalChamberCiteCount = 0;
    int relationalChamberResistCount = 0;
    int relationalChamberDeformCount = 0;
    int relationalChamberScarCount = 0;
    int relationalChamberSpatialCrossingCount = 0;
    int relationalChamberLastManifestedBodies = 0;
    std::string relationalChamberLastAction = "none";
    ChamberResponse relationalChamberResponse;

    // Report progress about once per piece-second -- plenty for a UI
    // progress bar/playhead without calling back on every sample. This is
    // a fixed cadence independent of total length; a shorter interval
    // sounds harmless until a seed's walk runs to 200+ seconds and the
    // resulting flood of UI callbacks (each one a repaint) turns what
    // should be a sub-second render into several seconds of dispatch
    // overhead on the message thread.
    size_t progressEvery = std::max<size_t>(1, static_cast<size_t>(sampleRate * 1.0));

    // Real bug the user caught by ear: "morte termica" abrupt death
    // (thermalAbruptDeath, tau 0.15-0.45s) crushes the mix to near-silence
    // within ~1-2s of the coda starting, exactly as intended -- but the
    // coda's own section length is rolled independently (8-20s,
    // ArcTemplates.hpp) and has no idea the mix already went silent, so
    // the render kept going for however many seconds were left on that
    // independent roll -- several real seconds of dead air tacked onto
    // an ending that was supposed to be abrupt, not silent-then-nothing.
    // Fixed by actually ending the render once the abrupt gain has
    // genuinely decayed away, instead of waiting for the coda's unrelated
    // length to run out.
    bool thermalAbruptEndReached = false;

    // The arc uses its own RNG and receives no audio feedback. Its copied
    // walk supplies an exact work count without a second audio render.
    const auto thermalCodaLimit = thermalAbruptDeath
        ? abruptCodaFrameLimit(sampleRate, thermalFadeTauSeconds)
        : std::numeric_limits<std::uint64_t>::max();
    const double narrativeDurationScale = applyDurationTarget(
        arc, cfg.targetDurationSeconds, sampleRate, thermalCodaLimit);
    const auto plannedCompositionFrames = onProgress
        ? arc.remainingRenderFrames(thermalCodaLimit)
        : 0;
    PieceProgress renderProgress{};
    renderProgress.totalFrames = plannedCompositionFrames;
    if (onProgress) onProgress(renderProgress);

    // Slow shared weather for noise-like materials. Presence targets change
    // only at bar boundaries and are eased at audio rate, so a layer can
    // recede or return without becoming a hard gate or a click.
    float noiseClimate = 0.2f;
    float noiseClimateTarget = 0.2f;
    float granularPresence = granularTextureEnabled ? 0.65f : 0.0f;
    float gendynPresence = gendynTextureEnabled ? 0.5f : 0.0f;
    float coloredNoisePresence = coloredNoiseBedEnabled ? 0.45f : 0.0f;
    float granularPresenceTarget = granularPresence;
    float gendynPresenceTarget = gendynPresence;
    float coloredNoisePresenceTarget = coloredNoisePresence;
    std::uint32_t noiseClimateState = cfg.seed ^ 0x9e3779b9u;
    float noiseClimateMin = noiseClimate;
    float noiseClimateMax = noiseClimate;
    double noiseClimateSum = 0.0;
    std::size_t noiseClimateSamples = 0;

    ArcState lastRenderedArcState{};
    bool renderedArcState = false;
    const auto triggerBassDirective = [&](const BassBodyDirective &directive) {
        if (!musicalDirection.mayStart(OrganismStem::BassFallback)) return;
        const int registerDown =
            directive.registerIntent == BassRegisterIntent::Sub ? 36
            : directive.registerIntent == BassRegisterIntent::Low ? 24
            : 12;
        const int targetMidi = std::clamp(
            bassBodyRootMidi + directive.semitoneOffset - registerDown,
            12, 72);
        const double targetHz = musicalRelations.enabled()
            ? MusicalRelations::bassFrequency(midiToFreq(targetMidi)) : midiToFreq(targetMidi);
        const auto& bassControl = musicalRelations.controls(MusicalBody::Bass);
        const double bassColor = musicalRelations.enabled() ? 1.0 + bassControl.material * .65 : 1.0;
        const double bassGateScale = musicalRelations.enabled() ? 1.25 - bassControl.articulation * .65 : 1.0;
        const int gateSamples = std::max(1, static_cast<int>(std::llround(
            directive.gate * static_cast<double>(samplesPerStep) * bassGateScale)));
        if (directive.body != BassBodyKind::None) musicalRelations.recordApplied(MusicalBody::Bass);
        double soundingBassHz = targetHz;
        switch (directive.body) {
            case BassBodyKind::Tuba:
                soundingBassHz = musicalRelations.enabled() && targetHz > 120
                    ? targetHz * .5 : std::clamp(targetHz, 24.0, 120.0);
                tubaResonator.setFundamental(soundingBassHz);
                tubaBreath.setFilterCutoff(std::clamp((360.0 + directive.brightness * 1180.0) * bassColor, 180.0, 2400.0));
                tubaBreath.setAttackRelease(
                    0.025 + directive.attack * 0.18,
                    0.08 + directive.release * 0.52);
                tubaBreath.setGate(!directive.dead);
                bassTubaGateSamples = directive.dead ? 0 : gateSamples;
                bassTubaDirective = directive;
                ++bassTubaEventCount;
                break;
            case BassBodyKind::Acid:
                acidBass.setBaseCutoff(std::clamp((150.0 + directive.brightness * 520.0) * bassColor, 90.0, 1400.0));
                acidBass.setEnvDepth(900.0 + directive.pressure * 3000.0);
                acidBass.setResonance(0.74 + directive.drive * 0.23);
                acidBass.setDecayTimes(
                    std::clamp(0.06 + directive.gate * 0.30, 0.06, 0.42),
                    std::clamp(0.045 + directive.release * 0.20, 0.045, 0.28));
                acidBass.trigger(targetHz, directive.accent, directive.slide);
                bassAcidDirective = directive;
                if (directive.accent) ++acidBasslineAccentCount;
                if (directive.slide) ++acidBasslineSlideCount;
                ++bassAcidEventCount;
                break;
            case BassBodyKind::CoreSub:
                bassPhraseTimbre.noteOn(static_cast<unsigned>(std::max(0, barCounter)),
                    directive.pressure, directive.accent, directive.dead);
                soundingBassHz = std::clamp(targetHz, 28.0, 130.0);
                bassCoreSubFundamental.setFrequency(soundingBassHz);
                bassCoreSubHarmonic.setFrequency(std::clamp(targetHz * 2.0, 56.0, 260.0));
                bassCoreSubEnvelope.setADSR(
                    0.003 + directive.attack * 0.055,
                    0.04 + directive.pressure * 0.10,
                    directive.dead ? 0.05 : 0.42 + directive.presence * 0.38,
                    0.035 + directive.release * 0.30);
                bassCoreSubEnvelope.noteOn();
                bassCoreSubGateSamples = directive.dead
                    ? std::max(1, gateSamples / 5) : gateSamples;
                bassCoreSubDirective = directive;
                ++bassCoreSubEventCount;
                break;
            case BassBodyKind::None:
            case BassBodyKind::Count:
                break;
        }
        if (directive.body != BassBodyKind::None
            && !(directive.body == BassBodyKind::Tuba && directive.dead))
            bassExecutionFeedback.observeTrigger(directive.pressure,
                69.0 + 12.0 * std::log2(soundingBassHz / 440.0));
        ++bassEventCount;
    };

    for (size_t i = 0; !arc.finished() && !thermalAbruptEndReached; ++i) {
        ArcState state = arc.tick();
        lastRenderedArcState = state;
        renderedArcState = true;
        const float percussionSectionHoldCandidate = percussionSectionHoldSource.tick();
        materialAgency.tick();
        organismAudibility.tick();
        musicalDirection.tick();
        musicalRelations.beginFrame();
        organismStemAudit.beginFrame();

        // Captured before either boundary can update samplesPerStep/
        // samplesPerBar below, so both checks this iteration reflect the
        // schedule as it stood when the iteration started.
        bool barBoundary = (i >= nextBarBoundary);
        if (barBoundary) musicalRelations.beginBar(barCounter);
        if (barBoundary && cfg.musicalDirectionEnabled) {
            musicalDirection.beginBar(barCounter, static_cast<int>(arc.history().size()),
                arc.inPeak(), arc.inCoda(), state.density, agencyMeter.takeBarObservation());
        }
        bool stepBoundary = (i >= nextStepBoundary);
        bool phaseStepBoundary = phasingEnabled && (i >= phaseNextStepBoundary);

        if (barBoundary) {
            // "Modo selvagem": real hardware entropy (std::random_device,
            // not the seed-derived rng stream) reseeds the SHARED rng
            // every bar -- not a new code path in any individual
            // mechanism, since every existing unit(rng) consumer already
            // safely handles whatever value it receives, by construction.
            // This is what actually delivers "characteristics that escape
            // the engine's own predictability": from this point on, every
            // remaining decision in the piece -- melody, rhythm, which
            // rare mechanisms fire -- is genuinely touched by real,
            // non-reproducible entropy, not just the one disclosed
            // foreign-signal exception (fase 51) read once at the start.
            if (cfg.wildMode) {
                std::random_device wildRd;
                rng.seed(static_cast<unsigned>(wildRd()) ^ rng());
                ++wildReseedCount;
            }

            // Noise climate: a shared, slow field gives all noise-like
            // materials a common dramaturgy without forcing them to move in
            // lockstep. Density biases the weather but never determines it.
            noiseClimateState ^= noiseClimateState << 13U;
            noiseClimateState ^= noiseClimateState >> 17U;
            noiseClimateState ^= noiseClimateState << 5U;
            const float climateRoll = static_cast<float>(noiseClimateState)
                * (1.0f / 2147483648.0f) - 1.0f;
            noiseClimateTarget = std::clamp(
                0.18f + climateRoll * 0.22f
                    + static_cast<float>((0.58 - state.density) * 0.22),
                0.0f, 1.0f);
            noiseClimate += (noiseClimateTarget - noiseClimate) * 0.38f;
            granularPresenceTarget = granularTextureEnabled
                ? std::clamp(0.28f + noiseClimate * 0.90f, 0.0f, 1.0f) : 0.0f;
            gendynPresenceTarget = gendynTextureEnabled
                ? std::clamp(0.18f + noiseClimate * 0.95f, 0.0f, 1.0f) : 0.0f;
            coloredNoisePresenceTarget = coloredNoiseBedEnabled
                ? std::clamp(0.20f + noiseClimate * 0.85f, 0.0f, 1.0f) : 0.0f;
            if (cfg.expressiveActuationEnabled
                && !performanceActors.select(
                    cfg.seed, static_cast<std::uint32_t>(barCounter),
                    expressiveTextureRequest).empty()) {
                const TextureGesturePlan textureGesture =
                    PerformanceGesturePlanner::planTexture(
                        PerformerState{}, state.density, noiseClimateTarget,
                        noiseClimate, state.resonance);
                const float emergence = static_cast<float>(
                    0.84 + textureGesture.emergence * 0.24);
                const float spectral = static_cast<float>(
                    0.86 + textureGesture.spectralBrightness * 0.22);
                granularPresenceTarget = std::clamp(
                    granularPresenceTarget * emergence, 0.0f, 1.0f);
                gendynPresenceTarget = std::clamp(
                    gendynPresenceTarget * spectral, 0.0f, 1.0f);
                coloredNoisePresenceTarget = std::clamp(
                    coloredNoisePresenceTarget
                        * static_cast<float>(0.88 + textureGesture.mass * 0.18),
                    0.0f, 1.0f);
            }

            // Close the previous bar's ACTUAL drum-bus observation. The
            // orchestra never infers life from drumsEnabled: only signal that
            // survived voice synthesis, per-hit dynamics and agency gain can
            // become a gesture another participant hears.
            relationalRhythmObservation = relationalOrchestra.finishRhythmBar(
                rasgarGesture.meanPulseDeviation());

            // Phase 119 -- the gesture reads the current piece rather than
            // firing from an unrelated Bernoulli roll. Density, recent
            // output energy, pattern persistence and a real section crossing
            // accumulate against its resistance. A tear splits the temporal
            // region, recreates the hihat page and sends rhythm through the
            // harmony and space passages. The resulting values are consumed
            // below by the sequencer clock, modulation and effects.
            rasgarHarmonicShiftThisBar = false;
            const bool rasgarSectionBoundary = !lastModulationSectionName.empty()
                && state.currentSectionName != lastModulationSectionName;
            if (rasgarSectionBoundary) {
                percussionSectionHold = percussionSectionHoldCandidate;
                const OrganismSectionSnapshot previousStemSnapshot =
                    organismStemAudit.sectionSnapshot();
                const auto previousSection = [&]() {
                    OrchestralSectionReport report;
                    orchestralSectionMemory.beginSection();
                    for (int stem = 0; stem < static_cast<int>(OrganismStem::Count); ++stem) {
                        const auto organism = static_cast<OrganismStem>(stem);
                        const auto &metrics = previousStemSnapshot[
                            static_cast<std::size_t>(stem)];
                        const auto profile = baseOrchestralProfile(organism, identity);
                        const bool active = metrics.audibleRatio > 0.004;
                        orchestralSectionMemory.observe(organism,
                            {metrics.rms, metrics.lowBandRms, metrics.midBandRms,
                             metrics.highBandRms,
                             metrics.audibleRatio, profile.registerBand == RegisterBand::Low ? 43.0
                                 : profile.registerBand == RegisterBand::High ? 78.0 : 60.0,
                             active});
                    }
                    return orchestralSectionMemory.finishSection();
                }();
                ++orchestralMemorySectionCount;
                orchestralHighConflictCount += previousSection.highConflictCount;
                if (previousSection.repeatedLeader) ++orchestralRepeatedLeaderCount;
                orchestralLastLeader = previousSection.strongest;
                // The bed can change spectral identity at a genuine section
                // boundary. It is intentionally occasional and seed-derived:
                // this creates weather, not a sample-rate modulation or a
                // generic brightening rule.
                if (coloredNoiseBedEnabled
                    && identityUnit(cfg.seed,
                        1880 + static_cast<std::size_t>(orchestralMemorySectionCount) * 17U)
                        < 0.30) {
                    const int current = static_cast<int>(activeNoiseColor);
                    int next = static_cast<int>(identityUnit(
                        cfg.seed, 1920 + static_cast<std::size_t>(orchestralMemorySectionCount) * 19U)
                        * 4.0);
                    next = std::clamp(next, 0, 3);
                    if (next == current) next = (next + 1) % 4;
                    activeNoiseColor = static_cast<ColoredNoise::Color>(next);
                    colorNoise.setColor(activeNoiseColor);
                    ++noiseColorChangeCount;
                }
                materialAgency.enterSection(agencyMeter.takeSectionObservation());
                organismAudibility.enterSection(state.density, materialAgency,
                                                previousSection);
                orchestralRoleResponseCount =
                    organismAudibility.orchestralRoleResponseCount();

                // The chamber only listens at a genuine section crossing and
                // only to measured prior-section bodies. Its response is
                // carried into the next section as local articulation and
                // breath pressure, never as a global gain matrix.
                if (relationalChamberActive) {
                    const auto &flute = previousStemSnapshot[
                        static_cast<std::size_t>(OrganismStem::FmFlute)];
                    const auto &tuba = previousStemSnapshot[
                        static_cast<std::size_t>(OrganismStem::Tuba)];
                    const auto &voice = previousStemSnapshot[
                        static_cast<std::size_t>(OrganismStem::Core)];
                    ChamberObservation fluteObservation{
                        flute.audibleRatio > 0.004, static_cast<double>(fmFluteRootForNote),
                        std::clamp(flute.rms * 8.0, 0.0, 1.0),
                        std::clamp(flute.peak * 4.0, 0.0, 1.0),
                        std::clamp(1.0 - flute.audibleRatio, 0.0, 1.0), 0};
                    ChamberObservation tubaObservation{
                        tuba.audibleRatio > 0.004,
                        static_cast<double>(sequencer.rootMidiNote() - 12 * tubaOctaveDown),
                        std::clamp(tuba.rms * 8.0, 0.0, 1.0),
                        std::clamp(tuba.peak * 4.0, 0.0, 1.0),
                        std::clamp(1.0 - tuba.audibleRatio, 0.0, 1.0), 0};
                    ChamberObservation voiceObservation{
                        voice.audibleRatio > 0.004, static_cast<double>(currentMidiNote),
                        std::clamp(voice.rms * 8.0, 0.0, 1.0),
                        std::clamp(voice.peak * 4.0, 0.0, 1.0),
                        std::clamp(1.0 - voice.audibleRatio, 0.0, 1.0), 0};
                    // A chamber does not inspect an abstract global archive:
                    // it reads the last memory page through its own typed
                    // reader. Re-reading is allowed only after memory has
                    // accumulated, preserving provenance and fatigue.
                    const auto archive = moduleArchive.readLatest(
                        harmonyRegion, ModuleDomain::Memory, barCounter,
                        relationalChamber.memory() > 0.6);
                    ChamberObservation archiveObservation{
                        archive.has_value(), 0.0,
                        archive.has_value() ? std::clamp(std::fabs(archive->value), 0.0, 1.0) : 0.0,
                        archive.has_value() ? std::clamp(std::fabs(archive->auxiliary), 0.0, 1.0) : 0.0,
                        archive.has_value() ? 0.35 : 0.0,
                        archive.has_value() ? archive->id : 0};
                    relationalChamberDecision = relationalChamber.evaluate(
                        fluteObservation, tubaObservation, voiceObservation,
                        archiveObservation, barCounter);
                    relationalChamberLastManifestedBodies =
                        relationalChamberDecision.manifestedBodies;
                    switch (relationalChamberDecision.action) {
                    case ChamberAction::Cite:
                        ++relationalChamberCiteCount;
                        break;
                    case ChamberAction::Resist:
                        ++relationalChamberResistCount;
                        break;
                    case ChamberAction::Deform:
                        ++relationalChamberDeformCount;
                        break;
                    case ChamberAction::Scar:
                        ++relationalChamberScarCount;
                        break;
                    case ChamberAction::None:
                        break;
                    }
                    if (relationalChamberDecision.consequence) {
                        ++relationalChamberConsequenceCount;
                        relationalChamberResponse = ChamberResponse::fromDecision(
                            relationalChamberDecision);
                    } else {
                        relationalChamberResponse = ChamberResponse{};
                    }
                    relationalChamberLastAction = chamberActionName(
                        relationalChamberDecision.action);

                    if (relationalChamberDecision.consequence) {
                        const auto archiveAction =
                            relationalChamberDecision.action == ChamberAction::Resist
                            || relationalChamberDecision.action == ChamberAction::Scar
                            ? ArchiveAction::Scar : ArchiveAction::Reinterpret;
                        if (relationalChamberDecision.sourcePage != 0) {
                            moduleArchive.reinterpret(
                                harmonyRegion, relationalChamberDecision.sourcePage,
                                ModuleDomain::Memory,
                                relationalChamberDecision.pressure,
                                relationalChamberDecision.interval,
                                barCounter, archiveAction);
                        } else {
                            moduleArchive.publish(
                                rhythmRegion, ModuleDomain::Memory,
                                relationalChamberDecision.pressure,
                                relationalChamberDecision.interval,
                                barCounter);
                        }

                        // A typed spatial crossing gives the encounter a
                        // delayed room consequence. It is section/bar-rate,
                        // bounded and smoothed downstream by the existing
                        // delay/reverb lag processors.
                        if (harmonyToSpacePassage != 0) {
                            const TraversalResult spatialArrival = topology.traverse(
                                harmonyToSpacePassage, harmonyRegion,
                                relationalChamberResponse.spatialPolarity
                                    * relationalChamberDecision.pressure,
                                barCounter);
                            if (spatialArrival.crossed) {
                                topologySpaceBias = 0.76 * topologySpaceBias
                                    + 0.24 * spatialArrival.output;
                                ++relationalChamberSpatialCrossingCount;
                            }
                        }
                    }
                }

                if (narrativeAttentionActive) {
                    OrganismStem consequenceTarget = OrganismStem::Other;
                    switch (relationalChamberDecision.target) {
                        case ChamberBody::Flute:
                            consequenceTarget = OrganismStem::FmFlute;
                            break;
                        case ChamberBody::Tuba:
                            consequenceTarget = OrganismStem::Tuba;
                            break;
                        case ChamberBody::Voice:
                            consequenceTarget = OrganismStem::Core;
                            break;
                        case ChamberBody::Archive:
                            consequenceTarget = OrganismStem::Other;
                            break;
                    }
                    narrativeAttentionDecision = narrativeAttention.observe(
                        previousStemSnapshot, narrativeAttentionProfiles,
                        {state.density, state.resonance,
                         std::max(gamePlan.memory, relationalChamber.memory()),
                         relationalChamberDecision.isCausallyInfluential(),
                         consequenceTarget});
                    if (narrativeMixActive) {
                        narrativeMixDirective = narrativeMix.decide(
                            narrativeAttentionDecision, previousStemSnapshot,
                            narrativeAttentionProfiles);
                        organismAudibility.applyNarrativeDirective(
                            narrativeMixDirective);
                    }
                }

                // Reset only after every section-boundary observer has read the
                // same immutable evidence from the section that just ended.
                organismStemAudit.beginSection();
            }
            rasgarFrame = rasgarGesture.advanceBar(
                {state.density,
                 std::clamp(outputFeedbackEnergy * 4.0, 0.0, 1.0),
                 sequencer.lockAmount(),
                 rasgarSectionBoundary},
                barCounter);
            if (rasgarFrame.tearTriggered) {
                // The rupture does not ask a closed generator for another
                // unrelated roll. Harmony opens the rhythm archive, reads or
                // rereads a page, and rhythm recreates that trace as a new
                // hihat density. Provenance and every interpretive action
                // remain queryable in the archive.
                const bool isRereading = rasgarGesture.tearCount() > 1;
                const auto archivedRhythm = moduleArchive.readLatest(
                    harmonyRegion, ModuleDomain::Rhythm, barCounter,
                    isRereading);
                const int minimumHits = 3;
                const int maximumHits = std::max(4, currentBarLength - 2);
                int recreatedHits = minimumHits;
                if (archivedRhythm) {
                    const double archivedShape = std::fmod(
                        std::fabs(archivedRhythm->value)
                            + 0.37 * std::fabs(archivedRhythm->auxiliary),
                        1.0);
                    recreatedHits = minimumHits + static_cast<int>(std::lround(
                        archivedShape * static_cast<double>(maximumHits - minimumHits)));
                }
                recreatedHits = std::clamp(recreatedHits, minimumHits, maximumHits);
                generatedEuclidHihat = generateEuclideanPattern(
                    rng, recreatedHits, recreatedHits, currentBarLength);
                if (archivedRhythm) {
                    moduleArchive.reinterpret(
                        rhythmRegion, archivedRhythm->id, ModuleDomain::Rhythm,
                        static_cast<double>(recreatedHits)
                            / static_cast<double>(std::max(1, currentBarLength)),
                        static_cast<double>(generatedEuclidHihat.rotation),
                        barCounter, ArchiveAction::Recreate);
                }
                rasgarHarmonicShiftThisBar = true;
            }
            if (rhythmToHarmonyPassage != 0) {
                const double traversalMaterial = state.density
                    * (0.18 + 0.82 * rasgarFrame.instability)
                    + rasgarFrame.strain * 0.16;
                TraversalResult harmonicReading = topology.traverse(
                    rhythmToHarmonyPassage, rhythmRegion,
                    traversalMaterial, barCounter);
                if (harmonicReading.crossed) {
                    topologyHarmonyBias = 0.72 * topologyHarmonyBias
                                        + 0.28 * harmonicReading.output;
                }
            }
            if (harmonyToSpacePassage != 0) {
                TraversalResult spaceReading = topology.traverse(
                    harmonyToSpacePassage, harmonyRegion,
                    topologyHarmonyBias, barCounter);
                if (spaceReading.crossed) {
                    topologySpaceBias = 0.76 * topologySpaceBias
                                      + 0.24 * spaceReading.output;
                }
                if (rasgarFrame.tearTriggered) {
                    FeedbackResult spatialFeedback = topology.feedback(
                        harmonyToSpacePassage, harmonyRegion,
                        topologyHarmonyBias,
                        -0.25 - 0.55 * gamePlan.risk, barCounter);
                    if (spatialFeedback.returning.crossed) {
                        topologyHarmonyBias = 0.6 * topologyHarmonyBias
                                            + 0.4 * spatialFeedback.returning.output;
                    }
                }
            }
            // Atropelo: a real per-bar chance of a fresh event (competing
            // with, or interrupting, whatever's currently speaking) --
            // register drawn near the melody's own current pitch, an
            // octave or two down into a voice-like fundamental range.
            // Nesting (bounded at 3 levels, the same recursive
            // self-application principle Bloom, fase 79, already
            // validated): a fresh phrase has a real chance of
            // immediately spawning its own smaller nested phrase, itself
            // eligible to nest again up to the depth limit -- never an
            // unbounded regress, always eventually cut off rather than
            // resolved.
            if (atropeloEnabled && unit(rng) < atropeloFireChancePerBar) {
                double registerHz = midiToFreq(currentMidiNote) * (unit(rng) < 0.5 ? 0.5 : 0.25);
                int syllables = 3 + static_cast<int>(unit(rng) * 7.0);
                atropelo.triggerEvent(registerHz, syllables, 0.45);
                double nestChance = 0.45;
                for (int depth = 0; depth < 3 && unit(rng) < nestChance; ++depth) {
                    registerHz *= 1.15 + unit(rng) * 0.3;
                    syllables = std::max(2, syllables - 2);
                    atropelo.triggerEvent(registerHz, syllables, 0.2);
                    ++atropeloNestingCount;
                    nestChance *= 0.5; // each further level of regress is genuinely less likely, not equally probable forever
                }
            }
            // KS Synthesis 4006 Randomizer Core's Probability idea, wired
            // to the arc's own density: busier sections bias the melody's
            // octave jitter upward, sparser ones bias it downward, instead
            // of a flat uniform choice. See StepSequencer::regenerate.
            double octaveBias = (state.density - 0.5) * 2.0;
            // Advance to this bar's chord before regenerating its notes, so
            // the strong-beat weighting above (when both harmonicWeight and
            // chordProgression are on for this piece) targets the chord
            // that's actually current for the bar being drawn, not last
            // bar's.
            sequencer.advanceChord();

            // Phase 121 -- rhythm is heard by theremin, whose pitch proposal
            // is then interpreted by harmony. The return changes the NEXT
            // rhythmic behaviour. No participant is forced into unison: the
            // theremin target is bar-scale and independent of currentMidiNote.
            if (relationalOrchestraActive) {
                double transportedPressure = 0.0;
                if (relationalRhythmObservation.manifested
                    && rhythmToThereminRelationPassage != 0) {
                    TraversalResult rhythmArrival = topology.traverse(
                        rhythmToThereminRelationPassage, rhythmRegion,
                        relationalRhythmObservation.pressure, barCounter);
                    if (rhythmArrival.crossed)
                        transportedPressure = std::fabs(rhythmArrival.output);
                }

                HarmonyObservation harmonyObservation;
                harmonyObservation.rootMidiNote = sequencer.rootMidiNote();
                harmonyObservation.chordOffsets = sequencer.currentChordOffsets();
                harmonyObservation.tension = std::clamp(
                    std::fabs(topologyHarmonyBias), 0.0, 1.0);
                relationalDecision = relationalOrchestra.interpretBar(
                    relationalRhythmObservation, harmonyObservation,
                    transportedPressure, barCounter);

                if (relationalDecision.thereminResponded) {
                    const int targetMidi = static_cast<int>(std::lround(
                        relationalDecision.thereminTargetMidi));
                    if (!relationalThereminPitchTelemetryInitialized) {
                        relationalThereminPitchTelemetryInitialized = true;
                        relationalThereminCurrentRepeatedBars = 1;
                        relationalThereminPitchMin = relationalThereminPitchMax = targetMidi;
                    } else if (targetMidi == relationalThereminPreviousTelemetryMidi) {
                        ++relationalThereminCurrentRepeatedBars;
                        ++relationalThereminRepeatedPitchCount;
                    } else {
                        ++relationalThereminPitchChangeCount;
                        relationalThereminCurrentRepeatedBars = 1;
                    }
                    relationalThereminLongestRepeatedBars = std::max(
                        relationalThereminLongestRepeatedBars,
                        relationalThereminCurrentRepeatedBars);
                    relationalThereminPitchMin = std::min(
                        relationalThereminPitchMin, targetMidi);
                    relationalThereminPitchMax = std::max(
                        relationalThereminPitchMax, targetMidi);
                    relationalThereminPreviousTelemetryMidi = targetMidi;
                }

                if (relationalDecision.thereminResponded
                    && thereminToHarmonyRelationPassage != 0) {
                    const double pitchProposal =
                        (relationalDecision.thereminTargetMidi
                         - static_cast<double>(harmonyObservation.rootMidiNote)) / 12.0;
                    TraversalResult harmonyArrival = topology.traverse(
                        thereminToHarmonyRelationPassage, thereminRelationRegion,
                        pitchProposal, barCounter);
                    if (!harmonyArrival.crossed) {
                        relationalDecision.harmonyResponded = false;
                        relationalDecision.harmonyShiftSemitones = 0;
                        relationalDecision.consequence = false;
                    }
                }

                if (relationalDecision.harmonyResponded
                    && harmonyToRhythmRelationPassage != 0) {
                    const double returnGesture =
                        (relationalDecision.rhythmVelocityScale - 1.0)
                        + static_cast<double>(relationalDecision.rhythmDelaySamples)
                          / (sampleRate * 0.04);
                    TraversalResult rhythmReturn = topology.traverse(
                        harmonyToRhythmRelationPassage, harmonyRegion,
                        returnGesture, barCounter);
                    if (rhythmReturn.crossed) {
                        relationalDecision.rhythmVelocityScale = std::clamp(
                            relationalDecision.rhythmVelocityScale
                            + rhythmReturn.output * 0.12, 0.65, 1.38);
                    }
                }
                relationalRhythmVelocityScale = relationalDecision.rhythmVelocityScale;
                relationalRhythmDelaySamples = relationalDecision.rhythmDelaySamples;

                if (relationalDecision.harmonyShiftSemitones != 0) {
                    const int oldRoot = sequencer.rootMidiNote();
                    const int newRoot = std::clamp(
                        oldRoot + relationalDecision.harmonyShiftSemitones, 36, 84);
                    if (newRoot != oldRoot) {
                        if (microtonalEnabled)
                            microtonalTuning.accumulateModulationDrift(oldRoot, newRoot);
                        sequencer.modulateTo(newRoot, sequencer.scale());
                        ++relationalAppliedHarmonyShiftCount;
                    }
                }
            }
            // Fase 107 -- sequencer wild timing (task #30): the harmonic
            // rhythm above still advances one chord per bar, steady and
            // predictable -- what regenerate() gets fed is captured here
            // regardless, but WHEN it actually fires is what changes.
            const double composedDensity = gamePlan.composeDensity(state.density, barCounter);
            sequencerRegenDensity = composedDensity;
            sequencerRegenOctaveBias = octaveBias;
            if (!sequencerWildTimingEnabled) {
                // Fase 117 -- enquanto uma reorganizacao forcada por
                // colapso de coerencia esta em andamento, ela substitui a
                // regeneracao normal desta barra por uma versao real e
                // extrema (densidade maxima, registro puxado pro extremo
                // oposto a cada barra) -- a "reorganizacao" audivel e
                // concreta da formula do proprio usuario. So se aplica
                // quando o timing selvagem da fase 107 nao ja possui o
                // proprio agendamento de regenerate() (interacao real,
                // nao um bug -- nesse caso so o estalo audivel do colapso
                // ainda soa).
                if (coherenceCollapseEnabled && coherenceReorgBarsRemaining > 0) {
                    double extremeOctaveBias = (unit(rng) < 0.5) ? -1.0 : 1.0;
                    sequencer.regenerate(1.0, extremeOctaveBias);
                    --coherenceReorgBarsRemaining;
                } else {
                    sequencer.regenerate(composedDensity, octaveBias);
                }
            }
            sequencer.setRegisterOffset(state.registerOffset);

            if (rasgarHarmonicShiftThisBar) {
                const int oldRoot = sequencer.rootMidiNote();
                int displacement = static_cast<int>(std::round(
                    std::clamp(topologyHarmonyBias, -1.0, 1.0) * 7.0));
                if (displacement == 0) {
                    displacement = (identityBits(cfg.seed,
                        240 + static_cast<std::uint64_t>(rasgarGesture.tearCount())) & 1ULL)
                        == 0ULL ? -1 : 1;
                }
                const int newRoot = std::clamp(oldRoot + displacement, 36, 84);
                if (microtonalEnabled)
                    microtonalTuning.accumulateModulationDrift(oldRoot, newRoot);
                sequencer.modulateTo(newRoot, sequencer.scale());
                ++topologyHarmonicShiftCount;
            }

            // Fase 117 -- verificacao real de colapso por coerencia, uma
            // vez por barra (mesma cadencia da regeneracao do sequencer
            // acima). Quando dispara: um estalo audivel real (ruido com
            // envelope de ataque quase instantaneo) mais a reorganizacao
            // forcada por algumas barras, armada acima.
            if (coherenceCollapseEnabled) {
                if (coherenceCollapse.checkBarBoundary()) {
                    coherenceCrackEnv.noteOn();
                    coherenceReorgBarsRemaining = 2 + static_cast<int>(unit(rng) * 3.0);
                    ++coherenceCollapseAudibleCount;
                }
            }

            // Fase 115 -- oportunidade real de sinal de blefe, uma vez
            // por compasso, so quando nao ha um sinal ja pendente de
            // resolucao (um de cada vez -- a expectativa de um precisa se
            // resolver antes do proximo comecar, ou a "informacao
            // incompleta" perderia sentido, dois sinais simultaneos
            // disputando qual promessa se refere a qual consequencia).
            if (bluffSignalEnabled && !bluffPendingActive) {
                if (bluffSignal.maybeSignal(bluffSignalChancePerBar)) {
                    bluffRespondentGain = static_cast<float>(bluffSignal.responseGain());
                    bluffSignalEnv.noteOn();
                    bluffRespondentEnv.noteOn();
                    bluffPendingActive = true;
                    bluffResolveCountdownSamples = static_cast<size_t>(bluffResolveDelayBars) * samplesPerBar;
                }
            }

            // Fase 77 -- Harmonaig: re-voice the chord pad for whatever
            // chord the harmony just advanced to. Root/third/fifth/seventh
            // come from the sequencer's own currentChordOffsets() -- a real
            // four-note chord, matching the real Instruo Harmonaig's own
            // root/3rd/5th/7th voicing (see CREDITS.md), not a doubled
            // triad. Rotated so the inversion-chosen tone sits in the
            // bass register; each successive voice gets its own climbing
            // register slot, wider apart for an "open" seed-chosen spread
            // than a "tight" one. Retriggering noteOn() on an already-
            // sustaining envelope restarts its attack -- a fresh swell
            // into the new chord, not a hard cut.
            if (harmonaigEnabled) {
                auto chordOffsets = sequencer.currentChordOffsets();
                std::array<int, 4> rotated{};
                for (int v = 0; v < 4; ++v) {
                    rotated[static_cast<size_t>(v)] =
                        chordOffsets[static_cast<size_t>((harmonaigInversion + v) % 4)];
                }
                int baseMidi = sequencer.rootMidiNote();
                int openLift = (harmonaigSpreadOctaves >= 2) ? 12 : 0;
                harmonaigOsc[0].setFrequency(midiToFreq(baseMidi + rotated[0] - 12));
                harmonaigOsc[1].setFrequency(midiToFreq(baseMidi + rotated[1]));
                harmonaigOsc[2].setFrequency(midiToFreq(baseMidi + rotated[2] + openLift));
                harmonaigOsc[3].setFrequency(midiToFreq(baseMidi + rotated[3] + openLift + 12));
                for (auto &voiceEnv : harmonaigEnv) voiceEnv.noteOn();
                ++harmonaigChordCount;
            }

            // Fase 99 -- Coral: same real chord-change trigger point as
            // Harmonaig just above, but only a fraction of the time (the
            // choir joins in, doesn't sing every single chord).
            if (choirEnabled && unit(rng) < choirPhraseChance) {
                auto choirChordOffsets = sequencer.currentChordOffsets();
                int choirVowel = static_cast<int>(unit(rng) * 5.0) % 5;
                double choirDuration = 0.5 + unit(rng) * 0.6;
                choir.triggerPhrase(sequencer.rootMidiNote(), choirChordOffsets, choirVowel, choirDuration);
                ++choirPhraseCount;
            }

            // Fase 70: once per bar, a rare roll (with a minimum-bar gap
            // already enforced) to open a parenthesis -- excluded from the
            // coda, which has its own dedicated ending gestures.
            if (parenthesisEnabled && !arc.inCoda()) {
                ++parenthesisBarsSinceLast;
                if (!parenthesis.isActive() && parenthesisBarsSinceLast >= parenthesisMinGapBars
                    && unit(rng) < parenthesisChance) {
                    // The fragment length follows the current pulse rather
                    // than an arbitrary 50--200 ms range: sixteenth,
                    // triplet and eighth-note-sized windows remain
                    // recognizable as musical subdivisions while still
                    // sounding like a rupture. The event itself spans a
                    // seed-varied 1--2 seconds, so it does not become a
                    // fixed one-bar tic.
                    const double beatSeconds = 60.0 / std::max(currentBpm, 1.0);
                    const std::array<double, 5> loopSubdivisions{
                        0.125, 1.0 / 6.0, 0.25, 1.0 / 3.0, 0.5};
                    double loopSeconds = beatSeconds
                        * loopSubdivisions[static_cast<std::size_t>(unit(rng)
                            * loopSubdivisions.size()) % loopSubdivisions.size()];
                    loopSeconds = std::clamp(loopSeconds, 0.045, 0.24);
                    const std::array<double, 4> durationBeats{2.0, 3.0, 4.0, 6.0};
                    double durationSeconds = beatSeconds
                        * durationBeats[static_cast<std::size_t>(unit(rng)
                            * durationBeats.size()) % durationBeats.size()];
                    durationSeconds = std::clamp(durationSeconds, 0.9, 2.0);
                    double fadeSeconds = 0.008 + unit(rng) * 0.012;
                    const auto modeRoll = unit(rng);
                    const auto mode = modeRoll < 0.45
                        ? ParenthesisGlitch::PlaybackMode::Forward
                        : modeRoll < 0.75
                            ? ParenthesisGlitch::PlaybackMode::PingPong
                            : ParenthesisGlitch::PlaybackMode::Reverse;
                    // A subset of events keeps the exact stutter; the rest
                    // mutate the cycle phase slightly so a short parenthesis
                    // behaves like a living fragment rather than a rigid
                    // machine loop. Ping-pong remains deliberately exact.
                    const bool cycleVariation =
                        mode != ParenthesisGlitch::PlaybackMode::PingPong
                        && unit(rng) < 0.58;
                    const int loopSamples = std::max(1, static_cast<int>(loopSeconds * sampleRate));
                    const std::array<double, 6> variationStrideFractions{
                        0.17, 0.23, 0.31, 0.41, 0.53, 0.67};
                    const double strideFraction = variationStrideFractions[
                        static_cast<std::size_t>(unit(rng) * variationStrideFractions.size())
                        % variationStrideFractions.size()];
                    parenthesis.setCycleVariation(
                        cycleVariation,
                        static_cast<int>(unit(rng) * loopSamples),
                        std::max(1, static_cast<int>(loopSamples * strideFraction)));
                    parenthesis.open(durationSeconds, loopSeconds, fadeSeconds, mode);
                    parenthesisLastMode = ParenthesisGlitch::playbackModeName(mode);
                    parenthesisLastDurationSeconds = durationSeconds;
                    parenthesisLastLoopSeconds = loopSeconds;
                    parenthesisBarsSinceLast = 0;
                    ++parenthesisCount;
                    observeExpressiveEvent(
                        ExpressiveEventSource::Structural,
                        ExpressiveScale::Form,
                        i,
                        -1,
                        1.0,
                        durationSeconds,
                        state.density,
                        state.resonance,
                        noiseClimate,
                        expressiveBit(ExpressiveDomain::Structure)
                            | expressiveBit(ExpressiveDomain::Timing),
                        expressiveBit(ExpressiveCause::Glitch),
                        0.0,
                        0.0,
                        false,
                        false);
                }
            }

            // Fase 84: a single rupture per piece, not a repeating glitch
            // like Parentheses -- "curto-circuito" reads as one real
            // event, not a recurring tic. Excluded from the coda for the
            // same reason Parenthesis is.
            double curtoCircuitoRoll = -1.0;
            if (curtoCircuitoEnabled && !curtoCircuitoFired && !arc.inCoda()) curtoCircuitoRoll = unit(rng);
            if (curtoCircuitoRoll >= 0.0 && apocryphalNearMisses.size() < 6
                && curtoCircuitoRoll >= curtoCircuitoFireChancePerBar
                && curtoCircuitoRoll < curtoCircuitoFireChancePerBar * 3.0) {
                apocryphalNearMisses.push_back({"um Curto-Circuito", curtoCircuitoRoll - curtoCircuitoFireChancePerBar});
            }
            if (curtoCircuitoRoll >= 0.0 && curtoCircuitoRoll < curtoCircuitoFireChancePerBar) {
                curtoCircuitoFired = true;
                curtoCircuitoHoldSamples = static_cast<int>((0.1 + unit(rng) * 0.3) * sampleRate);
                curtoCircuitoSamplesIntoEvent = 0;
                observeExpressiveEvent(
                    ExpressiveEventSource::Structural,
                    ExpressiveScale::Form,
                    i,
                    -1,
                    1.0,
                    static_cast<double>(curtoCircuitoHoldSamples) / sampleRate,
                    state.density,
                    state.resonance,
                    noiseClimate,
                    expressiveBit(ExpressiveDomain::Structure)
                        | expressiveBit(ExpressiveDomain::Material)
                        | expressiveBit(ExpressiveDomain::Timing),
                    expressiveBit(ExpressiveCause::Rupture),
                    0.0,
                    0.0,
                    false,
                    false);

                auto gatecrash = [&](const char *name) {
                    if (!curtoCircuitoGatecrashedNames.empty()) curtoCircuitoGatecrashedNames += ", ";
                    curtoCircuitoGatecrashedNames += name;
                    ++curtoCircuitoGatecrashCount;
                };
                if (!apocryphalEnabled && unit(rng) < 0.4) {
                    apocryphalEnabled = true;
                    apocryphal.configure(0.045, unit(rng) < 0.25 ? 2 : 1);
                    gatecrash("arquivo apocrifo");
                }
                if (!freeIntonationEnabled && unit(rng) < 0.4) {
                    freeIntonationEnabled = true;
                    freeIntonationDepth = 2.0 + unit(rng) * 6.0;
                    gatecrash("entonacao livre");
                }
                if (!tonalModulationEnabled && unit(rng) < 0.4) {
                    tonalModulationEnabled = true;
                    gatecrash("modulacao tonal");
                }
                if (!selfFeedbackEnabled && unit(rng) < 0.4) {
                    selfFeedbackEnabled = true;
                    gatecrash("self-feedback");
                }
                // Two more candidates beyond the fase-40 safety-net set --
                // both fully configured unconditionally before the loop
                // (Rungler's rates, Estranho's chance/gap/duration), so
                // flipping the flag mid-piece is exactly as safe as the
                // four above.
                if (!runglerEnabled && unit(rng) < 0.4) {
                    runglerEnabled = true;
                    gatecrash("rungler");
                }
                if (!estranhoEnabled && unit(rng) < 0.4) {
                    estranhoEnabled = true;
                    gatecrash("elemento estranho");
                }
            }

            // Fase 97 -- "Galo": at most once per piece, same "single real
            // event" idiom as curto-circuito just above, excluded from the
            // coda for the same reason.
            double galoRoll = -1.0;
            if (galoEnabled && !galoFired && !arc.inCoda()) galoRoll = unit(rng);
            if (galoRoll >= 0.0 && apocryphalNearMisses.size() < 6
                && galoRoll >= galoFireChancePerBar && galoRoll < galoFireChancePerBar * 3.0) {
                apocryphalNearMisses.push_back({"um chamado de Galo", galoRoll - galoFireChancePerBar});
            }
            if (galoRoll >= 0.0 && galoRoll < galoFireChancePerBar) {
                galoFired = true;
                galo.trigger(galoBaseFreq, galoCallDuration);
            }

            // Fase 101 -- "deja vu": can recur, minimum gap enforced so
            // occurrences never cluster (same idiom stochastic mute
            // already uses for its own minimum-gap rule).
            ++dejaVuBarsSinceLast;
            if (dejaVuEnabled && dejaVuBarsSinceLast > dejaVuMinGapBars && !arc.inCoda()
                && unit(rng) < dejaVuFireChancePerBar) {
                dejaVu.trigger(0.4 + unit(rng) * 0.5);
                dejaVuBarsSinceLast = 0;
                ++dejaVuEventCount;
            }

            if (tubaVoiceEnabled && !bassBehaviourEnabled) {
                double tubaFreq = midiToFreq(sequencer.rootMidiNote() - 12 * tubaOctaveDown);
                tubaResonator.setFundamental(std::clamp(tubaFreq, 24.0, 120.0));
                // Drops out during the thinnest sections rather than
                // droning underneath everything unconditionally -- a real
                // player breathes with the music, doesn't hold one note
                // through the whole piece regardless of what it's doing.
                tubaBreath.setGate(state.density > 0.3 && noiseClimate > 0.16f
                    && relationalChamberResponse.tubaPressure > 0.72);
            }

            if (apocryphalEnabled) apocryphal.maybeArm(outSamples.size() / 2);
            if (montageCutEnabled) montageCut.maybeArm(outSamples, outSamples.size() / 2);

            if (agonEnabled) {
                agonSamplesSinceConflict += samplesPerBar;
                if (agonConflictSamplesRemaining <= 0 && agonSamplesSinceConflict > agonMinGapSamples
                    && agonPitchVar < agonStalenessThreshold && unit(rng) < agonConflictChance) {
                    double agonSolidify = std::clamp(arc.elapsedSeconds() / agonSolidifyTauSeconds, 0.0, 1.0);
                    double agonPower[4];
                    double agonBestPower = -1.0;
                    int agonBestIdx = 0;
                    for (int agonK = 0; agonK < 4; ++agonK) {
                        agonPower[agonK] = 0.5 + (agonPowerExtreme[agonK] - 0.5) * agonSolidify;
                        if (agonPower[agonK] > agonBestPower) { agonBestPower = agonPower[agonK]; agonBestIdx = agonK; }
                    }
                    agonConflictWinner = agonBestIdx;
                    // Standing relative to the winner, not a flat split --
                    // a close second cedes little, a distant fourth cedes
                    // close to the old full amount.
                    for (int agonK = 0; agonK < 4; ++agonK)
                        agonRelativePower[agonK] = agonPower[agonK] / agonBestPower;
                    agonConflictSamplesRemaining = static_cast<int>((3.0 + unit(rng) * 4.0) * sampleRate);
                    agonSamplesSinceConflict = 0;
                    ++agonRusgaCount; // a rusga just broke out -- see PieceModuleInfo::agonRusgaCount
                }
                if (agonConflictSamplesRemaining > 0) agonConflictSamplesRemaining -= static_cast<int>(samplesPerBar);
            }

            if (estranhoEnabled) {
                estranhoSamplesSinceLast += samplesPerBar;
                if (estranhoSamplesRemaining <= 0 && estranhoSamplesSinceLast > estranhoMinGapSamples
                    && unit(rng) < estranhoChance) {
                    estranhoSamplesRemaining = static_cast<int>(estranhoDurationSeconds * sampleRate);
                    estranhoSamplesSinceLast = 0;
                    ++estranhoCount;
                }
                if (estranhoSamplesRemaining > 0) estranhoSamplesRemaining -= static_cast<int>(samplesPerBar);
            }

            if (fixedOverflowEnabled) {
                fixedOverflowSamplesSinceLast += samplesPerBar;
                if (fixedOverflowSamplesRemaining <= 0 && fixedOverflowSamplesSinceLast > fixedOverflowMinGapSamples
                    && unit(rng) < fixedOverflowChance) {
                    fixedOverflowSamplesRemaining = static_cast<int>(fixedOverflowDurationSeconds * sampleRate);
                    fixedOverflowSamplesSinceLast = 0;
                    ++fixedOverflowCount;
                }
                if (fixedOverflowSamplesRemaining > 0) fixedOverflowSamplesRemaining -= static_cast<int>(samplesPerBar);
            }

            if (spectralGhostEnabled) {
                spectralGhostSamplesSinceLast += samplesPerBar;
                if (spectralGhostSamplesRemaining <= 0 && spectralGhostSamplesSinceLast > spectralGhostMinGapSamples
                    && unit(rng) < spectralGhostChance) {
                    spectralGhostSamplesRemaining = static_cast<int>(spectralGhostDurationSeconds * sampleRate);
                    spectralGhostSamplesSinceLast = 0;
                    ++spectralGhostCount;
                }
                if (spectralGhostSamplesRemaining > 0) spectralGhostSamplesRemaining -= static_cast<int>(samplesPerBar);
            }

            if (sampleHoldGlitchEnabled) {
                sampleHoldGlitchSamplesSinceLast += samplesPerBar;
                if (sampleHoldGlitchSamplesRemaining <= 0 && sampleHoldGlitchSamplesSinceLast > sampleHoldGlitchMinGapSamples
                    && unit(rng) < sampleHoldGlitchChance) {
                    sampleHoldGlitchSamplesRemaining = static_cast<int>(sampleHoldGlitchDurationSeconds * sampleRate);
                    sampleHoldGlitchSamplesSinceLast = 0;
                    ++sampleHoldGlitchCount;
                }
                if (sampleHoldGlitchSamplesRemaining > 0) sampleHoldGlitchSamplesRemaining -= static_cast<int>(samplesPerBar);
            }

            // Tempo change: a discrete jump-and-hold at a genuine arc
            // transition, gated by its own per-piece chance, not a
            // continuous per-bar recompute -- see tempoDriftEnabled above.
            // Never fires once the ritardando below has armed -- the
            // ending's own gesture takes over from here, not one more
            // structural jump competing with it.
            if (tempoDriftEnabled && !ritardandoArmed && !accelerandoArmed && !lastModulationSectionName.empty()
                && state.currentSectionName != lastModulationSectionName
                && unit(rng) < tempoChangeChance) {
                double changeFactor;
                if (metricModulationEnabled) {
                    int numRatios = static_cast<int>(sizeof(kMetricModulationRatios) / sizeof(kMetricModulationRatios[0]));
                    changeFactor = kMetricModulationRatios[static_cast<int>(unit(rng) * numRatios) % numRatios];
                    ++metricModulationEventCount;
                } else {
                    changeFactor = 1.0 + (unit(rng) * 2.0 - 1.0) * tempoDriftDepth;
                }
                currentBpm = std::clamp(currentBpm * changeFactor, cfg.bpm * 0.55, cfg.bpm * 1.75);
                ++tempoChangeCount;
                tempoTimeline.push_back({arc.elapsedSeconds(), currentBpm});
            }
            // Accelerando/stringendo glide -- same exponential-approach
            // curve as the ritardando below, easing UP toward a faster
            // target instead of down. Ritardando gets final say: once the
            // coda genuinely arms, that block below overwrites currentBpm
            // every bar from then on, same as it already does over the
            // discrete tempo-jump logic above.
            if (accelerandoArmed) {
                double tSincePeak = arc.elapsedSeconds() - accelerandoStartSeconds;
                double targetBpm = accelerandoStartBpm * accelerandoTargetFactor;
                double glide = 1.0 - std::exp(-tSincePeak / accelerandoTauSeconds);
                currentBpm = accelerandoStartBpm + (targetBpm - accelerandoStartBpm) * glide;
                tempoTimeline.push_back({arc.elapsedSeconds(), currentBpm});
            }
            if (ritardandoArmed) {
                // Exponential glide from the tempo the piece happened to
                // be at when the coda began, down toward a slower target --
                // a real ritardando curve (fast at first, easing into the
                // target) rather than a linear ramp or another instant jump.
                double tSinceCoda = arc.elapsedSeconds() - ritardandoStartSeconds;
                double targetBpm = ritardandoStartBpm * ritardandoTargetFactor;
                double glide = 1.0 - std::exp(-tSinceCoda / ritardandoTauSeconds);
                currentBpm = ritardandoStartBpm + (targetBpm - ritardandoStartBpm) * glide;
                tempoTimeline.push_back({arc.elapsedSeconds(), currentBpm});
            }
            double secondsPerStepNow = 60.0 / currentBpm / 4.0;
            samplesPerStep = static_cast<size_t>(secondsPerStepNow * sampleRate);
            samplesPerBar = samplesPerStep * static_cast<size_t>(stepsPerBar);
            nextBarBoundary = i + samplesPerBar;
            if (phasingEnabled) {
                phaseSamplesPerStep = std::max<size_t>(1, static_cast<size_t>(static_cast<double>(samplesPerStep) / (1.0 + phasingDelta)));
            }
            if (spectralObjectEnabled) {
                spectralObject.setBasePeriodSamples(samplesPerBar * static_cast<size_t>(spectralObjectRhythmBars));
            }

            // Real tonal-centre modulation (and its cheaper sibling, modal
            // shift): only ever considered right at the start of a
            // genuinely new arc section (not the very first one -- there's
            // no "previous" key to move away from yet). Mutually exclusive
            // per transition -- a tonal modulation gets first refusal, a
            // modal shift only fires if that didn't happen here.
            if (!rasgarHarmonicShiftThisBar
                && (tonalModulationEnabled || modalShiftEnabled) && !lastModulationSectionName.empty()
                && state.currentSectionName != lastModulationSectionName) {
                bool didTonalModulation = false;
                // Even when enabled, only 40% of eligible transitions
                // actually modulate -- most section changes stay in the
                // same key, so an actual modulation still reads as a
                // structural event, not background noise.
                if (tonalModulationEnabled && unit(rng) < 0.4) {
                    double weightTotal = 0.0;
                    for (double w : kModulationWeights) weightTotal += w;
                    double roll = unit(rng) * weightTotal;
                    double acc = 0.0;
                    int interval = kModulationIntervals[8];
                    for (int k = 0; k < 9; ++k) {
                        acc += kModulationWeights[k];
                        if (roll <= acc) { interval = kModulationIntervals[k]; break; }
                    }
                    int direction = (unit(rng) < 0.5) ? 1 : -1;
                    int newRoot = sequencer.rootMidiNote() + direction * interval;
                    // Wrap back toward the original octave rather than
                    // letting several modulations in a long piece drift the
                    // melodic voice arbitrarily high or low.
                    while (newRoot > pieceRootMidiNote + 12) newRoot -= 12;
                    while (newRoot < pieceRootMidiNote - 12) newRoot += 12;

                    Scale newScale = sequencer.scale();
                    if (unit(rng) < 0.6) {
                        int scaleCount = static_cast<int>(Scale::Count);
                        Scale candidate;
                        do {
                            candidate = static_cast<Scale>(static_cast<int>(unit(rng) * scaleCount) % scaleCount);
                        } while (candidate == sequencer.scale());
                        newScale = candidate;
                    }
                    if (microtonalEnabled) {
                        // A real, unscripted consequence of tuning by pure
                        // ratio instead of equal temperament: chaining the
                        // interval's OWN pure-ratio cents (not resetting
                        // cleanly to 12-TET) across genuine modulations is
                        // exactly how a real JI ensemble/choir's absolute
                        // pitch reference honestly drifts over a long piece
                        // -- see MicrotonalTuning.hpp's own comment for the
                        // Pythagorean-comma math this reproduces.
                        microtonalTuning.accumulateModulationDrift(sequencer.rootMidiNote(), newRoot);
                    }
                    sequencer.modulateTo(newRoot, newScale);
                    ++modulationCount;
                    didTonalModulation = true;
                }

                // Same root, new scale/mode -- the tonal centre itself
                // never moves, just its colour. A lighter, more frequent
                // event than the full modulation above.
                if (!didTonalModulation && modalShiftEnabled && unit(rng) < 0.35) {
                    int scaleCount = static_cast<int>(Scale::Count);
                    Scale candidate;
                    do {
                        candidate = static_cast<Scale>(static_cast<int>(unit(rng) * scaleCount) % scaleCount);
                    } while (candidate == sequencer.scale());
                    sequencer.modulateTo(sequencer.rootMidiNote(), candidate);
                    ++modalShiftCount;
                }
            }
            // Fase 89 -- real meter change, rolled at the same genuine
            // section boundary as everything else here. Weighted so 16
            // stays the single most common outcome even for pieces that
            // have the capability at all.
            if (meterVarianceEnabled && !lastModulationSectionName.empty()
                && state.currentSectionName != lastModulationSectionName) {
                static const int kBarLengths[] = {16, 16, 12, 20, 14};
                std::uniform_int_distribution<int> meterDist(0, 4);
                int newBarLength = kBarLengths[meterDist(meterRng)];
                if (newBarLength != currentBarLength) {
                    currentBarLength = newBarLength;
                    currentGroupSize = groupSizeForBarLength(currentBarLength);
                    sequencer.setNumSteps(currentBarLength);
                    generatedRhythm = generateRhythmSieve(rng, currentBarLength);
                    generatedAksak = generateAksakPattern(rng, currentBarLength);
                    generatedEuclid = generateEuclideanPattern(rng, 2, 11, currentBarLength);
                    generatedEuclidSnare = generateEuclideanPattern(rng, 2, 11, currentBarLength);
                    for (int guard = 0; guard < 10 && generatedEuclidSnare.onset == generatedEuclid.onset; ++guard) {
                        generatedEuclidSnare = generateEuclideanPattern(rng, 2, 11, currentBarLength);
                    }
                    generatedEuclidHihat = generateEuclideanPattern(
                        rng, 4, std::max(4, currentBarLength - 2), currentBarLength);
                    ++meterChangeCount;
                }
            }
            // Fase 67: a fresh arc section is exactly the "passagem" the
            // user asked for -- forced to a genuinely DIFFERENT archetype
            // (never re-rolling the same one) so a section change is
            // always audible as a real feel change, not a coin flip that
            // might silently repeat. Fase 89: once the current bar length
            // (just possibly updated above) isn't 16, only the three
            // generated slots are real candidates -- the fixed catalog
            // can't represent any other length.
            if (drumArchetypeEnabled && !lastModulationSectionName.empty()
                && state.currentSectionName != lastModulationSectionName) {
                const int newIndex = chooseArchetype(currentBarLength != 16);
                currentArchetypeIndex = newIndex;
                rememberArchetype(currentArchetypeIndex);
                ++archetypeChangeCount;
            }
            // Fase 85 stage 3 -- H: re-decide orchestration at the same
            // genuine section boundary, independent of whether archetype
            // mode happens to be on (every piece has a real percussion
            // ensemble to arrange, archetype or not). Voice count scales
            // with the arc's own density; kick/hihat get a much higher
            // floor than the other six since real drumming almost never
            // drops both of its anchor voices at once.
            if (drumsEnabled && !lastModulationSectionName.empty()
                && state.currentSectionName != lastModulationSectionName) {
                std::uniform_real_distribution<double> orchUnit(0.0, 1.0);
                int extraVoices = 2 + static_cast<int>(std::round(state.density * 4.0)); // 2..6 of the remaining 6
                std::array<int, 6> others = {1, 3, 4, 5, 6, 7}; // snare/cowbell/clap/toms/cymbal/maracas
                std::shuffle(others.begin(), others.end(), orchestrationRng);
                voiceActive[0] = orchUnit(orchestrationRng) < 0.92; // kick anchor
                voiceActive[2] = orchUnit(orchestrationRng) < 0.90; // hihat anchor
                for (int oi = 0; oi < 6; ++oi) voiceActive[static_cast<size_t>(others[static_cast<size_t>(oi)])] = oi < extraVoices;
                ++orchestrationChangeCount;
            }
            // Fase 104 -- the flute's own harmonic wanderer advances at the
            // same genuine section boundary as everything else above, one
            // real technique deciding the new key centre (see
            // HarmonicWanderer.hpp) -- never on the very first section,
            // same guard every other section-boundary mechanism here uses.
            if (fmFluteEnabled && !lastModulationSectionName.empty()
                && state.currentSectionName != lastModulationSectionName) {
                harmonicWanderer.advance();
                voicingBank.retune(harmonicWanderer.current());
            }
            // Fase 113 -- objeto espectral, Eixo A e Eixo B avancam na
            // mesma fronteira real de secao. Eixo A (harmonicidade<->
            // inarmonicidade) e pilotado pela densidade do arco -- a
            // propria forma da peca decide o quao estavel o espectro soa
            // agora, secao calma quase-harmonico, climax esticado. Eixo B
            // (buracos de sieve) reamostra as condicoes -- o espectro
            // regenera ao vivo, nunca fica estatico a peca inteira.
            if (spectralObjectEnabled && !lastModulationSectionName.empty()
                && state.currentSectionName != lastModulationSectionName) {
                spectralObject.setInharmonicity(state.density * 0.004);
                spectralObject.regenerateSieveHoles(cfg.seed + 79000u + static_cast<unsigned>(barCounter));
            }
            // Fase 116 -- rasgo espectral: tensao ambiente real aplicada
            // na mesma fronteira de secao, proporcional a densidade do
            // arco -- a propria proximidade do climax e a "pressao" que
            // empurra o feixe de fibras rumo aos proprios limiares.
            // Identifica quais fibras rasgaram AGORA (comparando estado
            // antes/depois) pra disparar o gesto de rasgo real na
            // frequencia exata daquele parcial, e liga o loop permanente
            // da envelope -- o "deshiar" que nunca mais para.
            if (rasgoFractureEnabled && !lastModulationSectionName.empty()
                && state.currentSectionName != lastModulationSectionName) {
                std::array<bool, RasgoFracture::kNumFibers> wasBroken{};
                for (int fi = 0; fi < RasgoFracture::kNumFibers; ++fi) wasBroken[static_cast<size_t>(fi)] = rasgoFracture.isBroken(fi);
                rasgoFracture.applyAmbientStress(state.density * rasgoAmbientStressGain);
                for (int fi = 0; fi < RasgoFracture::kNumFibers; ++fi) {
                    if (!wasBroken[static_cast<size_t>(fi)] && rasgoFracture.isBroken(fi)) {
                        double tearFreq = spectralObject.partialFrequency(fi) * (0.97 + unit(rng) * 0.06);
                        rasgoTearOsc[static_cast<size_t>(fi)].setFrequency(tearFreq);
                        rasgoTearEnv[static_cast<size_t>(fi)].setLoopEnabled(true);
                        rasgoTearEnv[static_cast<size_t>(fi)].noteOn();
                        ++rasgoTotalBreaksReported;
                    }
                }
            }
            // Fase 104 -- flute phrasing: a real melodic PHRASE (legato,
            // one note sustained 1-3 bars), not another voice locked to
            // the 16-step grid -- re-decided once per bar, independent of
            // the section-boundary harmonic advance above (the key centre
            // can hold steady across several phrase notes). Each new note
            // picks a real scale degree from the CURRENT key centre's
            // scale, two octaves up into a real flute register.
            if (fmFluteEnabled && musicalDirection.mayStart(OrganismStem::FmFlute)
                && fmFluteNoteSamplesRemaining <= samplesPerBar) {
                if (unit(rng) < 0.65) {
                    const ScaleTable &table = scaleTable(harmonicWanderer.current().scale);
                    int degreeIdx = static_cast<int>(unit(rng) * table.length) % table.length;
                    if (cfg.musicalDirectionEnabled)
                        degreeIdx = musicalDirection.motifDegree(static_cast<unsigned>(fmFluteNoteCount), table.length);
                    if (musicalRelations.enabled())
                        degreeIdx = musicalRelations.melodyDegree(static_cast<unsigned>(fmFluteNoteCount), table.length);
                    int semitoneOffset = table.degrees[static_cast<size_t>(degreeIdx)];
                    int registerOffset = 24 + (unit(rng) < 0.3 ? 12 : 0);
                    if (cfg.musicalDirectionEnabled && !musicalDirection.plan().highFlash) registerOffset = 12;
                    int targetOffset = semitoneOffset + registerOffset;
                    // Eixo C: what actually sounds may differ from the real
                    // target (wrong note/bebop/suspension) -- see
                    // DissonanceTreatment.hpp. fmFluteRootForNote_ is
                    // captured once per note so the mid-note event fired
                    // from tick() below transposes against the SAME root
                    // even if the wanderer or bar boundary moves on.
                    fmFluteRootForNote = harmonicWanderer.current().rootMidiNote;
                    fmFlutePendingEvents = dissonanceTreatment.apply(targetOffset, fmFlutePreviousOffset);
                    const double targetMidi = static_cast<double>(fmFluteRootForNote + fmFlutePendingEvents[0].semitoneOffset);
                    int barsForThisNote = 1 + static_cast<int>(unit(rng) * 3.0);
                    const double noteBars = musicalRelations.enabled()
                        ? musicalRelations.fmDurationBars(static_cast<unsigned>(fmFluteNoteCount)) : barsForThisNote;
                    const bool phraseBoundary = fmFlutePhraseIndex == fmFlutePhraseLength - 1;
                    MelodicEvent melodicEvent;
                    melodicEvent.midi = targetMidi;
                    melodicEvent.duration = noteBars
                        * static_cast<double>(samplesPerBar) / sampleRate;
                    melodicEvent.density = state.density;
                    melodicEvent.accent = (degreeIdx == 0 ? 0.25 : 0.0) + state.resonance * 0.2;
                    melodicEvent.phrasePosition = fmFlutePhraseLength > 1
                        ? static_cast<double>(fmFlutePhraseIndex) / static_cast<double>(fmFlutePhraseLength - 1)
                        : 0.0;
                    melodicEvent.phraseBoundary = phraseBoundary;
                    fmFluteInterpretation = fmFluteMelodicInterpreter.interpret(melodicEvent);
                    const double arrivalMidi = std::clamp(fmFluteInterpretation.arrivalMidi, 36.0, 108.0);
                    const double renderedMidi = cfg.expressiveActuationEnabled
                        ? std::clamp(fmFluteInterpretation.livingMidi, 36.0, 108.0)
                        : arrivalMidi;
                    if (cfg.expressiveActuationEnabled) {
                        lastMaterialExpressionPlan = materialExpressionField.interpret({
                            fmFluteInterpretation.timbreBrightness / 1.5,
                            fmFluteInterpretation.articulation / 1.15,
                            1.0 - fmFluteInterpretation.breath,
                            1.0 - fmFluteInterpretation.sustain,
                            state.resonance});
                        lastSpatialExpressionPlan = spatialExpressionField.interpret({
                            relationalChamberResponse.spatialPolarity,
                            fmFluteInterpretation.phraseEnergy,
                            fmFluteInterpretation.gestureMemory,
                            state.resonance,
                            1.0});
                        lastExpressiveInterpretation.enabled = true;
                        lastExpressiveInterpretation.melodic = fmFluteInterpretation;
                        lastExpressiveInterpretation.material = lastMaterialExpressionPlan;
                        lastExpressiveInterpretation.spatial = lastSpatialExpressionPlan;
                    }
                    const int arrivalTelemetryNote = static_cast<int>(std::lround(renderedMidi));
                    if (!fmFlutePitchTelemetryInitialized) {
                        fmFlutePitchTelemetryInitialized = true;
                        fmFluteCurrentRepeatedNotes = 1;
                        fmFlutePitchMin = fmFlutePitchMax = arrivalTelemetryNote;
                    } else if (arrivalTelemetryNote == fmFlutePreviousTelemetryNote) {
                        ++fmFluteCurrentRepeatedNotes;
                        ++fmFluteRepeatedPitchCount;
                    } else {
                        ++fmFlutePitchChangeCount;
                        fmFluteCurrentRepeatedNotes = 1;
                    }
                    fmFluteLongestRepeatedNotes = std::max(
                        fmFluteLongestRepeatedNotes, fmFluteCurrentRepeatedNotes);
                    fmFlutePitchMin = std::min(fmFlutePitchMin, arrivalTelemetryNote);
                    fmFlutePitchMax = std::max(fmFlutePitchMax, arrivalTelemetryNote);
                    fmFlutePreviousTelemetryNote = arrivalTelemetryNote;
                    fmFlute.setFrequency(midiToFreq(renderedMidi));
                    if (cfg.musicalDirectionEnabled) {
                        const auto& gesture = musicalDirection.plan();
                        fmFlute.configure(1.0 + gesture.material * 0.035,
                            std::clamp(2.5 + gesture.material * 1.8 + gesture.articulation, 0.8, 5.0),
                            std::clamp(0.30 + gesture.material * 0.22, 0.08, 0.60),
                            0.018 + (1.0 - gesture.articulation) * 0.11,
                            0.10 + (1.0 - gesture.articulation) * 0.34);
                    }
                    if (musicalRelations.enabled()) {
                        const auto& control = musicalRelations.controls(MusicalBody::Fm);
                        // Third configure argument is sustain INDEX, not breath noise.
                        fmFlute.configure(musicalRelations.fmRatio(),
                            std::clamp(2.2 + control.material * 2.0 + control.articulation, .8, 4.8),
                            std::clamp(.30 + control.material * .28, .05, .65),
                            .008 + (1.0 - control.articulation) * .10,
                            .08 + (1.0 - control.articulation) * .30);
                        musicalRelations.recordApplied(MusicalBody::Fm);
                    }
                    fmFlute.setGain(fmFluteBaseGain * static_cast<float>(
                        std::clamp(relationalChamberResponse.fluteGain
                            * fmFluteInterpretation.articulation
                            * (0.78 + 0.22 * fmFluteInterpretation.breath),
                            0.45, 1.2)));
                    fmFlute.noteOn();
                    fmFluteNextEventIdx = 1;
                    fmFlutePreviousOffset = fmFlutePendingEvents.back().semitoneOffset;
                    fmFluteNoteTotalSamples = musicalRelations.enabled()
                        ? std::max<size_t>(1, static_cast<size_t>(std::llround(samplesPerBar * noteBars)))
                        : samplesPerBar * static_cast<size_t>(barsForThisNote);
                    fmFluteNoteSamplesRemaining = fmFluteNoteTotalSamples;
                    fmFluteNoteSamplesElapsed = 0;
                    ++fmFluteNoteCount;
                    observeExpressiveEvent(
                        ExpressiveEventSource::FmFlute,
                        ExpressiveScale::PatternCycle,
                        i,
                        static_cast<int>(fmFlutePhraseIndex),
                        renderedMidi,
                        fmFluteBaseGain * fmFluteInterpretation.articulation,
                        static_cast<double>(fmFluteNoteTotalSamples) / sampleRate,
                        state.density,
                        state.resonance,
                        noiseClimate,
                        expressiveBit(ExpressiveDomain::Energy)
                            | expressiveBit(ExpressiveDomain::Duration)
                            | expressiveBit(ExpressiveDomain::Gesture)
                            | expressiveBit(ExpressiveDomain::Pitch)
                            | expressiveBit(ExpressiveDomain::Material),
                        expressiveBit(ExpressiveCause::Phrase)
                            | expressiveBit(ExpressiveCause::Density)
                            | (phraseBoundary
                                ? expressiveBit(ExpressiveCause::MetricAccent) : 0u),
                        fmFluteInterpretation.breath);
                    fmFlutePhraseIndex = phraseBoundary ? 0 : fmFlutePhraseIndex + 1;
                } else {
                    fmFlute.noteOff();
                    fmFluteNoteSamplesRemaining = samplesPerBar;
                    fmFlutePendingEvents.clear();
                    fmFluteNextEventIdx = 0;
                }
            } else if (fmFluteEnabled) {
                fmFluteNoteSamplesRemaining = MusicalDirection::advanceNoteCountdown(
                    fmFluteNoteSamplesRemaining, samplesPerBar);
            }
            lastModulationSectionName = state.currentSectionName;
        }

        const float noiseClimateCoeff = static_cast<float>(
            1.0 - std::exp(-1.0 / (0.35 * sampleRate)));
        const float noisePresenceCoeff = static_cast<float>(
            1.0 - std::exp(-1.0 / (0.18 * sampleRate)));
        noiseClimate += (noiseClimateTarget - noiseClimate) * noiseClimateCoeff;
        granularPresence += (granularPresenceTarget - granularPresence) * noisePresenceCoeff;
        gendynPresence += (gendynPresenceTarget - gendynPresence) * noisePresenceCoeff;
        coloredNoisePresence += (coloredNoisePresenceTarget - coloredNoisePresence) * noisePresenceCoeff;
        noiseClimate = std::clamp(noiseClimate, 0.0f, 1.0f);
        noiseClimateMin = std::min(noiseClimateMin, noiseClimate);
        noiseClimateMax = std::max(noiseClimateMax, noiseClimate);
        noiseClimateSum += noiseClimate;
        ++noiseClimateSamples;

        if (stepBoundary) {
            int step = static_cast<int>(stepCounter % static_cast<size_t>(sequencer.numSteps()));
            const std::size_t bassStep = stepCounter % BassFunctionPlan::kMaxSteps;
            if (polymeterEnabled) polymeterEnsemble.tickStep(stepCounter);
            ++stepCounter;
            const double rasgarStepScale = rasgarGesture.stepDurationScale(
                step, sequencer.numSteps());
            const size_t rasgarStepSamples = std::max<size_t>(
                1, static_cast<size_t>(std::llround(
                    static_cast<double>(samplesPerStep) * rasgarStepScale)));
            nextStepBoundary = i + rasgarStepSamples;
            if (step != globalStep) {
                globalStep = step;
                if (step == 0) {
                    ++barCounter;
                    grooveDialogue.beginBar(static_cast<unsigned>(std::max(0, barCounter)),
                        static_cast<unsigned>(arc.history().size()), currentBarLength,
                        currentBpm, state.density);
                    // Fase 85 stage 2, gesture (C): resample the phrase
                    // contour every 4 bars, holding it steady in between --
                    // a real phrase-level push/pull, not a per-step wobble.
                    if (barCounter % gesturePhraseBars == 1) {
                        gestureBias = std::clamp(static_cast<double>(lastLfoOut1), -1.0, 1.0);
                    }
                    // Fase 85 stage 2, repetition with difference (E): a
                    // bounded chance, every real bar repeat, of arming ONE
                    // quiet ghost hihat at a freshly-chosen step -- never
                    // touching the archetype's own defining kick/snare
                    // pattern, just a small live variation on top of it.
                    // Fase 85 stage 3, F: that chance itself now scales
                    // with the arc's own density -- a calm section stays
                    // drier, the piece varies itself more insistently as
                    // it builds toward the climax.
                    if (drumsEnabled) {
                        double ghostChance = 0.12 + 0.25 * state.density;
                        ghostHihatArmedThisBar = std::uniform_real_distribution<double>(0.0, 1.0)(ghostRng) < ghostChance;
                        ghostHihatStep = ghostHihatArmedThisBar
                            ? std::uniform_int_distribution<int>(0, currentBarLength - 1)(ghostRng)
                            : -1;
                    } else {
                    // A silent drum family has no audible repetition to
                    // vary. Do not report or schedule ghost hits that
                    // will be discarded at the bus gate.
                        ghostHihatArmedThisBar = false;
                        ghostHihatStep = -1;
                    }
                    // The temporal architecture now has a second consumer:
                    // a restrained per-bar imprint for the drum voices. It
                    // preserves the selected archetype and only contributes
                    // a bounded relational offset on top of the existing
                    // humanization/swing path.
                    drumGroove = TemporalGrooveImprint{};
                    drumGroove.enabled = cfg.expressiveActuationEnabled && drumsEnabled;
                    drumGroove.strength = drumGroove.enabled ? 0.18 : 0.0;
                    drumGroove.swing = (drumArchetypeEnabled
                                         && currentArchetypeIndex < rhythmSieveSlotIndex)
                        ? std::clamp(drumArchetypes()[static_cast<std::size_t>(
                              currentArchetypeIndex)].swingAmount * 4.0, 0.0, 1.0)
                        : 0.0;
                    drumGroove.microtiming = std::clamp(std::abs(gestureBias), 0.0, 1.0);
                    drumGroove.humanization = samplesPerStep > 0
                        ? std::clamp(static_cast<double>(humanizeMaxSamples)
                                         / static_cast<double>(samplesPerStep) * 4.0,
                                     0.0, 1.0)
                        : 0.0;
                    drumGroove.accent = 0.38;
                    drumGroove.gate = 0.24;
                    drumGroove.memory = std::clamp(gamePlan.memory, 0.0, 1.0);
                    for (std::size_t grooveStep = 0;
                         grooveStep < TemporalGrooveImprint::kMaxSteps; ++grooveStep) {
                        const double metric = metricalStrengthForBar(
                            static_cast<int>(grooveStep), 16, 4);
                        const double detail = identityUnit(
                            cfg.seed, 18800u
                                + static_cast<std::uint64_t>(barCounter) * 16u
                                + grooveStep);
                        auto &imprint = drumGroove.steps[grooveStep];
                        imprint.timingOffsetSteps = std::clamp(
                            ((grooveStep % 2u == 1u) ? drumGroove.swing * 0.25 : 0.0)
                                + gestureBias * 0.012
                                + (detail - 0.5) * drumGroove.humanization * 0.018,
                            -0.08, 0.08);
                        imprint.accentDelta = std::clamp(
                            (metric - 0.5) * drumGroove.accent, -1.0, 1.0);
                        imprint.gateScale = std::clamp(
                            0.94 + metric * drumGroove.gate * 0.18,
                            0.75, 1.20);
                        imprint.attackDelta = std::clamp(
                            (metric - 0.5) * 0.28, -1.0, 1.0);
                    }
                    // Fase 85 stage 3, G: the post-rupture full-kit window
                    // (armed above, right when Curto-Circuito's gate
                    // finishes reopening) counts down in real bars.
                    if (postGlitchFullKitBars > 0) --postGlitchFullKitBars;
                }

                if (bassBehaviourEnabled && bassStep == 0) {
                    if (bassCycleCount > 0) bassKickMemory = bassKickCapture;
                    bassKickCapture.fill(false);

                    const auto chordOffsets = sequencer.currentChordOffsets();
                    bassBodyRootMidi = std::clamp(
                        sequencer.rootMidiNote() + chordOffsets[0], 0, 127);
                    const int nextBassRootMidi = std::clamp(
                        sequencer.rootMidiNote() + sequencer.nextChordRootOffset(),
                        0, 127);
                    BassFunctionRequest bassFunctionRequest;
                    bassFunctionRequest.seed = cfg.seed;
                    bassFunctionRequest.sectionIndex = bassCycleCount;
                    bassFunctionRequest.stepCount = 16;
                    bassFunctionRequest.rootMidiNote = bassBodyRootMidi;
                    bassFunctionRequest.nextRootMidiNote = nextBassRootMidi;
                    bassFunctionRequest.density = state.density;
                    bassFunctionRequest.phrasePosition = static_cast<double>(
                        bassCycleCount % 4) / 3.0;
                    bassFunctionRequest.harmonicMotion = std::clamp(
                        std::fabs(static_cast<double>(nextBassRootMidi
                                                      - bassBodyRootMidi)) / 12.0,
                        0.0, 1.0);
                    bassFunctionRequest.narrativeTension = std::clamp(
                        state.resonance * 0.56 + gamePlan.risk * 0.44, 0.0, 1.0);
                    bassFunctionRequest.gravity = std::clamp(
                        0.48 + (1.0 - state.density) * 0.28
                            + gamePlan.memory * 0.18, 0.0, 1.0);
                    bassFunctionRequest.mobility = std::clamp(
                        0.18 + state.density * 0.42
                            + gamePlan.freedom * 0.28, 0.0, 1.0);
                    bassFunctionRequest.space = std::clamp(
                        0.18 + (1.0 - state.density) * 0.64, 0.0, 1.0);
                    bassFunctionRequest.lock = drumsEnabled
                        ? std::clamp(0.34 + gamePlan.memory * 0.34
                                         + (rhythmicProtagonist ? 0.18 : 0.0),
                                     0.0, 1.0)
                        : 0.18;
                    bassFunctionRequest.mutation = std::clamp(
                        0.10 + gamePlan.freedom * 0.34
                            + gamePlan.risk * 0.18, 0.0, 1.0);
                    bassFunctionRequest.risk = gamePlan.risk;
                    bassFunctionRequest.memory = gamePlan.memory;
                    bassFunctionRequest.allowSilence = true;
                    bassFunctionRequest.kickPattern = bassKickMemory;
                    const BassFunctionPlan bassFunction =
                        BassFunctionPlanner::plan(bassFunctionRequest);

                    BassPhraseRequest bassPhraseRequest;
                    bassPhraseRequest.function = bassFunction;
                    bassPhraseRequest.history = bassPhraseHistory;
                    bassPhraseRequest.phraseStart = bassCycleCount % 4 == 0;
                    bassPhraseRequest.phraseEnd = bassCycleCount % 4 == 3;
                    bassPhraseRequest.kickPattern = bassKickMemory;
                    const BassPhrasePlan bassPhrase =
                        BassPhrasePlanner::plan(bassPhraseRequest);

                    TemporalGrooveImprint bassGroove;
                    bassGroove.enabled = true;
                    bassGroove.strength = std::clamp(
                        0.48 + gamePlan.memory * 0.30, 0.0, 1.0);
                    const bool generatedRhythmHere =
                        currentArchetypeIndex >= rhythmSieveSlotIndex;
                    const double bassSwing = drumArchetypeEnabled
                        && !generatedRhythmHere
                        ? drumArchetypes()[static_cast<std::size_t>(
                              currentArchetypeIndex)].swingAmount
                        : 0.0;
                    bassGroove.swing = std::clamp(bassSwing * 4.0, 0.0, 1.0);
                    bassGroove.microtiming = std::clamp(
                        std::fabs(gestureBias), 0.0, 1.0);
                    bassGroove.humanization = samplesPerStep > 0
                        ? std::clamp(static_cast<double>(humanizeMaxSamples)
                                         / static_cast<double>(samplesPerStep) * 8.0,
                                     0.0, 1.0)
                        : 0.0;
                    bassGroove.accent = 0.62;
                    bassGroove.gate = 0.48;
                    bassGroove.memory = gamePlan.memory;
                    for (std::size_t grooveStep = 0;
                         grooveStep < BassFunctionPlan::kMaxSteps; ++grooveStep) {
                        const double metric = metricalStrengthForBar(
                            static_cast<int>(grooveStep), 16, 4);
                        const double detail = identityUnit(
                            cfg.seed, 17700u
                                + static_cast<std::uint64_t>(bassCycleCount) * 16u
                                + grooveStep);
                        auto &imprint = bassGroove.steps[grooveStep];
                        imprint.timingOffsetSteps = std::clamp(
                            ((grooveStep % 2u == 1u) ? bassSwing : 0.0)
                                + gestureBias * 0.025
                                + (detail - 0.5) * bassGroove.humanization * 0.035,
                            -0.25, 0.25);
                        imprint.accentDelta = std::clamp(
                            (metric - 0.5) * 0.72, -1.0, 1.0);
                        imprint.gateScale = std::clamp(
                            0.82 + metric * 0.34
                                + (detail - 0.5) * 0.10,
                            0.25, 1.75);
                        imprint.attackDelta = std::clamp(
                            (metric - 0.5) * 0.58, -1.0, 1.0);
                    }

                    if (grooveDialogue.active()) {
                        bassGroove = grooveDialogue.bassImprint();
                        grooveDialogue.recordBassImprint();
                    }
                    BassPerformanceRequest bassPerformanceRequest;
                    bassPerformanceRequest.phrase = bassPhrase;
                    bassPerformanceRequest.groove = bassGroove;
                    bassPerformanceRequest.performer = bassPerformer;
                    bassPerformanceRequest.stepDurationSeconds =
                        static_cast<double>(samplesPerStep) / sampleRate;
                    bassPerformanceRequest.controlledImperfection =
                        std::clamp(0.08 + gamePlan.risk * 0.22, 0.0, 0.34);
                    bassPerformanceRequest.hasObservedFeedback =
                        bassExecutionFeedback.takeObservation(bassFeedbackPhraseBoundary,
                            bassPerformanceRequest.previousObservation);
                    const BassPerformancePlan bassPerformance =
                        BassPerformancePlanner::plan(bassPerformanceRequest);
                    bassBodyPlan = BassBodyAdapterPlanner::plan(
                        bassPerformance,
                        {tubaVoiceEnabled, acidBasslineEnabled, true});
                    if (bassBodyPlan.valid()) {
                        if (bassPerformanceRequest.hasObservedFeedback) {
                            bassPerformer = bassPerformance.performerAtPlan;
                            bassExecutionFeedback.applied(bassPerformer);
                        }
                        bassFeedbackPhraseBoundary = bassPhrase.boundary != BassBoundaryGesture::None;
                        bassPhraseHistory = bassPhrase.nextHistory;
                        bassFinalRole = bassPhrase.role;
                        bassFinalBody = bassBodyPlan.primaryBody;
                        bassGrooveApplicationCount +=
                            bassPerformance.grooveApplicationCount;
                        ++bassPlanCount;
                    }
                    ++bassCycleCount;
                }

                // The divider advances on every step regardless of whether
                // the melodic voice gates -- it's a separate master clock,
                // same grid, different pattern family (XOR combinations of
                // its four divisions, not the Euclidean gate above).
                if (crossDomainContagionEnabled) {
                    ++stepsSinceContagion;
                    if (audioPeakFollower > contagionThreshold && stepsSinceContagion >= contagionMinGapSteps) {
                        // The extra pulse IS the contamination -- every
                        // downstream XOR pattern (kick/snare/hihat/
                        // cowbell/clap/tom) permanently shifts phase from
                        // here on, a real consequence of how loud the
                        // piece's own recent output actually got.
                        polyDivider.clock();
                        stepsSinceContagion = 0;
                        ++crossDomainContagionCount;
                    }
                }
                polyDivider.clock();
                // Rhythmic humanization: rather than firing exactly on this
                // sample-accurate step boundary, each hit's actual trigger
                // is scheduled a small random number of samples later (see
                // rollHumanizeDelay() below) -- a human player's timing
                // imprecision, not a lookahead-scheduled "early" hit (no
                // lookahead exists, so jitter is delay-only). Actually
                // firing happens in the per-sample pending-trigger check
                // further down, every iteration of this loop.
                unsigned plannedSnareAttacks = 1, plannedHihatAttacks = 1;
                bool scheduledSnareHead = false, scheduledHihatHead = false;
                if (drumArchetypeEnabled || grooveDialogue.active()) {
                    // Fase 67: kick/snare/hihat driven by the current
                    // archetype's explicit 16-step onset grid instead of
                    // the XOR clock -- real rhythmic identity (backbeat,
                    // blast beat, march pulse, swing) that the XOR
                    // permutation structurally can't produce. Cowbell/
                    // clap/tom/cymbal/maracas are untouched, still on
                    // their existing systems. Fase 82: the one extra slot
                    // (rhythmSieveSlotIndex) reads from the generated
                    // rhythmic sieve instead of the fixed catalog --
                    // steady 16th hihat and no swing, same convention the
                    // catalog's own denser archetypes already use. Fase 85
                    // stage 1: two more generated slots, same convention
                    // (steady 16th hihat, no swing) -- generalized aksak
                    // reads groupStart/longAccent as kick/snare, generalized
                    // Euclidean reads its own onset pattern as kick and a
                    // second, independently-rotated Euclidean pattern as
                    // snare (never identical to the kick one, re-rolled if
                    // it collides).
                    bool usingSieve = (currentArchetypeIndex == rhythmSieveSlotIndex);
                    bool usingGenAksak = (currentArchetypeIndex == rhythmAksakSlotIndex);
                    bool usingGenEuclid = (currentArchetypeIndex == rhythmEuclidSlotIndex);
                    bool usingGenerated = usingSieve || usingGenAksak || usingGenEuclid;
                    const DrumArchetype *arch = usingGenerated ? nullptr : &drumArchetypes()[static_cast<size_t>(currentArchetypeIndex)];
                    const int kickReadStep = rasgarGesture.patternReadStep(
                        step, currentBarLength, 0);
                    const int snareReadStep = rasgarGesture.patternReadStep(
                        step, currentBarLength, 1);
                    const int hihatReadStep = rasgarGesture.patternReadStep(
                        step, currentBarLength, 2);
                    bool kickHere, snareHere, hihatHere;
                    double swingAmountHere;
                    if (usingSieve) {
                        kickHere = generatedRhythm.kick[static_cast<size_t>(kickReadStep)];
                        snareHere = generatedRhythm.snare[static_cast<size_t>(snareReadStep)];
                        hihatHere = generatedEuclidHihat.onset[static_cast<size_t>(hihatReadStep)];
                        swingAmountHere = 0.0;
                    } else if (usingGenAksak) {
                        kickHere = generatedAksak.groupStart[static_cast<size_t>(kickReadStep)];
                        snareHere = generatedAksak.longAccent[static_cast<size_t>(snareReadStep)];
                        hihatHere = generatedEuclidHihat.onset[static_cast<size_t>(hihatReadStep)];
                        swingAmountHere = 0.0;
                    } else if (usingGenEuclid) {
                        kickHere = generatedEuclid.onset[static_cast<size_t>(kickReadStep)];
                        snareHere = generatedEuclidSnare.onset[static_cast<size_t>(snareReadStep)];
                        hihatHere = generatedEuclidHihat.onset[static_cast<size_t>(hihatReadStep)];
                        swingAmountHere = 0.0;
                    } else {
                        kickHere = arch->kick[static_cast<size_t>(kickReadStep)];
                        snareHere = arch->snare[static_cast<size_t>(snareReadStep)];
                        hihatHere = arch->hihat[static_cast<size_t>(hihatReadStep)];
                        swingAmountHere = arch->swingAmount;
                    }
                    const auto grooveKick = grooveDialogue.hit(GrooveDialogue::Kick,
                        static_cast<unsigned>(kickReadStep));
                    const auto grooveSnare = grooveDialogue.hit(GrooveDialogue::Snare,
                        static_cast<unsigned>(snareReadStep));
                    const auto grooveHat = grooveDialogue.hit(GrooveDialogue::Hat,
                        static_cast<unsigned>(hihatReadStep));
                    if (grooveDialogue.active()) {
                        kickHere = grooveKick.onset;
                        snareHere = grooveSnare.onset;
                        hihatHere = grooveHat.onset;
                        // Substitute the old shared swing, do not stack it.
                        swingAmountHere = 0.0;
                    }
                    const auto grooveDelay = [&](const GrooveDialogue::Hit& hit) {
                        return grooveDialogue.active()
                            ? std::max(0, static_cast<int>(std::llround(
                                hit.delaySteps * static_cast<double>(rasgarStepSamples)))) : 0;
                    };
                    const int grooveKickDelay = grooveDelay(grooveKick);
                    const int grooveSnareDelay = grooveDelay(grooveSnare);
                    const int grooveHatDelay = grooveDelay(grooveHat);
                    int swingDelaySamples = (swingAmountHere > 0.0 && (step % 2) == 1)
                        ? static_cast<int>(swingAmountHere * static_cast<double>(samplesPerStep))
                        : 0;
                    const auto &drumGrooveStep = drumGroove.steps[
                        static_cast<std::size_t>(step) % TemporalGrooveImprint::kMaxSteps];
                    const int drumGrooveDelaySamples = drumGroove.enabled && !grooveDialogue.active()
                        ? std::max(0, static_cast<int>(std::lround(
                            drumGrooveStep.timingOffsetSteps
                                * static_cast<double>(samplesPerStep)
                                * drumGroove.strength)))
                        : 0;
                    if (drumGroove.enabled && !grooveDialogue.active() && !drumGrooveStep.neutral()
                        && (kickHere || snareHere || hihatHere)) {
                        ++drumGrooveApplicationCount;
                    }
                    // Fase 85 stage 2, gesture (C): the same phrase contour
                    // nudges every hit's timing a few samples early/late,
                    // on top of swing and per-hit humanization.
                    // A negative delay cannot be scheduled without lookahead.
                    // The old code wrote it directly into kickDelay/etc.; the
                    // pending-trigger loop only accepts 0 or positive values,
                    // so those hits disappeared. Keep the phrase gesture as a
                    // delay-only quantity and let the cism delay the opposing
                    // temporal field instead of silently censoring attacks.
                    int gestureDelaySamples = std::max(
                        0, static_cast<int>(gestureBias * gestureMaxSamples));
                    const double rasgarHitVelocity = rasgarGesture.velocityScale(
                        step, currentBarLength);
                    if (kickHere && voiceOn(0)) {
                        if (hitSucceeds()) {
                            const int rasgarDelay = rasgarGesture.voiceDelaySamples(
                                0, samplesPerStep);
                            kickDelay = rollHumanizeDelay(grooveDialogue.active()) + swingDelaySamples
                                      + drumGrooveDelaySamples + gestureDelaySamples
                                      + rasgarDelay + grooveKickDelay;
                            kickVelocity = static_cast<float>(hitVelocityForBar(step, currentBarLength, currentGroupSize, gestureBias, state.density) * rasgarHitVelocity);
                            if (grooveDialogue.active()) {
                                kickVelocity = std::min(1.35f,
                                    kickVelocity * static_cast<float>(grooveKick.velocityScale));
                                grooveDialogue.recordHit(GrooveDialogue::Kick, grooveKick);
                            }
                            observePercussionPlan(ExpressiveEventSource::Kick, i, step,
                                kickDelay, kickVelocity, lastHumanizationSamples,
                                swingDelaySamples, gestureDelaySamples, rasgarDelay,
                                relationalRhythmDelaySamples + grooveKickDelay,
                                static_cast<double>(samplesPerStep) / sampleRate,
                                state.density);
                        } else ++drumHitFailureCount;
                    }
                    if (snareHere && voiceOn(1)) {
                        if (hitSucceeds()) {
                            const int rasgarDelay = rasgarGesture.voiceDelaySamples(
                                1, samplesPerStep);
                            snareDelay = rollHumanizeDelay(grooveDialogue.active()) + swingDelaySamples
                                       + drumGrooveDelaySamples + gestureDelaySamples
                                       + rasgarDelay + grooveSnareDelay;
                            snareVelocity = static_cast<float>(hitVelocityForBar(step, currentBarLength, currentGroupSize, gestureBias, state.density) * rasgarHitVelocity);
                            scheduledSnareHead = true;
                            if (grooveDialogue.active()) {
                                snareVelocity *= static_cast<float>(grooveSnare.velocityScale);
                                plannedSnareAttacks = grooveSnare.attacks;
                                grooveDialogue.recordHit(GrooveDialogue::Snare, grooveSnare);
                            }
                            observePercussionPlan(ExpressiveEventSource::Snare, i, step,
                                snareDelay, snareVelocity, lastHumanizationSamples,
                                swingDelaySamples, gestureDelaySamples, rasgarDelay,
                                relationalRhythmDelaySamples + grooveSnareDelay,
                                static_cast<double>(samplesPerStep) / sampleRate,
                                state.density, grooveSnare.ghost);
                        } else ++drumHitFailureCount;
                    }
                    if (hihatHere && voiceOn(2)) {
                        if (hitSucceeds()) {
                            const int rasgarDelay = rasgarGesture.voiceDelaySamples(
                                2, samplesPerStep);
                            hihatDelay = rollHumanizeDelay(grooveDialogue.active()) + swingDelaySamples
                                       + drumGrooveDelaySamples + gestureDelaySamples
                                       + rasgarDelay + grooveHatDelay;
                            hihatDelayedOpen = hihatBranch.next();
                            hihatVelocity = static_cast<float>(hitVelocityForBar(step, currentBarLength, currentGroupSize, gestureBias, state.density) * rasgarHitVelocity);
                            scheduledHihatHead = true;
                            if (grooveDialogue.active()) {
                                hihatVelocity *= static_cast<float>(grooveHat.velocityScale);
                                plannedHihatAttacks = grooveHat.attacks;
                                grooveDialogue.recordHit(GrooveDialogue::Hat, grooveHat);
                            }
                            observePercussionPlan(ExpressiveEventSource::Hihat, i, step,
                                hihatDelay, hihatVelocity, lastHumanizationSamples,
                                swingDelaySamples, gestureDelaySamples, rasgarDelay,
                                relationalRhythmDelaySamples + grooveHatDelay,
                                static_cast<double>(samplesPerStep) / sampleRate,
                                state.density, grooveHat.ghost);
                        } else ++drumHitFailureCount;
                    }
                    if (swingDelaySamples > 0 && (kickHere || snareHere || hihatHere)) {
                        ++swingApplicationCount;
                    }
                } else {
                    int gestureDelaySamples = std::max(
                        0, static_cast<int>(gestureBias * gestureMaxSamples));
                    const double rasgarHitVelocity = rasgarGesture.velocityScale(
                        step, currentBarLength);
                    if (polyDivider.risingEdge(kickA, kickB) && voiceOn(0)) {
                        if (hitSucceeds()) {
                            const int rasgarDelay = rasgarGesture.voiceDelaySamples(
                                0, samplesPerStep);
                            kickDelay = rollHumanizeDelay() + gestureDelaySamples
                                      + rasgarDelay;
                            kickVelocity = static_cast<float>(hitVelocityForBar(step, currentBarLength, currentGroupSize, gestureBias, state.density) * rasgarHitVelocity);
                            observePercussionPlan(ExpressiveEventSource::Kick, i, step,
                                kickDelay, kickVelocity, lastHumanizationSamples, 0,
                                gestureDelaySamples, rasgarDelay,
                                relationalRhythmDelaySamples,
                                static_cast<double>(samplesPerStep) / sampleRate,
                                state.density);
                        } else ++drumHitFailureCount;
                    }
                    if (polyDivider.risingEdge(snareA, snareB) && voiceOn(1)) {
                        if (hitSucceeds()) {
                            const int rasgarDelay = rasgarGesture.voiceDelaySamples(
                                1, samplesPerStep);
                            snareDelay = rollHumanizeDelay() + gestureDelaySamples
                                       + rasgarDelay;
                            snareVelocity = static_cast<float>(hitVelocityForBar(step, currentBarLength, currentGroupSize, gestureBias, state.density) * rasgarHitVelocity);
                            observePercussionPlan(ExpressiveEventSource::Snare, i, step,
                                snareDelay, snareVelocity, lastHumanizationSamples, 0,
                                gestureDelaySamples, rasgarDelay,
                                relationalRhythmDelaySamples,
                                static_cast<double>(samplesPerStep) / sampleRate,
                                state.density);
                        } else ++drumHitFailureCount;
                    }
                    if (polyDivider.risingEdge(hihatA, hihatB) && voiceOn(2)) {
                        if (hitSucceeds()) {
                            const int rasgarDelay = rasgarGesture.voiceDelaySamples(
                                2, samplesPerStep);
                            hihatDelay = rollHumanizeDelay() + gestureDelaySamples
                                       + rasgarDelay;
                            hihatDelayedOpen = hihatBranch.next();
                            hihatVelocity = static_cast<float>(hitVelocityForBar(step, currentBarLength, currentGroupSize, gestureBias, state.density) * rasgarHitVelocity);
                            observePercussionPlan(ExpressiveEventSource::Hihat, i, step,
                                hihatDelay, hihatVelocity, lastHumanizationSamples, 0,
                                gestureDelaySamples, rasgarDelay,
                                relationalRhythmDelaySamples,
                                static_cast<double>(samplesPerStep) / sampleRate,
                                state.density);
                        } else ++drumHitFailureCount;
                    }
                }
                // Fase 85 stage 2, repetition with difference (E): the one
                // ghost hihat armed for this bar, only if the main pattern
                // (whichever branch above just ran) didn't already put a
                // real hit there -- an addition, never a replacement.
                if (ghostHihatArmedThisBar && step == ghostHihatStep && hihatDelay < 0 && voiceOn(2)) {
                    if (hitSucceeds()) {
                        hihatDelay = rollHumanizeDelay();
                        hihatDelayedOpen = hihatBranch.next();
                        scheduledHihatHead = true;
                        hihatVelocity = static_cast<float>(hitVelocityForBar(step, currentBarLength, currentGroupSize, gestureBias, state.density)
                            * rasgarGesture.velocityScale(step, currentBarLength) * 0.55);
                        observePercussionPlan(ExpressiveEventSource::Hihat, i, step,
                            hihatDelay, hihatVelocity, lastHumanizationSamples, 0,
                            0, 0, relationalRhythmDelaySamples,
                            static_cast<double>(samplesPerStep) / sampleRate,
                            state.density, true, false);
                        ++ghostHihatCount;
                    }
                    ghostHihatArmedThisBar = false;
                }
                const size_t fixedPercussionStep = static_cast<size_t>(step)
                    % generatedCymbalPattern.onset.size();
                const size_t cowbellStep = euclideanPattern.empty() ? 0
                    : static_cast<size_t>(step) % euclideanPattern.size();
                bool cowbellPatternHere = euclideanCowbellEnabled
                    ? euclideanPattern[cowbellStep]
                    : polyDivider.risingEdge(cowbellA, cowbellB);
                const double rasgarPercVelocity = rasgarGesture.velocityScale(
                    step, currentBarLength);
                if (cowbellPatternHere && voiceOn(3)) {
                    if (hitSucceeds()) {
                        const int rasgarDelay = rasgarGesture.voiceDelaySamples(
                            3, samplesPerStep);
                        cowbellDelay = rollHumanizeDelay()
                                     + rasgarDelay;
                        cowbellVelocity = static_cast<float>(hitVelocityForBar(step, currentBarLength, currentGroupSize, gestureBias, state.density) * rasgarPercVelocity);
                        observePercussionPlan(ExpressiveEventSource::Cowbell, i, step,
                            cowbellDelay, cowbellVelocity, lastHumanizationSamples,
                            0, 0, rasgarDelay, relationalRhythmDelaySamples,
                            static_cast<double>(samplesPerStep) / sampleRate,
                            state.density);
                    } else ++drumHitFailureCount;
                }
                if (polyDivider.risingEdge(clapA, clapB) && voiceOn(4)) {
                    if (hitSucceeds()) {
                        const int rasgarDelay = rasgarGesture.voiceDelaySamples(
                            4, samplesPerStep);
                        clapDelay = rollHumanizeDelay()
                                  + rasgarDelay;
                        clapVelocity = static_cast<float>(hitVelocityForBar(step, currentBarLength, currentGroupSize, gestureBias, state.density) * rasgarPercVelocity);
                        observePercussionPlan(ExpressiveEventSource::Clap, i, step,
                            clapDelay, clapVelocity, lastHumanizationSamples,
                            0, 0, rasgarDelay, relationalRhythmDelaySamples,
                            static_cast<double>(samplesPerStep) / sampleRate,
                            state.density);
                    } else ++drumHitFailureCount;
                }
                if (polyDivider.risingEdge(tomA, tomB) && voiceOn(5)) {
                    if (hitSucceeds()) {
                        const size_t tomIndex = static_cast<size_t>(tomCycle);
                        const int rasgarDelay = rasgarGesture.voiceDelaySamples(
                            5, samplesPerStep);
                        tomDelay[tomIndex] = rollHumanizeDelay() + rasgarDelay;
                        tomVelocity[tomIndex] = static_cast<float>(hitVelocityForBar(step, currentBarLength, currentGroupSize, gestureBias, state.density) * rasgarPercVelocity);
                        observePercussionPlan(ExpressiveEventSource::Tom, i, step,
                            tomDelay[tomIndex], tomVelocity[tomIndex],
                            lastHumanizationSamples, 0, 0, rasgarDelay,
                            relationalRhythmDelaySamples,
                            static_cast<double>(samplesPerStep) / sampleRate,
                            state.density);
                        tomCycle = (tomCycle + 1) % 3;
                    } else {
                        ++drumHitFailureCount;
                    }
                }
                // Fase 85 stage 3 -- cymbal/maracas now read the dedicated
                // generalized-Euclidean patterns generated once per piece
                // in stage 1's own machinery (see the declarations above),
                // instead of the chaos zero-crossing check that used to
                // live here (moved out entirely, not left dormant).
                if (generatedCymbalPattern.onset[fixedPercussionStep] && voiceOn(6)) {
                    if (hitSucceeds()) {
                        const int rasgarDelay = rasgarGesture.voiceDelaySamples(
                            6, samplesPerStep);
                        cymbalDelay = rollHumanizeDelay()
                                    + rasgarDelay;
                        cymbalVelocity = static_cast<float>(hitVelocityForBar(step, currentBarLength, currentGroupSize, gestureBias, state.density) * rasgarPercVelocity);
                        observePercussionPlan(ExpressiveEventSource::Cymbal, i, step,
                            cymbalDelay, cymbalVelocity, lastHumanizationSamples,
                            0, 0, rasgarDelay, relationalRhythmDelaySamples,
                            static_cast<double>(samplesPerStep) / sampleRate,
                            state.density);
                    } else ++drumHitFailureCount;
                }
                if (generatedMaracasPattern.onset[fixedPercussionStep] && voiceOn(7)) {
                    if (hitSucceeds()) {
                        const int rasgarDelay = rasgarGesture.voiceDelaySamples(
                            7, samplesPerStep);
                        maracasDelay = rollHumanizeDelay()
                                     + rasgarDelay;
                        maracasVelocity = static_cast<float>(hitVelocityForBar(step, currentBarLength, currentGroupSize, gestureBias, state.density) * rasgarPercVelocity);
                        observePercussionPlan(ExpressiveEventSource::Maracas, i, step,
                            maracasDelay, maracasVelocity, lastHumanizationSamples,
                            0, 0, rasgarDelay, relationalRhythmDelaySamples,
                            static_cast<double>(samplesPerStep) / sampleRate,
                            state.density);
                    } else ++drumHitFailureCount;
                }

                // Phase 168 A/B gate: apply once, at the scheduling boundary,
                // before the pending-trigger countdown begins. Disabled mode
                // is a strict no-op; moving this out of the sample loop avoids
                // repeatedly learning the same pending hit.
                if (cfg.expressiveActuationEnabled) {
                    if (kickDelay >= 0) {
                        applyExpressivePercussion(RasgarTimingVoice::Kick,
                                                  kickVelocity, kickDelay, step,
                                                  state.resonance, state.density, samplesPerStep);
                    }
                    if (snareDelay >= 0) {
                        applyExpressivePercussion(RasgarTimingVoice::Snare,
                                                  snareVelocity, snareDelay, step,
                                                  state.resonance, state.density, samplesPerStep);
                    }
                    if (hihatDelay >= 0) {
                        applyExpressivePercussion(RasgarTimingVoice::Hihat,
                                                  hihatVelocity, hihatDelay, step,
                                                  state.resonance, state.density, samplesPerStep);
                    }
                    if (cowbellDelay >= 0) {
                        applyExpressivePercussion(RasgarTimingVoice::Cowbell,
                                                  cowbellVelocity, cowbellDelay, step,
                                                  state.resonance, state.density, samplesPerStep);
                    }
                }

                // Capture the final delay AFTER expressive scheduling. Keep
                // subdivisions inside this actual (possibly Rasgar-scaled)
                // step, at least 6ms apart. Delayed heads that leave no room
                // remain single attacks rather than leaking into the next step.
                if (grooveDialogue.subdivisionsEnabled()) {
                    const int windowSamples = static_cast<int>(std::min<std::size_t>(
                        rasgarStepSamples, static_cast<std::size_t>(INT32_MAX)));
                    const int minimumSpacing = std::max(1, static_cast<int>(std::ceil(sampleRate * .006)));
                    if (scheduledSnareHead)
                        snareRepeat.arm(plannedSnareAttacks, windowSamples, snareDelay, minimumSpacing);
                    if (scheduledHihatHead)
                        hihatRepeat.arm(plannedHihatAttacks, windowSamples, hihatDelay, minimumSpacing);
                }

                // Fase 94 -- acid bassline: deliberately kept on its own
                // fixed 16-step cycle regardless of the melodic sequencer's
                // own bar length (fase 89's meter change resizes THAT), the
                // same "honest polymeter" convention cowbell/clap/tom/
                // cymbal/maracas already use.
                if (acidBasslineEnabled && !bassBehaviourEnabled) {
                    size_t acidStep = static_cast<size_t>(step % 16);
                    if (acidActive[acidStep]) {
                        acidBass.trigger(midiToFreq(acidMidiNote[acidStep]), acidAccent[acidStep], acidSlide[acidStep]);
                        if (acidAccent[acidStep]) ++acidBasslineAccentCount;
                        if (acidSlide[acidStep]) ++acidBasslineSlideCount;
                        ExpressiveMask acidCauses = expressiveBit(ExpressiveCause::Grid);
                        if (acidAccent[acidStep]) acidCauses |= expressiveBit(ExpressiveCause::MetricAccent);
                        if (acidSlide[acidStep]) acidCauses |= expressiveBit(ExpressiveCause::TimbreVariation);
                        observeExpressiveEvent(
                            ExpressiveEventSource::Acid,
                            ExpressiveScale::Step,
                            i,
                            step,
                            acidMidiNote[acidStep],
                            acidAccent[acidStep] ? 1.0 : 0.68,
                            0.18,
                            state.density,
                            state.resonance,
                            noiseClimate,
                            expressiveBit(ExpressiveDomain::Timing)
                                | expressiveBit(ExpressiveDomain::Energy)
                                | expressiveBit(ExpressiveDomain::Pitch)
                                | expressiveBit(ExpressiveDomain::Material),
                            acidCauses,
                            acidSlide[acidStep] ? 1.0 : 0.0);
                    }
                }

                if (bassBehaviourEnabled && bassBodyPlan.valid()) {
                    bassKickCapture[bassStep] = kickDelay >= 0;
                    const auto &directive = bassBodyPlan.steps[bassStep];
                    const bool alreadyScheduledEarly =
                        bassStepScheduledEarly[bassStep];
                    bassStepScheduledEarly[bassStep] = false;
                    if (directive.active && !alreadyScheduledEarly) {
                        bassPendingDirective = directive;
                        if (directive.timingOffsetSteps < 0.0) {
                            // Only the very first cycle, or a plan change at
                            // the cycle seam, can arrive too late for
                            // lookahead. Keep those events audible and report
                            // the bounded clamp honestly.
                            ++bassEarlyTimingClampCount;
                            bassPendingDelaySamples = 0;
                        } else {
                            bassPendingDelaySamples = std::max(
                                0, static_cast<int>(std::llround(
                                    directive.timingOffsetSteps
                                    * static_cast<double>(rasgarStepSamples))));
                        }
                    }

                    // Negative microtiming must be scheduled from the
                    // previous step. This second fixed slot supplies that
                    // lookahead without allocation, lock or callback I/O.
                    const std::size_t nextBassStep =
                        (bassStep + 1u) % bassBodyPlan.stepCount;
                    const auto &nextDirective = bassBodyPlan.steps[nextBassStep];
                    if (nextDirective.active
                        && nextDirective.timingOffsetSteps < 0.0
                        && !bassStepScheduledEarly[nextBassStep]) {
                        bassEarlyPendingDirective = nextDirective;
                        bassEarlyPendingDelaySamples = std::max(
                            0, static_cast<int>(std::llround(
                                (1.0 + nextDirective.timingOffsetSteps)
                                * static_cast<double>(rasgarStepSamples))));
                        bassStepScheduledEarly[nextBassStep] = true;
                    }
                }

                // Fase 96 -- "nota de rodape": arms once, at most, at a
                // weak metrical position -- a small per-qualifying-step
                // chance rather than the very first weak step, so it can
                // land at a plausible random point across the piece
                // instead of always right at the start.
                if (footnoteEnabled && !footnoteFired) {
                    double footnoteStrength = metricalStrengthForBar(step, currentBarLength, currentGroupSize);
                    if (footnoteStrength <= 0.35) {
                        double footnoteRoll = unit(rng);
                        if (footnoteRoll < 0.03) {
                            footnoteFired = true;
                            footnoteArmed = true;
                            footnoteNotesRemaining = 2 + static_cast<int>(unit(rng) * 4.0);
                            footnoteNoteGapCountdown = 0;
                        } else if (footnoteRoll < 0.09 && apocryphalNearMisses.size() < 6) {
                            apocryphalNearMisses.push_back({"uma Nota de Rodape", footnoteRoll - 0.03});
                        }
                    }
                }

                // Fase 98 -- scat: a real chord tone every qualifying
                // step, not a fixed pattern -- reads as an improvised
                // vocal run over the changes rather than a written line.
                if (scatEnabled && unit(rng) < scatFireChancePerStep) {
                    auto scatChordOffsets = sequencer.currentChordOffsets();
                    int scatDegreeIdx = static_cast<int>(unit(rng) * 4.0) % 4;
                    int scatMidiNote = sequencer.rootMidiNote() + scatChordOffsets[static_cast<size_t>(scatDegreeIdx)] + 12;
                    int scatVowelIdx = static_cast<int>(unit(rng) * 5.0) % 5;
                    double scatDuration = 0.15 + unit(rng) * 0.25;
                    scat.beginSyllable(midiToFreq(scatMidiNote), scatDuration, scatVowelIdx);
                    ++scatEventCount;
                    observeExpressiveEvent(
                        ExpressiveEventSource::Scat,
                        ExpressiveScale::Microevent,
                        i,
                        step,
                        scatMidiNote,
                        0.62 + state.density * 0.28,
                        scatDuration,
                        state.density,
                        state.resonance,
                        noiseClimate,
                        expressiveBit(ExpressiveDomain::Energy)
                            | expressiveBit(ExpressiveDomain::Duration)
                            | expressiveBit(ExpressiveDomain::Gesture)
                            | expressiveBit(ExpressiveDomain::Pitch)
                            | expressiveBit(ExpressiveDomain::Material),
                        expressiveBit(ExpressiveCause::Phrase)
                            | expressiveBit(ExpressiveCause::Density)
                            | expressiveBit(ExpressiveCause::Relation),
                        static_cast<double>(scatVowelIdx) / 4.0);
                }

                // Fase 107 -- sequencer wild timing: a real zero-crossing
                // of the chaotic value (not just "value > threshold",
                // which would just split steps into two fixed classes)
                // triggers a genuine regeneration event -- irregular by
                // construction, gated by a real minimum gap so it can
                // never fire every step.
                if (sequencerWildTimingEnabled) {
                    ++sequencerRegenStepsSinceLast;
                    float chaosVal = sequencerRegenChaos.tick();
                    bool crossedZero = sequencerRegenChaosHasPrev
                        && ((sequencerRegenChaosPrevValue < 0.0f) != (chaosVal < 0.0f));
                    sequencerRegenChaosPrevValue = chaosVal;
                    sequencerRegenChaosHasPrev = true;
                    if (crossedZero && sequencerRegenStepsSinceLast >= sequencerRegenMinGapSteps) {
                        sequencer.regenerate(sequencerRegenDensity, sequencerRegenOctaveBias);
                        sequencerRegenStepsSinceLast = 0;
                        ++sequencerWildRegenCount;
                        observeExpressiveEvent(
                            ExpressiveEventSource::Structural,
                            ExpressiveScale::PatternCycle,
                            i,
                            -1,
                            sequencerRegenDensity,
                            0.0,
                            state.density,
                            state.resonance,
                            noiseClimate,
                            expressiveBit(ExpressiveDomain::Structure)
                                | expressiveBit(ExpressiveDomain::Timing),
                            expressiveBit(ExpressiveCause::Glitch),
                            0.0,
                            0.0,
                            false,
                            false);
                    }
                }

                // Fase 109 -- Atari Punk Console wild clock: same real
                // zero-crossing event, own independent chaotic map, only
                // arms a new glitch if the previous one has already
                // finished (glitches never stack/extend indefinitely).
                if (atariPunkGlitchEnabled && atariPunkGlitchSamplesRemaining == 0) {
                    float glitchChaosVal = atariPunkGlitchChaos.tick();
                    bool glitchCrossedZero = atariPunkGlitchHasPrev
                        && ((atariPunkGlitchPrevValue < 0.0f) != (glitchChaosVal < 0.0f));
                    atariPunkGlitchPrevValue = glitchChaosVal;
                    atariPunkGlitchHasPrev = true;
                    if (glitchCrossedZero) {
                        int ratioIdx = static_cast<int>(unit(rng) * 5.0) % 5;
                        atariPunkGlitchMult = kAtariPunkGlitchRatios[ratioIdx];
                        atariPunkGlitchSamplesRemaining = static_cast<size_t>((0.05 + unit(rng) * 0.25) * sampleRate);
                        ++atariPunkGlitchCount;
                        observeExpressiveEvent(
                            ExpressiveEventSource::Structural,
                            ExpressiveScale::Microevent,
                            i,
                            -1,
                            1.0,
                            static_cast<double>(atariPunkGlitchSamplesRemaining) / sampleRate,
                            state.density,
                            state.resonance,
                            noiseClimate,
                            expressiveBit(ExpressiveDomain::Structure)
                                | expressiveBit(ExpressiveDomain::Timing),
                            expressiveBit(ExpressiveCause::Glitch),
                            0.0,
                            0.0,
                            false,
                            false);
                    }
                }

                int melodyReadStep = step;
                if (chaosStepAddressEnabled) {
                    float addressUnit = (addressChaos.tick() + 1.0f) * 0.5f; // 0..1
                    melodyReadStep = std::min(sequencer.numSteps() - 1,
                                               static_cast<int>(addressUnit * sequencer.numSteps()));
                    if (melodyReadStep != step) ++chaosStepAddressCount;
                }
                if (sequencer.gateAt(melodyReadStep) && musicalDirection.mayStart(OrganismStem::Core)) {
                    currentMidiNote = sequencer.noteAt(melodyReadStep);
                    if (cfg.musicalDirectionEnabled && (directedMelodyNoteIndex % 3u != 2u)) {
                        const auto& table = scaleTable(sequencer.scale());
                        const int degree = musicalDirection.motifDegree(directedMelodyNoteIndex, table.length);
                        currentMidiNote = std::clamp(sequencer.rootMidiNote()
                            + table.degrees[static_cast<std::size_t>(degree)]
                            + MusicalDirection::melodicRegisterOffset(state.registerOffset), 36, 96);
                    }
                    if (cfg.musicalDirectionEnabled) ++directedMelodyNoteIndex;
                    if (!melodicPitchTelemetryInitialized) {
                        melodicPitchTelemetryInitialized = true;
                        melodicCurrentRepeatedSteps = 1;
                        melodicPitchMin = melodicPitchMax = currentMidiNote;
                    } else if (currentMidiNote == melodicPreviousTelemetryNote) {
                        ++melodicCurrentRepeatedSteps;
                        ++melodicRepeatedPitchCount;
                    } else {
                        ++melodicPitchChangeCount;
                        melodicCurrentRepeatedSteps = 1;
                    }
                    melodicLongestRepeatedSteps = std::max(
                        melodicLongestRepeatedSteps, melodicCurrentRepeatedSteps);
                    melodicPitchMin = std::min(melodicPitchMin, currentMidiNote);
                    melodicPitchMax = std::max(melodicPitchMax, currentMidiNote);
                    melodicPreviousTelemetryNote = currentMidiNote;
                    if (coherenceCollapseEnabled) coherenceCollapse.pushNote(currentMidiNote);
                    observeExpressiveEvent(
                        ExpressiveEventSource::CoreSequence,
                        ExpressiveScale::Step,
                        i,
                        melodyReadStep,
                        currentMidiNote,
                        std::clamp(0.45 + 0.55 * state.density, 0.0, 1.0),
                        static_cast<double>(samplesPerStep) / sampleRate,
                        state.density,
                        state.resonance,
                        noiseClimate,
                        expressiveBit(ExpressiveDomain::Energy)
                            | expressiveBit(ExpressiveDomain::Duration)
                            | expressiveBit(ExpressiveDomain::Gesture)
                            | expressiveBit(ExpressiveDomain::Pitch)
                            | expressiveBit(ExpressiveDomain::Material),
                        expressiveBit(ExpressiveCause::Grid)
                            | expressiveBit(ExpressiveCause::MetricAccent)
                            | expressiveBit(ExpressiveCause::Density)
                            | expressiveBit(ExpressiveCause::Phrase)
                            | (noiseClimate > 0.5f
                                ? expressiveBit(ExpressiveCause::NoiseField) : 0u),
                        noiseClimate);
                    if (agonEnabled) {
                        double agonDelta = currentMidiNote - agonPitchMean;
                        agonPitchMean += 0.08 * agonDelta;
                        agonPitchVar += 0.08 * (agonDelta * agonDelta - agonPitchVar);
                    }
                    ampEnv.noteOn();
                    // An octave down from the voice's note, so the struck
                    // body reads as something underneath it, not a copy.
                    resonator.setFundamental(midiToFreq(currentMidiNote) * 0.5);
                    resonator.excite(0.9f);

                    // AY8910Chip's three tone channels re-tune to a simple
                    // root/third/fifth-ish triad above the new note (a
                    // fixed +7/+10 semitone spread rather than a real scale
                    // lookup -- close enough for a background arpeggio
                    // texture, not meant to read as harmonically "correct"
                    // the way the main voice's scale-aware notes are).
                    chiptune.setToneFrequency(0, midiToFreq(currentMidiNote + 12));
                    chiptune.setToneFrequency(1, midiToFreq(currentMidiNote + 12 + 7));
                    // LinCongMap: its own described character ("closer to
                    // an old PRNG than a smooth attractor") makes it the
                    // right source for an occasional glitchy pitch nudge
                    // on the chiptune layer's third voice -- a rare event,
                    // not continuous modulation, matching its digital
                    // rather than analog-drift flavour.
                    int lincongGlitch = (lincongVal > 0.7f) ? 5 : 0;
                    chiptune.setToneFrequency(2, midiToFreq(currentMidiNote + 12 + 10 + lincongGlitch));

                    if (hocketCascadeEnabled) {
                        // Shift register: whatever hocketOsc1 was playing
                        // moves down into hocketOsc2 (its envelope carries
                        // over rather than resetting -- it keeps decaying
                        // from wherever it already was), then hocketOsc1
                        // takes the newest note at full strength. Every
                        // gated note does this, not just every third one.
                        hocketOsc2.setFrequency(cascadeFreq1);
                        hocketEnv2 = std::max(hocketEnv2, hocketEnv1);
                        cascadeFreq1 = midiToFreq(currentMidiNote + hocketOctaveShift1 * 12);
                        hocketOsc1.setFrequency(cascadeFreq1);
                        hocketEnv1 = 1.0f;
                        ++hocketCount;
                    } else if (hocketEnabled) {
                        // Hocketing: every third gated note, hand it to one
                        // of the two extra oscillators instead of leaving
                        // every note to read as coming from the same single
                        // voice.
                        int turn = hocketCycle % 3;
                        ++hocketCycle;
                        if (turn == 1) {
                            hocketOsc1.setFrequency(midiToFreq(currentMidiNote + hocketOctaveShift1 * 12));
                            hocketEnv1 = 1.0f;
                            ++hocketCount;
                        } else if (turn == 2) {
                            hocketOsc2.setFrequency(midiToFreq(currentMidiNote + hocketOctaveShift2 * 12));
                            hocketEnv2 = 1.0f;
                            ++hocketCount;
                        }
                    }
                }
            }
        }

        // Release the melodic voice exactly when the coda begins -- there's
        // no fixed lookahead to guess anymore since the ending's timing is
        // itself part of what the walk decides.
        if (arc.inCoda() && !codaNoteOffSent) {
            ampEnv.noteOff();
            codaNoteOffSent = true;
            if (codaRitardandoEnabled) {
                ritardandoArmed = true;
                ritardandoStartBpm = currentBpm;
                ritardandoStartSeconds = arc.elapsedSeconds();
            }
        }

        // Accelerando/stringendo: arms once, the instant the climax is
        // first reached -- the same one-time-transition pattern as the
        // ritardando's own arming just above, at the opposite end of the
        // arc.
        if (arc.inPeak() && !peakEnteredOnce) {
            peakEnteredOnce = true;
            if (accelerandoEnabled) {
                accelerandoArmed = true;
                accelerandoStartBpm = currentBpm;
                accelerandoStartSeconds = arc.elapsedSeconds();
            }
        }

        if (agonEnabled) {
            float agonTarget = (agonConflictSamplesRemaining > 0) ? 1.0f : 0.0f;
            float agonCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (agonConflictTauSeconds * sampleRate)));
            agonConflictEnv += agonCoeff * (agonTarget - agonConflictEnv);
            float agonBoost = 1.0f + agonConflictEnv * 0.6f;
            // Not a flat cut for every non-winner -- each voice cedes in
            // proportion to how far its own solidified power actually sits
            // below the winner's. A close second barely moves; a distant
            // fourth cedes close to the old full 0.5. Redistribution, not
            // a binary winner/loser split.
            float agonCutMain = 1.0f - agonConflictEnv * 0.5f * static_cast<float>(1.0 - agonRelativePower[0]);
            float agonCutAtari = 1.0f - agonConflictEnv * 0.5f * static_cast<float>(1.0 - agonRelativePower[1]);
            float agonCutResonator = 1.0f - agonConflictEnv * 0.5f * static_cast<float>(1.0 - agonRelativePower[2]);
            float agonCutTuba = 1.0f - agonConflictEnv * 0.5f * static_cast<float>(1.0 - agonRelativePower[3]);
            agonMainMult = (agonConflictWinner == 0) ? agonBoost : agonCutMain;
            agonAtariMult = (agonConflictWinner == 1) ? agonBoost : agonCutAtari;
            agonResonatorMult = (agonConflictWinner == 2) ? agonBoost : agonCutResonator;
            agonTubaMult = (agonConflictWinner == 3) ? agonBoost : agonCutTuba;
        }

        if (estranhoEnabled) {
            float estranhoTarget = (estranhoSamplesRemaining > 0) ? 1.0f : 0.0f;
            // Fast, ~15ms -- arrives and leaves quickly, not a gentle ease.
            float estranhoCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (0.015 * sampleRate)));
            estranhoGateEnv += estranhoCoeff * (estranhoTarget - estranhoGateEnv);
        }

        if (fixedOverflowEnabled) {
            float fixedOverflowTarget = (fixedOverflowSamplesRemaining > 0) ? 1.0f : 0.0f;
            float fixedOverflowCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (0.015 * sampleRate)));
            fixedOverflowGateEnv += fixedOverflowCoeff * (fixedOverflowTarget - fixedOverflowGateEnv);
        }

        if (spectralGhostEnabled) {
            float spectralGhostTarget = (spectralGhostSamplesRemaining > 0) ? 1.0f : 0.0f;
            // A slow ~300ms fade, unlike the other estranho-family events'
            // fast snap -- a sustained spectral texture reads as a ghost
            // drifting in, not a click switching on.
            float spectralGhostCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (0.3 * sampleRate)));
            spectralGhostGateEnv += spectralGhostCoeff * (spectralGhostTarget - spectralGhostGateEnv);
        }

        if (sampleHoldGlitchEnabled) {
            float sampleHoldGlitchTarget = (sampleHoldGlitchSamplesRemaining > 0) ? 1.0f : 0.0f;
            float sampleHoldGlitchCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (0.015 * sampleRate)));
            sampleHoldGlitchGateEnv += sampleHoldGlitchCoeff * (sampleHoldGlitchTarget - sampleHoldGlitchGateEnv);
        }

        double thermalGain = 1.0;
        float thermalStaticL = 0.0f, thermalStaticR = 0.0f;
        if (thermalFadeEnabled && arc.inCoda()) {
            ++thermalSamplesIntoCoda;
            double tCoda = static_cast<double>(thermalSamplesIntoCoda) / sampleRate;
            thermalGain = std::exp(-tCoda / thermalFadeTauSeconds);
            // See thermalAbruptEndReached above the main loop -- the
            // abrupt variant actually ends the render here instead of
            // silently riding out the coda's own unrelated length. A real
            // floor on tCoda keeps this from firing in the first instant
            // (tau can be as low as 0.15s -- without a floor this could
            // trigger almost immediately, before the abrupt death even
            // reads as a real gesture rather than an instant cut).
            if (thermalAbruptDeath && tCoda > 0.2 && thermalGain < 0.01) {
                thermalAbruptEndReached = true;
            }
            // Static swells as the signal itself fades, then dies with it
            // -- a radio losing the station, not a hiss left running.
            double staticEnvelope = 4.0 * thermalGain * (1.0 - thermalGain) * thermalStaticDepth;
            thermalStaticL = static_cast<float>(thermalUnit(thermalNoiseRng) * staticEnvelope);
            thermalStaticR = static_cast<float>(thermalUnit(thermalNoiseRng) * staticEnvelope);
        }

        float henonVal = henon.tick();
        if (selfFeedbackEnabled) {
            // Louder recent output speeds the Lorenz map up (faster pitch
            // drift); quieter recent output lets it settle back down --
            // read before this sample's own output exists yet, one sample
            // behind, same as any real electronic feedback loop's
            // propagation delay.
            lorenz.setRate(lorenzBaseRate + outputFeedbackEnergy * 90.0);
        }
        float lorenzVal = lorenz.tick();
        float latoVal = lato.tick();
        auto lfoOut = lfoBank.tick();
        lastLfoOut1 = lfoOut[1]; // fase 85 stage 2 gesture source, read at the next bar boundary

        // The five previously-dormant Chaos Cookbook maps, each ticked
        // once here and applied at their own target below.
        float standardVal = standardChaos.tick();
        float cuspVal = cuspChaos.tick();

        float muteGateTarget = 1.0f;
        if (stochasticMuteEnabled) {
            ++samplesSinceLastMute;
            if (muteRemaining > 0) {
                --muteRemaining;
                muteGateTarget = 0.0f;
            } else if (samplesSinceLastMute > muteMinGapSamples
                       && muteUnit(muteRng) < mutePerSampleProbability) {
                muteRemaining = muteLenMinSamples + static_cast<int>(muteUnit(muteRng) * (muteLenMaxSamples - muteLenMinSamples));
                samplesSinceLastMute = 0;
                muteGateTarget = 0.0f;
                // Atropelo: the exact instant a stochastic-mute silence
                // window opens is precisely what it targets -- an
                // intrusion into a moment this engine elsewhere promises
                // as rest, not a peaceful coexistence with it. Forces a
                // real event regardless of the usual per-bar chance.
                if (atropeloEnabled) {
                    atropelo.triggerEvent(midiToFreq(currentMidiNote) * 0.5, 3 + static_cast<int>(unit(rng) * 5.0), 0.4);
                    ++atropeloSilenceInvasionCount;
                }
            }
        }
        muteGateSmoothed += muteSlewCoeff * (muteGateTarget - muteGateSmoothed);
        float quadVal = quadChaos.tick();
        lincongVal = lincongChaos.tick();
        float fbsineVal = fbsineChaos.tick();

        // Cymbal/maracas used to trigger off henonVal/latoVal zero-crossings
        // here -- fase 85 stage 3 moved both onto the generalized-Euclidean
        // patterns generated once per piece, checked in the same
        // step-boundary block as every other percussion voice above
        // (kept the two chosen k-ranges' real sparse/dense distinction:
        // cymbal a rare accent, maracas riding close to the subdivision).

        // Pending-trigger check: runs every sample (not just at a boundary)
        // so a delay of a few samples can fire on the exact right one.
        if (kickDelay == 0) {
            if (cfg.expressiveActuationEnabled) {
                kick.setTuning(kickBaseHz * std::pow(2.0, expressiveKickPitchSemitones / 12.0),
                               kickSweepHz);
                expressiveKickGateWindow.arm(samplesPerStep, expressiveKickGate);
            }
            const double depth = chaosContagionEnabled
                ? chaosContagionDepth
                : (percussionTimbreVariationEnabled ? percussionTimbreDepth : 0.0);
            if (depth > 0.0) {
                double j = 1.0 + (latoVal + percussionSectionHold * 0.35) * depth;
                j *= expressiveKickDecayScale;
                kick.setDecayTimes(std::clamp(kickPitchDecayBase * j, 0.02, 0.4),
                                   std::clamp(kickAmpDecayBase * j, 0.05, 0.8));
                kick.setAttackCurve(std::clamp(
                    kickAttackCurveBase + standardVal * depth * 0.7, 0.55, 1.8));
            }
            if (cfg.expressiveActuationEnabled && depth <= 0.0) {
                kick.setDecayTimes(std::clamp(kickPitchDecayBase * expressiveKickDecayScale, 0.02, 0.4),
                                   std::clamp(kickAmpDecayBase * expressiveKickDecayScale, 0.05, 0.8));
            }
            if (cfg.expressiveActuationEnabled) {
                kick.setAttackCurve(std::clamp(
                    kickAttackCurveBase + (expressiveKickTimbreAmount - 0.5) * 0.3,
                    0.55, 1.8));
            }
            if (cfg.musicalDirectionEnabled) {
                const auto kit = directedKitInterpretation(0, kickVelocity);
                kick.setDecayTimes(std::clamp(kickPitchDecayBase * kit.decay * expressiveKickDecayScale, 0.02, 0.40),
                    std::clamp(kickAmpDecayBase * kit.decay, 0.05, 0.80));
                kick.setAttackCurve(std::clamp(kickAttackCurveBase * kit.attack, 0.55, 1.8));
            }
            kick.trigger(); kickDelay = -1;
            if (drumsEnabled && selectionTrace.enabled(StudioModule::Kick)
                && drumsPresence > 0 && kickGain > 0
                && materialAgency.gain(AgencyFamily::Rhythm) > 0
                && directedGain(OrganismStem::Drums) > 1e-8)
                bassKickInteraction.kick(kickVelocity);
            expressiveKickDecayScale = 1.0;
            if (asphyxiaEnabled) { asphyxiaEnv = 1.0f - static_cast<float>(asphyxiaDepth); ++asphyxiaCount; }
        } else if (kickDelay > 0) --kickDelay;
        bassKickInteraction.tick();
        if (snareDelay == 0) {
            if (snareRepeat.beginTrigger(snareVelocity))
                grooveDialogue.recordSubdivisionTrigger(GrooveDialogue::Snare);
            if (cfg.expressiveActuationEnabled) {
                snare.setTone(snareToneBaseHz
                              * std::pow(2.0, expressiveSnarePitchSemitones / 12.0));
                expressiveSnareGateWindow.arm(samplesPerStep, expressiveSnareGate);
            }
            const double depth = chaosContagionEnabled
                ? chaosContagionDepth
                : (percussionTimbreVariationEnabled ? percussionTimbreDepth : 0.0);
            if (depth > 0.0) {
                double j = 1.0 + standardVal * depth;
                j *= expressiveSnareDecayScale;
                snare.setDecayTimes(std::clamp(snareToneDecayBase * j, 0.02, 0.4),
                                    std::clamp(snareNoiseDecayBase * j, 0.03, 0.5));
                snare.setNoiseFilter(
                    std::clamp(snareNoiseCutoffBase * (1.0 + standardVal * depth * 0.8),
                               900.0, 4200.0),
                    std::clamp(snareNoiseResonanceBase + latoVal * depth * 0.35,
                               0.05, 0.75));
                snare.setAttackCurve(std::clamp(
                    snareAttackCurveBase + (quadVal + percussionSectionHold * 0.35)
                        * depth * 0.8, 0.55, 1.8));
                snareBodyHitGain = std::clamp(
                    1.0f + static_cast<float>(latoVal * depth * 0.45), 0.85f, 1.15f);
                snareNoiseHitGain = std::clamp(
                    1.0f + static_cast<float>(standardVal * depth * 0.65), 0.80f, 1.20f);
            }
            if (cfg.expressiveActuationEnabled && depth <= 0.0) {
                snare.setDecayTimes(std::clamp(snareToneDecayBase * expressiveSnareDecayScale, 0.02, 0.4),
                                    std::clamp(snareNoiseDecayBase * expressiveSnareDecayScale, 0.03, 0.5));
            }
            if (cfg.expressiveActuationEnabled) {
                snare.setNoiseFilter(
                    std::clamp(snareNoiseCutoffBase
                        * (1.0 + (expressiveSnareTimbreAmount - 0.5) * 0.35),
                        900.0, 4200.0),
                    std::clamp(snareNoiseResonanceBase
                        + (expressiveSnareTimbreAmount - 0.5) * 0.08,
                        0.05, 0.75));
            }
            if (cfg.musicalDirectionEnabled) {
                const auto kit = directedKitInterpretation(1, snareVelocity);
                snare.setDecayTimes(std::clamp(snareToneDecayBase * kit.decay * expressiveSnareDecayScale, 0.02, 0.40),
                    std::clamp(snareNoiseDecayBase * kit.decay, 0.03, 0.50));
                snare.setNoiseFilter(std::clamp(snareNoiseCutoffBase * kit.brightness, 700.0, 4200.0),
                    std::clamp(snareNoiseResonanceBase, 0.05, 0.65));
                snare.setAttackCurve(std::clamp(snareAttackCurveBase * kit.attack, 0.55, 1.8));
                snare.setComponentGains(snareBodyComponentGain * static_cast<float>(kit.body),
                    snareNoiseComponentGain * static_cast<float>(kit.noise));
            }
            snare.trigger(); snareDelay = snareRepeat.nextDelay();
            expressiveSnareDecayScale = 1.0;
        } else if (snareDelay > 0) --snareDelay;
        if (hihatDelay == 0) {
            if (hihatRepeat.beginTrigger(hihatVelocity)) {
                hihatDelayedOpen = false; // repeated closed hits, not open-hat wash
                grooveDialogue.recordSubdivisionTrigger(GrooveDialogue::Hat);
            }
            if (cfg.expressiveActuationEnabled) {
                hihat.setFundamental(hihatFundamentalBaseHz
                                     * std::pow(2.0, expressiveHihatPitchSemitones / 12.0));
                expressiveHihatGateWindow.arm(samplesPerStep, expressiveHihatGate);
            }
            const double depth = chaosContagionEnabled
                ? chaosContagionDepth
                : (percussionTimbreVariationEnabled ? percussionTimbreDepth : 0.0);
            if (depth > 0.0) {
                // Own chaos source (quadVal) -- kick already uses lato,
                // snare already uses standard; a third voice moving in
                // lockstep with either would read as one thing breathing,
                // not three independent instruments each with their own
                // small instability.
                double j = 1.0 + quadVal * depth;
                j *= expressiveHihatDecayScale;
                hihat.setDecayTimes(std::clamp(hihatDecay1Base * j, 0.01, 0.1),
                                    std::clamp(hihatDecay2Base * j, 0.08, 0.7));
                hihat.setFilter(
                    std::clamp(hihatCutoffBase * (1.0 + standardVal * depth * 0.9),
                               3200.0, 12000.0),
                    std::clamp(hihatResonanceBase + quadVal * depth * 0.3,
                               0.02, 0.6));
            }
            if (cfg.expressiveActuationEnabled && depth <= 0.0) {
                hihat.setDecayTimes(std::clamp(hihatDecay1Base * expressiveHihatDecayScale, 0.01, 0.1),
                                    std::clamp(hihatDecay2Base * expressiveHihatDecayScale, 0.08, 0.7));
            }
            if (cfg.expressiveActuationEnabled) {
                hihat.setFilter(
                    std::clamp(hihatCutoffBase
                        * (1.0 + (expressiveHihatTimbreAmount - 0.5) * 0.45),
                        3200.0, 12000.0),
                    std::clamp(hihatResonanceBase
                        + (expressiveHihatTimbreAmount - 0.5) * 0.08,
                        0.02, 0.6));
            }
            if (cfg.musicalDirectionEnabled) {
                const auto kit = directedKitInterpretation(2, hihatVelocity);
                hihat.setDecayTimes(std::clamp(hihatDecay1Base * kit.decay * expressiveHihatDecayScale, 0.01, 0.10),
                    std::clamp(hihatDecay2Base * kit.decay, 0.08, 0.65));
                hihat.setFilter(std::clamp(hihatCutoffBase * kit.brightness
                    * (musicalDirection.plan().highFlash ? 1.0 : 0.72), 2800.0, 10000.0),
                    std::clamp(hihatResonanceBase, 0.02, 0.45));
            }
            hihat.trigger(hihatDelayedOpen); hihatDelay = hihatRepeat.nextDelay();
            expressiveHihatDecayScale = 1.0;
        } else if (hihatDelay > 0) --hihatDelay;
        if (cowbellDelay == 0) {
            if (cfg.expressiveActuationEnabled) {
                cowbell.setFundamental(cowbellBaseFreq
                                       * std::pow(2.0, expressiveCowbellPitchSemitones / 12.0));
                expressiveCowbellGateWindow.arm(samplesPerStep, expressiveCowbellGate);
            }
            if (percussionTimbreVariationEnabled) {
                cowbell.setRatio(std::clamp(
                    cowbellRatioBase * (1.0 + latoVal * percussionTimbreDepth * 0.5),
                    1.15, 1.9));
                cowbell.setDecay(std::clamp(
                    cowbellDecayBase * expressiveCowbellDecayScale
                        * (1.0 + standardVal * percussionTimbreDepth),
                    0.08, 0.65));
                    cowbell.setResonance(std::clamp(
                    cowbellResonanceBase + quadVal * percussionTimbreDepth * 0.45,
                    0.15, 0.9));
            }
            if (cfg.expressiveActuationEnabled && !percussionTimbreVariationEnabled) {
                cowbell.setDecay(std::clamp(cowbellDecayBase * expressiveCowbellDecayScale,
                                            0.08, 0.65));
            }
            if (cfg.expressiveActuationEnabled) {
                cowbell.setRatio(std::clamp(
                    cowbellRatioBase
                        * (1.0 + (expressiveCowbellTimbreAmount - 0.5) * 0.12),
                    1.15, 1.9));
                cowbell.setResonance(std::clamp(
                    cowbellResonanceBase
                        + (expressiveCowbellTimbreAmount - 0.5) * 0.10,
                    0.15, 0.9));
            }
            if (cfg.musicalDirectionEnabled) {
                const auto kit = directedKitInterpretation(3, cowbellVelocity);
                cowbell.setDecay(std::clamp(cowbellDecayBase * kit.decay * expressiveCowbellDecayScale, 0.08, 0.65));
                cowbell.setRatio(std::clamp(cowbellRatioBase + (musicalRelations.enabled()
                    ? musicalRelations.kitMaterial(3) : musicalDirection.plan().material) * 0.20, 1.15, 1.90));
            }
            cowbell.trigger(); cowbellDelay = -1;
            expressiveCowbellDecayScale = 1.0;
        } else if (cowbellDelay > 0) --cowbellDelay;
        if (clapDelay == 0) {
            if (percussionTimbreVariationEnabled) {
                const double burstJitter = 1.0 + (standardVal + percussionSectionHold * 0.35)
                    * percussionTimbreDepth * 0.9;
                const double gapJitter = 1.0 + (quadVal + percussionSectionHold * 0.35)
                    * percussionTimbreDepth;
                clap.setDecay(std::clamp(clapDecayBase * burstJitter, 0.06, 0.65));
                clap.setFilter(
                    std::clamp(clapCutoffBase * (1.0 + latoVal * percussionTimbreDepth),
                               800.0, 3000.0),
                    std::clamp(clapResonanceBase + standardVal * percussionTimbreDepth * 0.3,
                               0.05, 0.75));
                clap.setBurstTiming(
                    std::clamp(clapBurstBase * burstJitter, 0.0025, 0.014),
                    std::clamp(clapGapBase * gapJitter, 0.003, 0.018));
            }
            if (cfg.musicalDirectionEnabled) {
                const auto kit = directedKitInterpretation(4, clapVelocity);
                clap.setDecay(std::clamp(clapDecayBase * kit.decay, 0.06, 0.65));
                clap.setFilter(std::clamp(clapCutoffBase * kit.brightness, 700.0, 3000.0), clapResonanceBase);
            }
            clap.trigger(); clapDelay = -1;
        } else if (clapDelay > 0) --clapDelay;
        if (cymbalDelay == 0) {
            // Decay is chosen fresh for every single hit (not left as
            // whatever the previous hit set) -- a real player doesn't
            // choke every cymbal the same way. Most hits land on the
            // piece's own base decay; a rare one rings out 2-4.5x longer.
            // No "restore" needed afterward -- the next hit picks its own
            // value here again regardless of what this one used.
            if (cymbalSustainEnabled && unit(rng) < 0.12) {
                cymbal.setDecay(cymbalDecayBase * (2.0 + unit(rng) * 2.5));
                ++cymbalSustainCount;
            } else {
                cymbal.setDecay(cymbalDecayBase);
            }
            if (cfg.musicalDirectionEnabled) {
                const auto kit = directedKitInterpretation(6, cymbalVelocity);
                cymbal.setDecay(std::clamp(cymbalDecayBase * kit.decay, 0.06,
                    musicalDirection.plan().highFlash ? 4.5 : 1.20));
                cymbal.setFilter(std::clamp(cymbalCutoffBase * kit.brightness
                    * (musicalDirection.plan().highFlash ? 1.0 : 0.70), 2500.0, 10000.0), cymbalResonanceBase);
            }
            cymbal.trigger();
            cymbalDelay = -1;
        } else if (cymbalDelay > 0) --cymbalDelay;
        if (maracasDelay == 0) {
            if (percussionTimbreVariationEnabled) {
                maracas.setDecay(std::clamp(
                    maracasDecayBase * (1.0 + quadVal * percussionTimbreDepth),
                    0.02, 0.16));
                maracas.setFilter(
                    std::clamp(maracasCutoffBase * (1.0 + standardVal * percussionTimbreDepth),
                               2400.0, 7600.0),
                    std::clamp(maracasResonanceBase + latoVal * percussionTimbreDepth * 0.25,
                               0.02, 0.5));
            }
            if (cfg.musicalDirectionEnabled) {
                const auto kit = directedKitInterpretation(7, maracasVelocity);
                maracas.setDecay(std::clamp(maracasDecayBase * kit.decay, 0.02, 0.16));
                maracas.setFilter(std::clamp(maracasCutoffBase * kit.brightness, 2000.0, 7000.0), maracasResonanceBase);
            }
            maracas.trigger(); maracasDelay = -1;
        } else if (maracasDelay > 0) --maracasDelay;
        for (int t = 0; t < 3; ++t) {
            size_t ti = static_cast<size_t>(t);
            if (tomDelay[ti] == 0) {
                if (percussionTimbreVariationEnabled) {
                    toms[ti].setAttackCurve(std::clamp(
                        tomAttackCurveBase[ti] + (latoVal + percussionSectionHold * 0.35)
                            * percussionTimbreDepth * 0.7,
                        0.55, 1.8));
                }
                toms[ti].trigger(); tomDelay[ti] = -1;
                if (tomRatchetEnabled && unit(rng) < 0.15) {
                    tomRatchetRemaining[ti] = 1 + static_cast<int>(unit(rng) * 3.0);
                    tomRatchetCountdown[ti] = tomRatchetGapSamples;
                    ++tomRatchetCount;
                }
            } else if (tomDelay[ti] > 0) --tomDelay[ti];

            if (tomRatchetRemaining[ti] > 0) {
                if (tomRatchetCountdown[ti] <= 0) {
                    if (percussionTimbreVariationEnabled) {
                        toms[ti].setAttackCurve(std::clamp(
                            tomAttackCurveBase[ti] + (quadVal + percussionSectionHold * 0.35)
                                * percussionTimbreDepth * 0.7,
                            0.55, 1.8));
                    }
                    toms[ti].trigger();
                    observePercussionPlan(ExpressiveEventSource::Tom, i,
                        globalStep, 0, tomVelocity[ti], 0, 0, 0, 0, 0,
                        static_cast<double>(tomRatchetGapSamples) / sampleRate,
                        state.density, false, true);
                    --tomRatchetRemaining[ti];
                    tomRatchetCountdown[ti] = tomRatchetGapSamples;
                } else {
                    --tomRatchetCountdown[ti];
                }
            }
        }

        if (bassBehaviourEnabled) {
            if (bassPendingDelaySamples == 0 && bassPendingDirective.active) {
                triggerBassDirective(bassPendingDirective);
                bassPendingDirective = BassBodyDirective{};
                bassPendingDelaySamples = -1;
            } else if (bassPendingDelaySamples > 0) {
                --bassPendingDelaySamples;
            }

            if (bassEarlyPendingDelaySamples == 0
                && bassEarlyPendingDirective.active) {
                triggerBassDirective(bassEarlyPendingDirective);
                bassEarlyPendingDirective = BassBodyDirective{};
                bassEarlyPendingDelaySamples = -1;
            } else if (bassEarlyPendingDelaySamples > 0) {
                --bassEarlyPendingDelaySamples;
            }

            if (bassTubaGateSamples > 0) {
                --bassTubaGateSamples;
                if (bassTubaGateSamples == 0) tubaBreath.setGate(false);
            }
            if (bassCoreSubGateSamples > 0) {
                --bassCoreSubGateSamples;
                if (bassCoreSubGateSamples == 0) bassCoreSubEnvelope.noteOff();
            }
        }

        // Fase 111 -- phasing's own trigger, on its own clock
        // (phaseStepBoundary), completely independent of stepBoundary
        // above: reads the SAME live sequencer.noteAt()/gateAt() content
        // the main voice just read (or will read) at its own step
        // boundary, so the two voices are always playing the identical
        // figure -- only the moment each one reads a given step differs,
        // which is the entire phasing effect. Read-only: never calls
        // sequencer.regenerate()/advanceChord(), so this can't itself
        // change what either voice hears, only when it hears it.
        if (phasingEnabled && phaseStepBoundary) {
            phaseNextStepBoundary = i + phaseSamplesPerStep;
            int pStep = static_cast<int>(phaseStepCounter % static_cast<size_t>(sequencer.numSteps()));
            ++phaseStepCounter;
            if (pStep != phaseGlobalStep) {
                phaseGlobalStep = pStep;
                if (sequencer.gateAt(pStep)) {
                    phaseVoice.setFrequency(midiToFreq(sequencer.noteAt(pStep)));
                    phaseVoiceEnv = 1.0f;
                    ++phasingEventCount;
                }
            }
        }

        // Fase 115 -- fechamento real da janela de expectativa do sinal
        // de blefe: so aqui, ao expirar, e que o mundo "descobre" se o
        // sinal era genuino -- nenhum outro mecanismo teve acesso a essa
        // informacao antes deste momento (pendingIsGenuine() so e lido
        // aqui, nunca no disparo). Se genuino, o gesto real de
        // confirmacao soa; se blefe, silencio -- a consequencia audivel
        // real da mentira.
        if (bluffPendingActive) {
            if (bluffResolveCountdownSamples == 0) {
                if (bluffSignal.pendingIsGenuine()) bluffConfirmEnv.noteOn();
                bluffSignal.resolveSignal(bluffEmaAlpha, bluffBetrayalThreshold);
                bluffPendingActive = false;
            } else {
                --bluffResolveCountdownSamples;
            }
        }

        // lfoOut[0] -> osc frequency (kosmische's osc1/osc2_freq_bus target):
        // a slow drift layered on top of the existing Lorenz-driven one.
        // freeIntonationEnabled adds a third, much wider term: the note
        // itself is still chosen from the scale, but the pitch actually
        // heard can wander several semitones away from it -- a real,
        // audible departure from "in scale," not a subtle detune.
        double runglerVal = runglerEnabled ? rungler.next() : 0.0;
        double pitchDriftSemitones = lorenzVal * 1.5 + lfoOut[0] * 0.3
                                    + (freeIntonationEnabled ? standardVal * freeIntonationDepth : 0.0)
                                    + runglerVal * runglerDepth;
        double baseFreq = midiToFreq(currentMidiNote);
        if (microtonalEnabled) {
            baseFreq = microtonalTuning.retune(baseFreq, currentMidiNote, sequencer.rootMidiNote());
        }
        // Cross-FM terms use *last* sample's output (this sample's own
        // hasn't been generated yet) -- a one-sample propagation delay,
        // same causality-respecting lag as the other feedback loop above.
        double hocketNudgeOsc1 = (hocket1ToOsc1 ? lastHocket1Out * hocketInfluenceDepth : 0.0)
                               + (hocket2ToOsc1 ? lastHocket2Out * hocketInfluenceDepth : 0.0);
        double hocketNudgeOsc2 = (hocket1ToOsc2 ? lastHocket1Out * hocketInfluenceDepth : 0.0)
                               + (hocket2ToOsc2 ? lastHocket2Out * hocketInfluenceDepth : 0.0);
        osc1.setFrequency(baseFreq * std::pow(2.0, (pitchDriftSemitones + lastOsc2Out * crossFmDepth + hocketNudgeOsc1) / 12.0));
        osc2.setFrequency(baseFreq * coreOsc2Ratio * std::pow(2.0, (-pitchDriftSemitones + lastOsc1Out * crossFmDepth + hocketNudgeOsc2) / 12.0));
        if (walshOscEnabled) walshOsc.setFrequency(baseFreq);
        if (vosimEnabled) vosim.setFrequency(baseFreq);
        if (hardSyncEnabled) {
            hardSync.setMasterFrequency(baseFreq);
            double ratio = hardSyncRatioBase + henonVal * hardSyncRatioDepth;
            hardSync.setSlaveFrequency(baseFreq * std::max(0.25, ratio));
        }
        if (thereminEnabled) {
            const double thereminTarget = relationalOrchestraActive
                ? midiToFreq(static_cast<int>(std::round(
                    relationalOrchestra.currentThereminTargetMidi())))
                : baseFreq;
            theremin.setTargetFrequency(thereminTarget);
        }
        if (binauralEnabled) {
            // latoVal is already ticking every sample for the main voice's
            // own pitch drift above -- reused here so the beat rate
            // itself speeds up and slows down over time (the "acelera e
            // desacelera" the research described), not a fixed offset.
            double offsetNow = binauralOffsetHz * (1.0 + latoVal * 0.5);
            binauralL.setFrequency(baseFreq);
            binauralR.setFrequency(baseFreq + offsetNow);
        }
        // lfoOut[1] -> osc2 width (osc2_width_bus target).
        osc2.setShape(std::clamp(0.5f + 0.45f * static_cast<float>(latoVal) + 0.08f * lfoOut[1], 0.0f, 1.0f));
        if (cfg.musicalDirectionEnabled)
            osc2.setShape(static_cast<float>(std::clamp(0.5 + musicalDirection.material() * 0.48
                + latoVal * 0.18, 0.0, 1.0)));

        float osc1Out = osc1.tick();
        float osc2Out = osc2.tick();
        lastOsc1Out = osc1Out;
        lastOsc2Out = osc2Out;
        float voice = voiceDiodeMixer.process(osc1Out * coreOsc1Gain, osc2Out * coreOsc2Gain, coreDiodeDrive);

        // Character colour applied right at the voice, before the
        // resonator/ring-mod/filter chain -- so everything downstream
        // reacts to the coloured signal the way it would in a real analog
        // chain, not as an afterthought tacked on at the very end. Blended
        // modestly (35%) rather than fully replacing the voice, so it
        // colours the piece without overriding the diode mix's own
        // character.
        if (voiceColor == VoiceColor::Tape) {
            voice = voice * 0.65f + tapeSat.process(voice) * 0.35f;
        } else if (voiceColor == VoiceColor::Tube) {
            voice = voice * 0.65f + tubeSat.process(voice) * 0.35f;
        } else if (voiceColor == VoiceColor::Fracture) {
            voice = voice * 0.65f + static_cast<float>(fractureFold.process(voice)) * 0.35f;
        }

        // The modal resonator rings on its own between strikes; how much
        // of it shows through the mix follows the same resonance-driven
        // curve as the ring modulator below, barely present in a quiet
        // opening and prominent at the climax.
        float modalOut = resonator.process();

        // StandardMap (Chirikov): "kick strength" k tied to the arc's own
        // resonance, so the ring carrier's drift genuinely gets more
        // chaotic (not just louder) as a section intensifies -- near-
        // quasi-periodic in a quiet opening, properly chaotic at a climax.
        double kickStrength = 0.3 + std::clamp((state.resonance - 0.8) / 2.2, 0.0, 1.0) * 2.2;
        standardChaos.setParams(kickStrength);
        ringCarrier.setFrequency(65.0 + standardVal * 12.0);
        float ringAmount = static_cast<float>(std::clamp((state.resonance - 1.0) / 1.6, 0.0, 1.0));
        voice = voice + modalOut * (0.08f + 0.18f * ringAmount);
        float ringed = ringMod.process(voice, ringCarrier.tick() * 1.8f);
        voice = voice * (1.0f - ringAmount) + ringed * ringAmount;

        if (leibnizEnabled) {
            float leibnizAmount = static_cast<float>(std::clamp((state.resonance - 0.6) / 2.0, 0.0, 1.0)) * static_cast<float>(leibnizMix);
            float corrupted = static_cast<float>(leibnizRect.process(voice));
            voice = voice * (1.0f - leibnizAmount) + corrupted * leibnizAmount;
        }

        if (rosslerEnabled) {
            rossler.tick();
            if (rossler.justErupted() && state.density < 0.45) rosslerKickEnv = 1.0f;
            rosslerKickEnv *= rosslerKickDecayCoeff;
        }

        // lfoOut[2] -> filter cutoff (cutoff_bus target).
        double cutoff = 300.0 + state.filterOpenness * 6500.0 +
                        henonVal / 1.5 * 400.0 * state.filterOpenness +
                        lfoOut[2] * 350.0 * state.filterOpenness +
                        rosslerKickEnv * rosslerKickDepthHz;
        if (cfg.musicalDirectionEnabled)
            cutoff = std::clamp(cutoff * (0.85 + musicalDirection.material() * 0.38), 100.0, 11000.0);
        if (asphyxiaEnabled) {
            // Recovers toward 1.0 (fully open) every sample regardless of
            // whether a kick just fired -- the trigger above only ever
            // snaps it DOWN, this is what brings it back up afterward.
            asphyxiaEnv += (1.0f - asphyxiaEnv) * (1.0f - asphyxiaRecoveryCoeff);
            cutoff *= asphyxiaEnv;
        }
        filter.setCutoff(std::clamp(cutoff, 60.0, sampleRate * 0.45));
        filter.setResonance(state.resonance);

        float env = ampEnv.tick();
        float voiceEnv = voice * env;
        float filtered = filter.process(voiceEnv) * agonMainMult;

        // Crossfade in a little of the second filter colour -- same
        // cutoff data as the ladder above so it reads as the same filter
        // drifting to a different character, not a second independent
        // filter fighting it. Capped at 0.6 of the blend so the ladder
        // stays the default voice and this is an accent.
        float gbmanVal = filterColorMap.tick();
        float filterColorBlend = std::clamp((gbmanVal + 1.5f) / 3.0f, 0.0f, 1.0f) * coreSecondFilterMax;
        float secondColourOut;
        if (polivoksEnabled) {
            // Full state.resonance range, not scaled down like the Chamberlin SVF
            // below -- Polivoks' entire point is genuine self-oscillation
            // right near the top of that range (see PolivoksFilter.hpp),
            // so a piece's own climax (state.resonance approaching 1) is
            // exactly when it should have a real chance to break loose,
            // not a tamed-down accent that can never reach that region.
            polivoksFilter.setCutoff(cutoff);
            polivoksFilter.setResonance(state.resonance);
            secondColourOut = polivoksFilter.process(voiceEnv);
            // Tracks real crossings into the genuine-self-oscillation
            // zone (see PolivoksFilter.hpp's damping formula) -- honest
            // reporting of whether this specific piece actually reached
            // it, not just "the mechanism was enabled".
            bool nowSelfOscillating = state.resonance > 0.96;
            if (nowSelfOscillating && !polivoksWasSelfOscillating) ++polivoksSelfOscillationCount;
            polivoksWasSelfOscillating = nowSelfOscillating;
        } else {
            sallenFilter.setCutoff(cutoff);
            sallenFilter.setResonance(std::clamp(state.resonance * 0.4, 0.0, 1.0));
            secondColourOut = sallenFilter.process(voiceEnv);
        }
        filtered = filtered * (1.0f - filterColorBlend) + secondColourOut * filterColorBlend;

        // One stepped-CV read per sample, shared by all four consumers below
        // (previously only 2 of the 4 correlated outputs were used).
        auto stepCV = stepRandom.tick();

        // Multiband spectral bloom, fading in with the same resonance-driven
        // curve as the ring modulator -- barely present in the opening,
        // prominent at the climax, using the two stepped outputs that used
        // to go unused to move each band's gain independently.
        bands.setBandGain(0, 0.3f + 0.7f * (stepCV[0] * 0.5f + 0.5f));
        bands.setBandGain(1, 0.3f + 0.7f * (stepCV[1] * 0.5f + 0.5f));
        bands.setBandGain(2, 0.3f + 0.7f * (stepCV[2] * 0.5f + 0.5f));
        bands.setBandGain(3, 0.3f + 0.7f * (stepCV[3] * 0.5f + 0.5f));
        float bloomed = bands.process(filtered);
        filtered = filtered * (1.0f - ringAmount * 0.5f) + bloomed * (ringAmount * 0.5f);

        // Granular mix rises as the section thins out (low density) --
        // capped modestly so it colors the texture rather than taking over.
        const float noiseTextureWeatherGain = 0.48f + 0.62f * noiseClimate;
        float grainMix = granularPresence
            * (0.02f + 0.09f * static_cast<float>(1.0 - state.density));
        granular.setMix(grainMix);
        // QuadMap: the classic logistic-family period-doubling map modulates
        // grain pitch jitter, so the granular wash's own instability has a
        // structurally different (buzzier, period-doubling-flavoured)
        // character than the smooth chaotic maps driving everything else.
        granular.setPitchJitter(5.0 + quadVal * 3.0);
        filtered = granular.process(filtered);

        filtered *= static_cast<float>(
            materialAgency.gain(AgencyFamily::Core)
            * directedGain(OrganismStem::Core));
        organismStemAudit.addPreDelay(OrganismStem::Core, filtered);
        agencyMeter.add(AgencyFamily::Core,
                        filtered * voiceMixLevel, filtered * voiceMixLevel);
        bassExecutionFeedback.beginFrame();
        auto addPreSpaceFamily = [&](AgencyFamily family, float signal,
                                     OrganismStem stem = OrganismStem::Other) {
            const float contribution = signal
                * static_cast<float>(materialAgency.gain(family)
                    * directedGain(stem));
            if (stem == OrganismStem::FmFlute)
                musicalRelations.observe(MusicalBody::Fm, contribution * voiceMixLevel);
            else if (stem == OrganismStem::Acid || stem == OrganismStem::Tuba || stem == OrganismStem::BassFallback) {
                musicalRelations.observe(MusicalBody::Bass, contribution * voiceMixLevel);
                bassExecutionFeedback.observeVoice(contribution * voiceMixLevel);
            }
            filtered += contribution;
            organismStemAudit.addPreDelay(stem, contribution);
            agencyMeter.add(family, contribution * voiceMixLevel,
                            contribution * voiceMixLevel);
        };

        // GendynOscillator ticks continuously (it's a self-mutating
        // texture, not a triggered voice) and mixes in at a small,
        // density-driven amount -- same shape as the granular mix above,
        // just a structurally different synthesis technique providing it.
        float gendynMix = gendynPresence
            * (0.015f + 0.05f * static_cast<float>(1.0 - state.density));
        addPreSpaceFamily(AgencyFamily::Texture,
            gendyn.tick() * gendynMix * agonResonatorMult,
            OrganismStem::Texture);

        // Colored noise bed: same density-inverse shape as granular/Gendyn
        // above, kept quieter still since it's a steady hiss/rumble rather
        // than a modulated texture -- a spectral flavour (pink/red/blue)
        // this engine never had outside percussion voices.
        float colorNoiseMix = coloredNoisePresence
            * (0.01f + 0.03f * static_cast<float>(1.0 - state.density));
        addPreSpaceFamily(AgencyFamily::Texture,
            colorNoise.tick() * colorNoiseMix, OrganismStem::Texture);

        if (walshOscEnabled) {
            // Same density-inverse shape as the granular/Gendyn/colorNoise
            // textures above -- more present as a section thins into
            // itself. Not multiplied by any agonXMult: this isn't one of
            // Agon's four contesting voices, same as colorNoise above it.
            float walshMix = noiseTextureWeatherGain
                * (0.02f + 0.07f * static_cast<float>(1.0 - state.density));
            addPreSpaceFamily(AgencyFamily::Texture,
                walshOsc.tick() * walshMix, OrganismStem::Texture);
        }

        if (vectorSynthEnabled) {
            // X from standardVal, Y from latoVal -- two chaos sources
            // already ticking every sample for the main voice's own pitch
            // drift above, reused read-only here (no new chaos state).
            float vx = std::clamp((standardVal + 1.0f) * 0.5f, 0.0f, 1.0f);
            float vy = std::clamp((latoVal + 1.0f) * 0.5f, 0.0f, 1.0f);
            float wGendyn = (1.0f - vx) * (1.0f - vy);
            float wWalsh  = vx * (1.0f - vy);
            float wAtari  = (1.0f - vx) * vy;
            float wNoise  = vx * vy;
            vsAtari.setClockFrequency(baseFreq * 0.5);
            float vectorMixed = vsGendyn.tick() * wGendyn
                               + vsWalsh.tick() * wWalsh
                               + vsAtari.tick() * wAtari * 0.4f
                               + vsNoise.tick() * wNoise * 0.6f;
            float vectorSynthMix = noiseTextureWeatherGain
                * (0.03f + 0.06f * static_cast<float>(1.0 - state.density));
            addPreSpaceFamily(AgencyFamily::Texture,
                vectorMixed * vectorSynthMix, OrganismStem::Texture);
        }

        if (vosimEnabled) {
            // Same density-inverse bucket as Walsh just above. VOSIM's
            // raw output is unipolar (sin^2 pips, always >= 0) -- relies
            // on the shared master DC blocker further down the chain to
            // remove the resulting bias, the same assumption already
            // documented as safe elsewhere in this engine.
            float vosimMix = noiseTextureWeatherGain
                * (0.03f + 0.08f * static_cast<float>(1.0 - state.density));
            addPreSpaceFamily(AgencyFamily::Texture,
                vosim.tick() * vosimMix, OrganismStem::Texture);
        }

        if (hardSyncEnabled) {
            // Same density-inverse bucket as Walsh/VOSIM above -- pitch
            // locked to the melody (master frequency = baseFreq), only the
            // sweepable slave/master ratio (henonVal-driven above) gives
            // this its own harsh, moving character distinct from either.
            float hardSyncMix = noiseTextureWeatherGain
                * (0.02f + 0.07f * static_cast<float>(1.0 - state.density));
            addPreSpaceFamily(AgencyFamily::Texture,
                hardSync.tick() * hardSyncMix, OrganismStem::Texture);
        }

        if (egLoopPulseEnabled) {
            // Same density-inverse background bucket as Walsh/VOSIM/hard
            // sync above -- ticked continuously regardless of section
            // density since it self-cycles on its own clock, unrelated to
            // any note/step logic here.
            if (egLoopPulse.justPulsed()) ++egLoopPulseCount;
            float egMix = noiseTextureWeatherGain
                * (0.03f + 0.07f * static_cast<float>(1.0 - state.density));
            addPreSpaceFamily(AgencyFamily::Texture,
                egLoopPulse.tick() * egMix, OrganismStem::Texture);
        }

        if (acidBasslineEnabled) {
            // A real voice, not a background texture -- same foreground
            // presence bucket as the tuba voice (fase 43), not Walsh/
            // VOSIM/hard sync's quiet background-colour bucket above.
            float acidMix = bassBehaviourEnabled
                ? static_cast<float>(0.20 + bassAcidDirective.presence * 0.36
                    + bassAcidDirective.drive * 0.10)
                : 0.25f + 0.25f * static_cast<float>(1.0 - state.density);
            addPreSpaceFamily(AgencyFamily::Instrument,
                acidBass.tick() * acidMix, OrganismStem::Acid);
        }

        if (bassBehaviourEnabled && bassCoreSubEnvelope.isActive()) {
            const float envelope = bassCoreSubEnvelope.tick();
            const float fundamental = bassCoreSubFundamental.tick();
            const float second = bassCoreSubHarmonic.tick();
            const float harmonic = second
                * static_cast<float>(bassCoreSubDirective.harmonicBody * (musicalRelations.enabled()
                    ? .45 + musicalRelations.controls(MusicalBody::Bass).material * .35 : .34));
            const auto& bassControl = musicalRelations.controls(MusicalBody::Bass);
            const float bassBody = bassPhraseTimbre.enabled()
                ? bassPhraseTimbre.process(fundamental, second,
                    bassControl.material, bassControl.articulation)
                : fundamental + harmonic;
            const float duck = bassKickInteraction.gain(bassCoreSubDirective.kickDuck);
            const float coreSub = bassBody * envelope
                * static_cast<float>(0.10 + bassCoreSubDirective.presence * 0.16)
                * duck;
            addPreSpaceFamily(AgencyFamily::Instrument, coreSub,
                              OrganismStem::BassFallback);
        }

        if (thereminEnabled) {
            // Legacy mode remains the density-inverse glide voice. In the
            // relational orchestra, pitch and temporal presence belong to
            // the theremin's own listening history. Presence can remain
            // continuous, move, vanish or return; none is mandatory.
            float thereminMix = thereminLead
                ? (0.17f + 0.20f * static_cast<float>(1.0 - state.density))
                : (0.025f + 0.07f * static_cast<float>(1.0 - state.density));
            if (relationalOrchestraActive) {
                relationalThereminPresenceSmoothed +=
                    (relationalOrchestra.currentThereminPresence()
                     - relationalThereminPresenceSmoothed)
                    * relationalThereminPresenceSmoothing;
                thereminMix *= static_cast<float>(
                    relationalOrchestra.currentThereminGainScale()
                    * relationalThereminPresenceSmoothed);
            }
            addPreSpaceFamily(AgencyFamily::Instrument,
                theremin.tick() * thereminMix, OrganismStem::Theremin);
        }

        if (fmFluteEnabled) {
            // Eixo C: a pending second event (bebop approach->target,
            // suspension hold->resolve) fires partway through the current
            // note, timed against this note's OWN real duration -- not the
            // next bar boundary, which could be 1-3 bars away.
            if (fmFluteNextEventIdx < fmFlutePendingEvents.size() && fmFluteNoteTotalSamples > 0) {
                double elapsedFraction = static_cast<double>(fmFluteNoteSamplesElapsed) / static_cast<double>(fmFluteNoteTotalSamples);
                if (elapsedFraction >= fmFlutePendingEvents[fmFluteNextEventIdx].fractionOfNoteDuration) {
                    double hz = midiToFreq(fmFluteRootForNote + fmFlutePendingEvents[fmFluteNextEventIdx].semitoneOffset);
                    fmFlute.setFrequency(hz);
                    fmFlute.noteOn();
                    ++fmFluteNextEventIdx;
                }
            }
            ++fmFluteNoteSamplesElapsed;

            // Presence bucket, not background -- this is a real melodic
            // voice with its own phrasing and harmony, same foreground
            // class as tuba/acid-bass/Atropelo, not the quiet 0.02-0.09
            // texture bucket the rest of this block's older mechanisms use.
            addPreSpaceFamily(AgencyFamily::Instrument, fmFlute.tick(),
                              OrganismStem::FmFlute);
            // Eixo B: quiet harmonic support under the flute, never
            // competing with it for foreground attention.
            addPreSpaceFamily(AgencyFamily::Polyphony, voicingBank.tick());
        }

        if (atropeloEnabled) {
            // Deliberately NOT in the same quiet 0.02-0.09 bucket as the
            // background textures above -- when this actually fires it's
            // meant to read as a real foreground interjection, not a
            // buried texture technically present in the metadata but
            // practically inaudible (the exact complaint that prompted a
            // real mixing-presence pass across the whole engine). Still
            // scales down modestly at high density so it doesn't fight
            // the climax for space, but never drops to background level.
            float atropeloMix = 0.22f + 0.15f * static_cast<float>(1.0 - state.density);
            addPreSpaceFamily(AgencyFamily::Instrument,
                atropelo.tick() * atropeloMix);
        }

        if (tubaVoiceEnabled) {
            const double behaviourBreath = bassBehaviourEnabled
                ? std::clamp(0.24 + bassTubaDirective.breath * 0.62, 0.0, 1.0)
                : tubaExciteAmount;
            float breathAmount = tubaBreath.next(static_cast<float>(behaviourBreath));
            tubaResonator.excite(breathAmount);
            // Brightness tracks the breath envelope itself, not the arc --
            // the timbre opens as this specific note's own intensity
            // builds, same articulation a real player's air pressure gives
            // a brass tone, independent of where the piece as a whole is.
            tubaResonator.setBrightness(static_cast<float>(std::clamp(
                0.2 + (bassBehaviourEnabled
                           ? 0.22 + bassTubaDirective.brightness * 0.34
                           : 0.4) * tubaBreath.envelope()
                    * relationalChamberResponse.tubaPressure, 0.1, 0.8)));
            // Soft output-stage limiter (same tanh-bounding idea already
            // used for PolivoksFilter's own real instability, fase 75):
            // continuous per-sample excitation, unlike this engine's other
            // ModalResonator uses (a single strike that rings out and goes
            // quiet before the next), never lets the resonator settle --
            // measured empirically (debug instrumentation comparing this
            // voice's own mean contribution against the piece's final
            // mixed output) to run ~20-25x hotter than the whole piece's
            // own final level, forcing the master limiter to compress
            // everything else to tame it. Tuning decay time alone wasn't
            // enough (tried three orders of magnitude, ratio barely
            // moved) -- a real ceiling independent of decay/gain tuning.
            float tubaRaw = std::tanh(tubaResonator.process() * 0.12f) * 0.9f;
            const double behaviourPresence = bassBehaviourEnabled
                ? std::clamp(0.42 + bassTubaDirective.presence * 0.58, 0.0, 1.0)
                : 1.0;
            float tubaContrib = tubaRaw * static_cast<float>(
                tubaMix * behaviourPresence) * agonTubaMult;
            addPreSpaceFamily(AgencyFamily::Instrument, tubaContrib,
                              OrganismStem::Tuba);
        }
        bassExecutionFeedback.finishFrame();

        if (galoEnabled && galo.isActive()) {
            // Real foreground presence for the ~1-2 seconds it plays,
            // same idiom as elemento estranho -- a genuine intrusion, not
            // a well-mannered background layer.
            addPreSpaceFamily(AgencyFamily::Instrument, galo.tick() * 0.6f,
                              OrganismStem::Galo);
        }

        if (scatEnabled && scat.isActive()) {
            // Real presence, learned from this same session's audibility
            // audit -- a vocal-adjacent layer should read as a genuine
            // voice, not a buried texture.
            addPreSpaceFamily(AgencyFamily::Voice,
                              scat.tick() * 0.4f
                                  * static_cast<float>(relationalChamberResponse.voiceGain),
                              OrganismStem::Scat);
        }

        if (dejaVuEnabled) {
            // Tracks the piece's own recent character continuously
            // (slow one-pole followers -- a mood, not an instant) so
            // whatever plays back on trigger is a genuine statistical
            // ghost of "how this piece has tended to sound", never a
            // captured buffer.
            dejaVu.trackMemory(midiToFreq(currentMidiNote), state.filterOpenness, state.density);
            if (dejaVu.isActive()) {
                addPreSpaceFamily(AgencyFamily::Voice, dejaVu.tick() * 0.5f,
                                  OrganismStem::DejaVu);
            }
        }

        if (choirEnabled && choir.anyActive()) {
            // Four voices summed already carry real presence on their
            // own -- a lighter per-sample multiplier than the scat solo
            // voice above, so a full choir chord doesn't overwhelm it.
            addPreSpaceFamily(AgencyFamily::Voice,
                              choir.tick() * 0.16f
                              * static_cast<float>(relationalChamberResponse.voiceGain),
                              OrganismStem::Choir);
        }

        if (fixedOverflowEnabled && fixedOverflowGateEnv > 0.0005f) {
            // Real Q1.15 wraparound (see FixedPointOverflow.hpp), crossfaded
            // in/out smoothly (fixedOverflowGateEnv) so the event itself
            // doesn't click -- the wraparound discontinuity inside the
            // event stays raw on purpose, same "smooth edges, raw interior"
            // shape as elemento estranho above.
            float overflowed = static_cast<float>(fixedOverflowProcessor.process(filtered));
            const float ruptureDelta = fixedOverflowGateEnv * (overflowed - filtered)
                * static_cast<float>(materialAgency.gain(AgencyFamily::Rupture)
                    * directedGain(OrganismStem::Rupture));
            filtered += ruptureDelta;
            organismStemAudit.addPreDelay(OrganismStem::Rupture, ruptureDelta);
            agencyMeter.add(AgencyFamily::Rupture, ruptureDelta, ruptureDelta);
        }

        if (spectralGhostEnabled) {
            // Always fed, regardless of the gate -- this has to run every
            // sample so its internal analysis buffers stay continuously
            // warm; skipping calls whenever the gate is closed would
            // desync its overlap-add bookkeeping and reintroduce the
            // startup latency gap every time it re-opens.
            float ghost = spectralGhostProcessor.process(filtered);
            if (spectralGhostGateEnv > 0.0005f) {
                const float ghostContribution = spectralGhostGateEnv * ghost
                    * static_cast<float>(materialAgency.gain(AgencyFamily::Rupture)
                        * directedGain(OrganismStem::Rupture));
                filtered += ghostContribution;
                organismStemAudit.addPreDelay(
                    OrganismStem::Rupture, ghostContribution);
                agencyMeter.add(AgencyFamily::Rupture,
                                ghostContribution, ghostContribution);
            }
        }

        if (sampleHoldGlitchEnabled && sampleHoldGlitchGateEnv > 0.0005f) {
            // Corrupts the time axis specifically -- stale, held values
            // crossfaded against the real signal, same smooth-edges/raw-
            // interior discipline as the rest of this family.
            float held = sampleHoldGlitchProcessor.process(filtered);
            const float ruptureDelta = sampleHoldGlitchGateEnv * (held - filtered)
                * static_cast<float>(materialAgency.gain(AgencyFamily::Rupture)
                    * directedGain(OrganismStem::Rupture));
            filtered += ruptureDelta;
            organismStemAudit.addPreDelay(OrganismStem::Rupture, ruptureDelta);
            agencyMeter.add(AgencyFamily::Rupture, ruptureDelta, ruptureDelta);
        }

        // Ochd: two of the eight rate-related outputs slowly breathe the
        // resonator's brightness and the reverb's damping around their base
        // values (channel 0 is the ensemble's slowest/foundational rate,
        // channel 3 a related-but-distinct faster one) -- both parameters
        // used to be set once and never touched again for the whole piece.
        auto ochdOut = ochd.tick();
        double reverbDampingL = 0.4;
        double reverbDampingR = 0.4;
        if (ochdEnabled) {
            resonator.setBrightness(0.5f + ochdOut[0] * ochdBrightnessDepth);
            reverbDampingL = 0.4 + ochdOut[3] * ochdDampingDepth;
            reverbDampingR = 0.4 + ochdOut[3] * ochdDampingDepth * 0.9;
            reverb.setDamping(reverbDampingL);
            reverbR.setDamping(reverbDampingR);
        }

        // lfoOut[3] -> delay time (delay_time_bus target). Both then pass
        // through the never-built Buchla lag processors so the per-clock
        // step lands as a glide, not a jump.
        // CuspMap: a bifurcation-flavoured wobble layered onto the delay
        // time target, on top of the Buchla-style stepped CV and LFO drift
        // already there -- a different texture of instability (a cusp
        // catastrophe's sudden-but-bounded jump) than either of those.
        const double spatialCrossing = std::clamp(topologySpaceBias, -1.0, 1.0);
        double delayTimeTarget = std::clamp(
            0.2 + (stepCV[2] * 0.5 + 0.5) * 0.5 + lfoOut[3] * 0.06
            + cuspVal * 0.04 + spatialCrossing * 0.16,
            0.05, 1.9);
        double reverbMixTarget = std::clamp(
            0.15 + (stepCV[3] * 0.5 + 0.5) * 0.35
            + std::fabs(spatialCrossing) * 0.18,
            0.05, 0.82);
        if (cfg.musicalDirectionEnabled)
            reverbMixTarget = std::clamp(reverbMixTarget * musicalDirection.space(), 0.03, 0.72);
        double delayTime = delayTimeLag.process(static_cast<float>(delayTimeTarget));
        double reverbMix = reverbMixLag.process(static_cast<float>(reverbMixTarget));
        delay.setDelayTime(delayTime);
        delayR.setDelayTime(delayTime * delayTimeRRatio);
        reverb.setMix(reverbMix);
        reverbR.setMix(reverbMix);

        // Mono up to here; L/R start diverging now (see the delayR/reverbR/
        // panLfo setup above).
        float delayedL = delay.process(filtered);
        float delayedR = delayR.process(filtered);
        organismStemAudit.closePreDelay(
            filtered, voiceMixLevel, delayMixLevel,
            delayTime, delayTime * delayTimeRRatio, delayedL, delayedR);
        float wetL = filtered * voiceMixLevel + delayedL * delayMixLevel;
        float wetR = filtered * voiceMixLevel + delayedR * delayMixLevel;

        if (binauralEnabled) {
            // Each oscillator only ever reaches its OWN channel -- that
            // asymmetry is the entire mechanism. Kept modest (0.05) so it
            // colours/destabilises the existing stereo image rather than
            // becoming a second lead tone.
            const float binauralGain = static_cast<float>(
                materialAgency.gain(AgencyFamily::Texture));
            const float binauralContributionL = binauralL.tick() * 0.05f * binauralGain;
            const float binauralContributionR = binauralR.tick() * 0.05f * binauralGain;
            wetL += binauralContributionL;
            wetR += binauralContributionR;
            organismStemAudit.addPreReverb(
                OrganismStem::Texture,
                binauralContributionL, binauralContributionR);
            agencyMeter.add(AgencyFamily::Texture,
                            binauralContributionL, binauralContributionR);
        }

        // The percussion layer joins the mix here, after the main voice's
        // own filtering but before delay/reverb -- so kick/snare/hi-hat get
        // caught in the same space as everything else instead of sitting
        // in a separate, obviously "drum machine" bus. Levels kept modest;
        // the leveler right below still has the final say on overall gain.
        // Only one tom/conga instance is ever triggered at a time (see the
        // round-robin above), but all three still need ticking every
        // sample so the untriggered ones' envelopes stay at rest instead
        // of holding stale state. Drums and the chiptune arpeggio stay
        // mono contributions (added to both channels alike) -- only the
        // delay/reverb tails actually differ between L and R.
        // FBSineMap: a feedback-sine cross-modulation between two internal
        // variables (unlike the smoother single-variable maps elsewhere)
        // gives cowbell/cymbal a small organic pitch instability of their
        // own -- real hardware cowbell/cymbal circuits drift with
        // temperature and component tolerance; this is that idea's
        // software-chaos analogue rather than a fixed, perfectly-tuned
        // fundamental every single hit.
        cowbell.setFundamental(cowbellBaseFreq * (1.0 + fbsineVal * 0.015));
        cymbal.setFundamental(cymbalBaseFreq * (1.0 + fbsineVal * 0.015));

        // Fase 85 stage 2 (D): each voice's own last-triggered velocity
        // multiplies its output here, on top of the seed-derived per-voice
        // gain above -- real per-hit dynamics riding alongside the
        // per-piece kit balance, not replacing it.
        float tomsOut = applyPercFx(tomsFx[0], toms[0].process()) * tomVelocity[0]
                      + applyPercFx(tomsFx[1], toms[1].process()) * tomVelocity[1]
                      + applyPercFx(tomsFx[2], toms[2].process()) * tomVelocity[2];
        // The kit is mixed as an arrangement, not as eight equal effects.
        // Kick and snare are the rhythmic anchors; cowbell is a pitched
        // decorative voice and must not acquire authority simply because its
        // bandpass transient is more concentrated. Keeping these stems
        // named also lets the ducking decision below listen to the anchors
        // instead of letting a bright cowbell duck the whole composition.
        auto applyExpressiveGate = [&](float signal, RasgarGateWindow &window) {
            return !cfg.expressiveActuationEnabled || window.tick() ? signal : 0.0f;
        };
        float kickDrum = applyPercFx(kickFx,
                                     applyExpressiveGate(kick.process(), expressiveKickGateWindow))
                       * 0.42f * kickGain * kickVelocity;
        const auto snareComponents = snare.processComponents();
        const float snareSignal = snareComponents.body * snareBodyHitGain
                                + snareComponents.noise * snareNoiseHitGain;
        float snareDrum = applyPercFx(snareFx,
                                      applyExpressiveGate(snareSignal, expressiveSnareGateWindow))
                        * 0.38f * snareGain * snareVelocity;
        float hihatDrum = applyPercFx(hihatFx,
                                      applyExpressiveGate(hihat.process(), expressiveHihatGateWindow))
                        * 0.16f * hihatGain * hihatVelocity;
        float cowbellDrum = applyPercFx(cowbellFx,
                                        applyExpressiveGate(cowbell.process(), expressiveCowbellGateWindow))
                          * 0.11f * cowbellGain * cowbellVelocity;
        float clapDrum = applyPercFx(clapFx, clap.process())
                       * 0.28f * clapGain * clapVelocity;
        float cymbalDrum = applyPercFx(cymbalFx, cymbal.process())
                         * 0.18f * cymbalGain * cymbalVelocity;
        float maracasDrum = applyPercFx(maracasFx, maracas.process())
                          * 0.09f * maracasGain * maracasVelocity;
        // Enforce source exclusion after local effects as well as at the
        // trigger gate: fills/delayed events must not bypass the choice.
        // Automatic changes neither the signal nor the legacy RNG stream.
        if (!selectionTrace.enabled(StudioModule::Kick)) kickDrum = 0.0f;
        if (!selectionTrace.enabled(StudioModule::Snare)) snareDrum = 0.0f;
        if (!selectionTrace.enabled(StudioModule::HiHat)) hihatDrum = 0.0f;
        if (!selectionTrace.enabled(StudioModule::Cowbell)) cowbellDrum = 0.0f;
        if (!selectionTrace.enabled(StudioModule::Clap)) clapDrum = 0.0f;
        if (!selectionTrace.enabled(StudioModule::Toms)) tomsOut = 0.0f;
        if (!selectionTrace.enabled(StudioModule::Cymbal)) cymbalDrum = 0.0f;
        if (!selectionTrace.enabled(StudioModule::Maracas)) maracasDrum = 0.0f;
        float drums = kickDrum + snareDrum + hihatDrum + cowbellDrum
                    + clapDrum + tomsOut * 0.26f * tomsGain
                    + cymbalDrum + maracasDrum;

        if (staticPopEnabled) {
            if (staticPopGate.tick()) staticPopEnv = 1.0f;
            drums += staticPopNoise.tick() * staticPopEnv * 0.10f;
            staticPopEnv *= staticPopDecay;
        }

        // Raised from the original fixed 0.22/0.22 (effectively ~0.048x
        // combined) after the user reported not being able to hear the
        // TR-808 percussion at all against the rest of the patch -- the
        // melodic voice/delay reach the mix at 0.75x/0.35x, so drums at a
        // ~5x quieter combined multiplier than that were effectively
        // buried, not a deliberate "distant percussion" choice. Now a
        // seed-derived `drumsPresence` instead of one fixed number for every
        // piece -- supporting kits now roll 0.55-0.8x, while protagonists
        // remain in their stronger range. The exact presence still varies
        // piece to piece like everything else here.
        //
        // Fase 66: that presence used to be one constant for the WHOLE
        // piece -- the kit sat at the exact same relative loudness in the
        // hushed intro as at the climax, the opposite of "the drums have
        // their own narrative". Now it rides the same arc density every
        // other texture already reacts to (granular/Gendyn/colorNoise/
        // Walsh above): receded to about a third of nominal in the
        // quietest passages, full strength at the busiest. Below density
        // 0.12 it cuts to true silence -- no separate "tacet" mechanism
        // needed, the arc's own narrative already provides the quiet
        // stretches where a real drummer would simply lay out.
        if (drumsEnabled) {
            float drumsPresenceNow = (state.density < 0.12)
                ? 0.0f
                : drumsPresence * static_cast<float>((rhythmicProtagonist ? 0.72 : 0.35)
                    + (rhythmicProtagonist ? 0.28 : 0.65) * state.density);
            drumsPresenceNow *= static_cast<float>(rasgarFrame.drumPresenceScale);
            drumsPresenceNow = std::min(drumsPresenceNow, 2.2f);
            float drumsContribution = drums * drumsPresenceNow
                * static_cast<float>(relationalRhythmVelocityScale)
                * static_cast<float>(materialAgency.gain(AgencyFamily::Rhythm)
                    * directedGain(OrganismStem::Drums));
            const float drumVoiceScale = drumsPresenceNow
                * static_cast<float>(relationalRhythmVelocityScale)
                * static_cast<float>(materialAgency.gain(AgencyFamily::Rhythm)
                    * directedGain(OrganismStem::Drums));
            const std::array<float, 8> drumVoiceContributions = {
                kickDrum * drumVoiceScale,
                snareDrum * drumVoiceScale,
                hihatDrum * drumVoiceScale,
                cowbellDrum * drumVoiceScale,
                clapDrum * drumVoiceScale,
                tomsOut * 0.26f * tomsGain * drumVoiceScale,
                cymbalDrum * drumVoiceScale,
                maracasDrum * drumVoiceScale
            };
            // Body/noise diagnostic is intentionally taken before the local
            // tape/tube/VCA colour stage: it describes the articulation that
            // the voice generated, not a post-effect attribution guess.
            const float snareSelectionScale = selectionTrace.enabled(StudioModule::Snare) ? 1.0f : 0.0f;
            const float snareBodyContribution = snareSelectionScale * snareComponents.body
                * snareBodyHitGain * 0.38f * snareGain * snareVelocity * drumVoiceScale;
            const float snareNoiseContribution = snareSelectionScale * snareComponents.noise
                * snareNoiseHitGain * 0.38f * snareGain * snareVelocity * drumVoiceScale;
            snareBodyEnergySum += static_cast<double>(snareBodyContribution) * snareBodyContribution;
            snareNoiseEnergySum += static_cast<double>(snareNoiseContribution) * snareNoiseContribution;
            snareBodyPeak = std::max(snareBodyPeak, std::fabs(snareBodyContribution));
            snareNoisePeak = std::max(snareNoisePeak, std::fabs(snareNoiseContribution));
            for (std::size_t voice = 0; voice < drumVoiceContributions.size(); ++voice) {
                drumVoiceEnergySum[voice] += static_cast<double>(drumVoiceContributions[voice])
                    * drumVoiceContributions[voice];
                drumVoicePeak[voice] = std::max(drumVoicePeak[voice],
                                                std::fabs(drumVoiceContributions[voice]));
            }
            relationalOrchestra.observeRhythmSample(drumsContribution);
            ++drumsMeasuredFrames;
            drumsEnergySum += static_cast<double>(drumsContribution) * drumsContribution;
            drumsPeak = std::max(drumsPeak, std::fabs(drumsContribution));
            if (std::fabs(drumsContribution) > 0.003f) ++drumsAudibleFrames;

            // In a rhythmic identity, transients create their own room in
            // the arrangement. This short, signal-driven duck happens only
            // while a real drum sample is present; it does not compress the
            // whole piece or alter non-rhythmic identities.
            const float drumClaim = static_cast<float>(std::clamp(
                (directedGain(OrganismStem::Drums) - 1.0) / 1.5,
                0.0, 1.0));
            if (rhythmicProtagonist || drumClaim > 0.0f) {
                const float duckCeiling = rhythmicProtagonist
                    ? 0.34f : 0.08f + 0.14f * drumClaim;
                // Only the beat anchors can claim space from the harmonic
                // field. Tonal/decorative voices remain audible, but a
                // frequent cowbell cannot repeatedly lower the entire core.
                const float anchorEnergy = std::max(
                    std::fabs(kickDrum) + std::fabs(snareDrum),
                    std::fabs(clapDrum) * 0.75f);
                float duck = std::clamp(
                    anchorEnergy * (0.62f + 0.24f * drumClaim),
                    0.0f, duckCeiling);
                wetL *= (1.0f - duck);
                wetR *= (1.0f - duck);
                organismStemAudit.scalePreReverb(1.0f - duck);
                agencyMeter.scaleFrame(1.0f - duck);
            }
            wetL += drumsContribution;
            wetR += drumsContribution;
            organismStemAudit.addPreReverb(
                OrganismStem::Drums, drumsContribution, drumsContribution);
            agencyMeter.add(AgencyFamily::Rhythm,
                            drumsContribution, drumsContribution);
            musicalRelations.observe(MusicalBody::Kit, drumsContribution);
        }

        musicalRelations.finishFrame();

        // AY8910Chip's arpeggio fades in with the same resonance-driven
        // curve as the ring modulator/multiband bloom above -- inaudible
        // in a quiet opening, a distinct chiptune-flavoured layer at the
        // climax, gone again by the close. Kept quiet enough (0.12 cap)
        // to read as a texture, not a competing lead line.
        float chiptuneOut = chiptune.tick() * (ringAmount * chiptuneCap)
            * static_cast<float>(materialAgency.gain(AgencyFamily::Texture)
                * directedGain(OrganismStem::Texture));
        wetL += chiptuneOut;
        wetR += chiptuneOut;
        organismStemAudit.addPreReverb(
            OrganismStem::Texture, chiptuneOut, chiptuneOut);
        agencyMeter.add(AgencyFamily::Texture, chiptuneOut, chiptuneOut);

        // AtariPunkConsole -- same resonance-driven fade as the ring
        // modulator/AY8910 above, a sub-octave stepped-tone glitch that's
        // essentially inaudible in a quiet opening and a distinct lo-fi
        // texture at the climax. Pulse width nudged by the Buchla-style
        // stepped source's first output for a little of its own drift.
        if (atariPunkGlitchSamplesRemaining > 0) --atariPunkGlitchSamplesRemaining;
        double atariPunkClockMult = (atariPunkGlitchEnabled && atariPunkGlitchSamplesRemaining > 0) ? atariPunkGlitchMult : 1.0;
        atariPunk.setClockFrequency(baseFreq * 0.5 * atariPunkClockMult);
        atariPunk.setPulseWidth(0.001 + (stepCV[0] * 0.5 + 0.5) * 0.006);
        float atariOut = atariPunk.tick() * (ringAmount * atariCap) * agonAtariMult
            * static_cast<float>(materialAgency.gain(AgencyFamily::Texture)
                * directedGain(OrganismStem::Texture));
        if (!atariVoiceEnabled) atariOut = 0.0f;
        if (estranhoEnabled && estranhoGateEnv > 0.0005f) {
            // Raw amplitude comparator between the main voice and this
            // texture -- whichever is louder this exact sample passes,
            // phase-modulated by the one that lost. Bounded by construction
            // (sin/cos never exceed 1, so |clash| can't exceed whichever
            // input was already loudest) but deliberately NOT smoothed
            // sample-to-sample -- that harsh discontinuity is the point,
            // not a bug to engineer away. Only entering/leaving the event
            // is smoothed (estranhoGateEnv above), so it doesn't click.
            float clash = (std::fabs(filtered) > std::fabs(atariOut))
                        ? filtered * std::sin(atariOut * 0.1f)
                        : atariOut * std::cos(filtered * 0.1f);
            float estranhoAtari = atariOut + estranhoGateEnv * (clash - atariOut);
            wetL += estranhoAtari;
            wetR += estranhoAtari;
            organismStemAudit.addPreReverb(
                OrganismStem::Texture, estranhoAtari, estranhoAtari);
            agencyMeter.add(AgencyFamily::Texture,
                            estranhoAtari, estranhoAtari);
        } else {
            wetL += atariOut;
            wetR += atariOut;
            organismStemAudit.addPreReverb(
                OrganismStem::Texture, atariOut, atariOut);
            agencyMeter.add(AgencyFamily::Texture, atariOut, atariOut);
        }

        // EnvelopeOscillator -- an octave-up AD/AR-as-VCO shimmer under
        // the main voice, its attack/release balance swept by the
        // Latoocarfian value already driving osc2's shape, so the
        // shimmer's own timbre keeps drifting rather than sitting fixed.
        // Kept very quiet (0.05 cap) -- a texture, not a second lead.
        envOsc.setFrequency(baseFreq * 2.0);
        envOsc.setShape(std::clamp(0.5f + 0.45f * static_cast<float>(latoVal), 0.02f, 0.98f));
        float envOscOut = envOsc.tick() * envOscCap
            * static_cast<float>(materialAgency.gain(AgencyFamily::Texture)
                * directedGain(OrganismStem::Texture));
        wetL += envOscOut;
        wetR += envOscOut;
        organismStemAudit.addPreReverb(
            OrganismStem::Texture, envOscOut, envOscOut);
        agencyMeter.add(AgencyFamily::Texture, envOscOut, envOscOut);

        // Harmonaig chord pad: four sine voices under independent ADSR
        // envelopes. The presence cap is deliberately below the foreground
        // melody, but not buried: the phase-122 stem audit found the former
        // 0.05 cap left this real chord organism at roughly 5% of piece
        // energy and ~80% level-buried in an ensemble seed.
        if (harmonaigEnabled) {
            float harmonaigMixed = 0.0f;
            for (size_t hv = 0; hv < 4; ++hv) {
                harmonaigMixed += harmonaigOsc[hv].tick() * harmonaigEnv[hv].tick();
            }
            const float harmonaigPresence = ensembleLead ? 0.08f : 0.065f;
            harmonaigMixed *= harmonaigPresence
                * static_cast<float>(materialAgency.gain(AgencyFamily::Polyphony)
                    * directedGain(OrganismStem::Harmonaig));
            wetL += harmonaigMixed;
            wetR += harmonaigMixed;
            organismStemAudit.addPreReverb(
                OrganismStem::Harmonaig, harmonaigMixed, harmonaigMixed);
            agencyMeter.add(AgencyFamily::Polyphony,
                            harmonaigMixed, harmonaigMixed);
        }

        // Fase 78 -- Xenakis sieve polyphony: three independently-clocked
        // elemental channels (wood/breath, metal/glass, water) each
        // picking notes from their own generated sieve, plus a reactive
        // "plasma" channel that only fires when the other three collide
        // into too consonant a combination.
        if (sievePolyphonyEnabled) {
            double woodRateNow = std::max(0.05, sieveWoodRate * (1.0 + 0.3 * lorenzVal));
            sieveWoodPhase += woodRateNow / sampleRate;
            if (sieveWoodPhase >= 1.0) {
                sieveWoodPhase -= 1.0;
                int degIdx = static_cast<int>(unit(rng) * sieveWood.table.length) % sieveWood.table.length;
                int degree = sieveWood.table.degrees[static_cast<size_t>(degIdx)];
                sieveWoodPc = mod12(degree);
                recordSievePitch(0, sequencer.rootMidiNote() - 12 + degree);
                sieveWoodBody.setFundamental(midiToFreq(sequencer.rootMidiNote() - 12 + degree));
                sieveWoodGateSamplesLeft = static_cast<int>(sampleRate * (0.5 / woodRateNow));
            }
            sieveWoodBreath.setGate(sieveWoodGateSamplesLeft > 0);
            if (sieveWoodGateSamplesLeft > 0) --sieveWoodGateSamplesLeft;
            sieveWoodBody.excite(sieveWoodBreath.next(1.2f));
            // Same soft output-stage limiter as the tuba voice's own fix
            // (see its comment above) -- continuous per-sample excitation
            // into a ModalResonator tuned with percussive-strike decay
            // times runs far hotter than a single-strike use, measured
            // empirically to force the master limiter to over-compress
            // the whole mix. Bounded here the same way, independent of
            // decay/gain tuning.
            float woodOut = std::tanh(sieveWoodBody.process() * 0.12f) * 0.9f;

            double metalRateNow = std::max(0.05, sieveMetalRate * (1.0 - 0.3 * lorenzVal));
            sieveMetalPhase += metalRateNow / sampleRate;
            bool metalStruckNow = false;
            if (sieveMetalPhase >= 1.0) {
                sieveMetalPhase -= 1.0;
                int degIdx = static_cast<int>(unit(rng) * sieveMetal.table.length) % sieveMetal.table.length;
                int degree = sieveMetal.table.degrees[static_cast<size_t>(degIdx)];
                sieveMetalPc = mod12(degree);
                recordSievePitch(1, sequencer.rootMidiNote() + degree);
                sieveMetalBody.setFundamental(midiToFreq(sequencer.rootMidiNote() + degree));
                sieveMetalBody.excite(1.0f);
                metalStruckNow = true;
            }
            float metalOut = sieveMetalBody.process();

            double waterRateNow = std::max(0.05, sieveWaterRate * (1.0 + 0.2 * lorenzVal * lorenzVal));
            sieveWaterPhase += waterRateNow / sampleRate;
            if (sieveWaterPhase >= 1.0) {
                sieveWaterPhase -= 1.0;
                int degIdx = static_cast<int>(unit(rng) * sieveWater.table.length) % sieveWater.table.length;
                int degree = sieveWater.table.degrees[static_cast<size_t>(degIdx)];
                sieveWaterPc = mod12(degree);
                recordSievePitch(2, sequencer.rootMidiNote() + 12 + degree);
                sieveWaterTone.setFrequency(midiToFreq(sequencer.rootMidiNote() + 12 + degree));
                const double timbreA = sieveWaterTimbreUnit(sieveWaterTimbreRng);
                const double timbreB = sieveWaterTimbreUnit(sieveWaterTimbreRng);
                const double timbreC = sieveWaterTimbreUnit(sieveWaterTimbreRng);
                const double grainMin = 0.014 + timbreA * 0.014;
                const double grainMax = 0.040 + timbreB * 0.050;
                const double density = 12.0 + timbreC * 18.0;
                const double durationMax = std::max(grainMin + 0.008, grainMax);
                const double pitchJitter = 4.0 + timbreB * 6.0;
                sieveWaterGrain.setDensity(density);
                sieveWaterGrain.setGrainDuration(grainMin, durationMax);
                sieveWaterGrain.setSpread(0.18 + timbreA * 0.42);
                sieveWaterGrain.setPitchJitter(pitchJitter);
                sieveWaterGrain.setMix(static_cast<float>(0.78 + timbreC * 0.20));
                if (sieveWaterArticulationChangeCount == 0) {
                    sieveWaterDensityMin = sieveWaterDensityMax = density;
                    sieveWaterGrainDurationMin = sieveWaterGrainDurationMax = grainMin;
                    sieveWaterPitchJitterMin = sieveWaterPitchJitterMax = pitchJitter;
                } else {
                    sieveWaterDensityMin = std::min(sieveWaterDensityMin, density);
                    sieveWaterDensityMax = std::max(sieveWaterDensityMax, density);
                    sieveWaterGrainDurationMin = std::min(sieveWaterGrainDurationMin, grainMin);
                    sieveWaterGrainDurationMax = std::max(sieveWaterGrainDurationMax, durationMax);
                    sieveWaterPitchJitterMin = std::min(sieveWaterPitchJitterMin, pitchJitter);
                    sieveWaterPitchJitterMax = std::max(sieveWaterPitchJitterMax, pitchJitter);
                }
                ++sieveWaterArticulationChangeCount;
            }
            float waterDry = sieveWaterTone.tick() * 0.5f;
            float waterOut = sieveWaterGrain.process(waterDry);

            // Plasma: reactive, not its own clock -- checked whenever the
            // metal channel strikes a fresh note (a natural "beat" for the
            // ensemble). "Too consonant" means every pairwise interval
            // among the three elemental channels' current pitch classes
            // is a unison, fourth or fifth (mod 12) -- the plainest
            // possible combination this sieve ensemble can produce. The
            // note played is the plasma scale's OWN closest pitch class
            // to a true tritone from the wood channel -- drawn from real
            // "rejected" material, not an arbitrary interval.
            if (metalStruckNow) {
                auto consonantInterval = [](int a, int b) {
                    int d = std::abs(a - b) % 12;
                    return d == 0 || d == 5 || d == 7;
                };
                bool tooConsonant = consonantInterval(sieveWoodPc, sieveMetalPc)
                                  && consonantInterval(sieveMetalPc, sieveWaterPc)
                                  && consonantInterval(sieveWoodPc, sieveWaterPc);
                if (tooConsonant) {
                    int tritonePc = mod12(sieveWoodPc + 6);
                    int chosenPc = mod12(sievePlasmaTable.degrees[0]);
                    int bestDist = 12;
                    for (int i = 0; i < sievePlasmaTable.length; ++i) {
                        int pc = mod12(sievePlasmaTable.degrees[static_cast<size_t>(i)]);
                        int dist = std::min(std::abs(pc - tritonePc), 12 - std::abs(pc - tritonePc));
                        if (dist < bestDist) { bestDist = dist; chosenPc = pc; }
                    }
                    sievePlasmaOsc.setFrequency(midiToFreq(sequencer.rootMidiNote() + chosenPc));
                    recordSievePitch(3, sequencer.rootMidiNote() + chosenPc);
                    sievePlasmaEnv.noteOn();
                    ++sievePolyphonyPlasmaCount;
                }
            }
            float plasmaOut = sievePlasmaOsc.tick() * sievePlasmaEnv.tick();

            float sieveMixed = (woodOut * 0.5f + metalOut * 0.4f
                              + waterOut * 0.35f + plasmaOut * 0.3f)
                * static_cast<float>(materialAgency.gain(AgencyFamily::Polyphony)
                    * directedGain(OrganismStem::Sieve));
            wetL += sieveMixed;
            wetR += sieveMixed;
            organismStemAudit.addPreReverb(
                OrganismStem::Sieve, sieveMixed, sieveMixed);
            agencyMeter.add(AgencyFamily::Polyphony, sieveMixed, sieveMixed);
        }

        // Fase 79 -- Bloom-inspired fractal melody: own independent clock,
        // own Trunk/Branch note table, own oscillator+envelope.
        if (bloomEnabled) {
            int notesInLevel = (bloomPathLevel == 0) ? 5 : 25;
            double stepSeconds = bloomLoopSeconds / notesInLevel;
            bloomPhase += 1.0 / (stepSeconds * sampleRate);
            if (bloomPhase >= 1.0) {
                bloomPhase -= 1.0;
                ++bloomStepIndex;
                bool loopCompleted = false;
                if (bloomStepIndex >= notesInLevel) {
                    bloomStepIndex = 0;
                    loopCompleted = true;
                }
                int rawSemitone = (bloomPathLevel == 0)
                    ? bloomTrunk[static_cast<size_t>(bloomStepIndex)]
                    : bloomBranch[static_cast<size_t>(bloomStepIndex)];
                // Snap to the nearest degree of the piece's own active
                // scale, keeping the fractal's raw semitone shape
                // harmonically consistent with everything else.
                const ScaleTable &bloomTable = sequencer.primaryScaleTable();
                int targetPc = mod12(sequencer.rootMidiNote() + rawSemitone);
                int bestPc = bloomTable.degrees[0];
                int bestDist = 12;
                for (int i = 0; i < bloomTable.length; ++i) {
                    int pc = mod12(sequencer.rootMidiNote() + bloomTable.degrees[static_cast<size_t>(i)]);
                    int dist = std::min(std::abs(pc - targetPc), 12 - std::abs(pc - targetPc));
                    if (dist < bestDist) { bestDist = dist; bestPc = bloomTable.degrees[static_cast<size_t>(i)]; }
                }
                int octaveOffset = (rawSemitone / 12) * 12;
                bloomOsc.setFrequency(midiToFreq(sequencer.rootMidiNote() + bestPc + octaveOffset));
                bloomEnv.noteOn();

                if (loopCompleted && unit(rng) < bloomSwitchProbability) {
                    bloomPathLevel = 1 - bloomPathLevel;
                    ++bloomLevelSwitchCount;
                }
            }
            float bloomOut = bloomOsc.tick() * bloomEnv.tick() * 0.12f
                * static_cast<float>(materialAgency.gain(AgencyFamily::Polyphony)
                    * directedGain(OrganismStem::Bloom));
            wetL += bloomOut;
            wetR += bloomOut;
            organismStemAudit.addPreReverb(
                OrganismStem::Bloom, bloomOut, bloomOut);
            agencyMeter.add(AgencyFamily::Polyphony, bloomOut, bloomOut);
        }

        // Hocketing oscillators: simple decaying-envelope voices, ticked
        // continuously but only audible right after their turn (see the
        // gated-note block above). Panned hard-ish apart from each other
        // and from the main voice, so a hocket turn genuinely reads as a
        // different instrument catching the note, not just a doubled
        // layer in the same place.
        if (hocketEnabled) {
            float h1, h2;
            if (polivoksVcaEnabled) {
                h1 = polivoksVca.processA(hocketOsc1.tick(), hocketEnv1) * 0.24f;
                h2 = polivoksVca.processB(hocketOsc2.tick(), hocketEnv2) * 0.24f;
            } else {
                h1 = hocketOsc1.tick() * hocketEnv1 * 0.24f;
                h2 = hocketOsc2.tick() * hocketEnv2 * 0.24f;
            }
            hocketEnv1 *= static_cast<float>(hocketDecay1);
            hocketEnv2 *= static_cast<float>(hocketDecay2);
            if (hocketCascadeEnabled) {
                // The cascade's whole point is that h1/h2 routinely overlap
                // now (several ghost lines actually ringing at once, not an
                // isolated alternating turn) -- combined through the same
                // diode-mixing nonlinearity the main voice uses instead of
                // a plain linear sum, so the ghost lines collide and
                // saturate against each other rather than just adding.
                float cascadeMixed = cascadeDiodeMixer.process(h1, h2, 2.5f) * 0.5f
                    * static_cast<float>(materialAgency.gain(AgencyFamily::Polyphony)
                        * directedGain(OrganismStem::Hocket));
                wetL += cascadeMixed;
                wetR += cascadeMixed;
                organismStemAudit.addPreReverb(
                    OrganismStem::Hocket, cascadeMixed, cascadeMixed);
                agencyMeter.add(AgencyFamily::Polyphony,
                                cascadeMixed, cascadeMixed);
            } else {
                const float polyphonyGain = static_cast<float>(
                    materialAgency.gain(AgencyFamily::Polyphony)
                    * directedGain(OrganismStem::Hocket));
                const float hocketL = (h1 * 1.3f + h2 * 0.5f) * polyphonyGain;
                const float hocketR = (h1 * 0.5f + h2 * 1.3f) * polyphonyGain;
                wetL += hocketL;
                wetR += hocketR;
                organismStemAudit.addPreReverb(
                    OrganismStem::Hocket, hocketL, hocketR);
                agencyMeter.add(AgencyFamily::Polyphony, hocketL, hocketR);
            }
            // One-sample propagation delay, same causality-respecting lag
            // as the osc1<->osc2 cross-FM above -- read next iteration by
            // the pitch-drift calc, not this one.
            lastHocket1Out = h1;
            lastHocket2Out = h2;
        }

        // Phasing voice: ticked continuously, audible only in the decay
        // tail after its own (independently-clocked) trigger above. Panned
        // hard opposite the main voice -- Reich's own ensemble performances
        // place the two identical instruments apart in the stereo field
        // precisely so the ear can track each line and hear the drift
        // between them, rather than hearing one blurred, denser voice.
        if (phasingEnabled) {
            float pv = phaseVoice.tick() * phaseVoiceEnv * phaseVoiceGain
                * static_cast<float>(materialAgency.gain(AgencyFamily::Polyphony)
                    * directedGain(OrganismStem::Hocket));
            phaseVoiceEnv *= static_cast<float>(phaseVoiceDecay);
            wetL += pv * 0.35f;
            wetR += pv * 1.0f;
            organismStemAudit.addPreReverb(
                OrganismStem::Hocket, pv * 0.35f, pv);
            agencyMeter.add(AgencyFamily::Polyphony, pv * 0.35f, pv);
        }

        // Objeto espectral: seis parciais somados, cada um pulsando na sua
        // propria taxa (Eixo C) numa altura que pode ja estar esticada
        // (Eixo A) e que pode estar ausente (Eixo B) -- mixado centrado,
        // sem pan proprio, porque o proprio objeto e uma unica identidade
        // sonora com varias camadas internas, nao vozes discretas que
        // precisem se distinguir espacialmente entre si.
        if (spectralObjectEnabled) {
            float so = spectralObject.tick()
                * static_cast<float>(materialAgency.gain(AgencyFamily::Texture)
                    * directedGain(OrganismStem::Spectral));
            wetL += so;
            wetR += so;
            organismStemAudit.addPreReverb(
                OrganismStem::Spectral, so, so);
            agencyMeter.add(AgencyFamily::Texture, so, so);
        }

        // Rasgo espectral: cada fibra rasgada toca uma textura de ruido
        // continua e permanente na frequencia real daquele parcial --
        // ticado sempre (envelope so avanca de verdade depois do
        // noteOn/loop real disparado no break), mixado bem quieto (e um
        // deshiar de fundo, nao um evento em primeiro plano) e centrado,
        // mesma logica do objeto espectral que ele estende.
        if (rasgoFractureEnabled) {
            float rasgoMix = 0.0f;
            for (int fi = 0; fi < RasgoFracture::kNumFibers; ++fi) {
                rasgoMix += rasgoTearOsc[static_cast<size_t>(fi)].tick() * rasgoTearEnv[static_cast<size_t>(fi)].tick();
            }
            rasgoMix *= 0.05f
                * static_cast<float>(materialAgency.gain(AgencyFamily::Rupture)
                    * directedGain(OrganismStem::Rupture));
            wetL += rasgoMix;
            wetR += rasgoMix;
            organismStemAudit.addPreReverb(
                OrganismStem::Rupture, rasgoMix, rasgoMix);
            agencyMeter.add(AgencyFamily::Rupture, rasgoMix, rasgoMix);
        }

        // Estalo do colapso por coerencia: ruido curto e brusco, o unico
        // evento em primeiro plano (nao continuo como o rasgo espectral
        // acima) -- e o proprio momento audivel da ruptura, nao um
        // resíduo dela.
        if (coherenceCollapseEnabled) {
            float crack = coherenceCrackOsc.tick() * coherenceCrackEnv.tick() * 0.5f
                * static_cast<float>(materialAgency.gain(AgencyFamily::Rupture)
                    * directedGain(OrganismStem::Rupture));
            wetL += crack;
            wetR += crack;
            organismStemAudit.addPreReverb(
                OrganismStem::Rupture, crack, crack);
            agencyMeter.add(AgencyFamily::Rupture, crack, crack);
        }

        // Ensemble polimetrico: cada uma das quatro vozes ja vem panorizada
        // internamente (registro E posicao distintos por voz, ver
        // PolymeterEnsemble::configure), entao aqui so soma o par
        // estereo que a propria classe ja calculou -- diferente do
        // objeto espectral, aqui a identidade separada de cada voz e o
        // ponto inteiro do mecanismo (contraponto de periodicidades so
        // se le como tal se cada relogio for rastreavel).
        if (polymeterEnabled) {
            float pmL = 0.0f, pmR = 0.0f;
            polymeterEnsemble.tick(&pmL, &pmR);
            const float rhythmGain = static_cast<float>(
                materialAgency.gain(AgencyFamily::Rhythm)
                * directedGain(OrganismStem::Polymeter));
            pmL *= rhythmGain;
            pmR *= rhythmGain;
            wetL += pmL;
            wetR += pmR;
            organismStemAudit.addPreReverb(
                OrganismStem::Polymeter, pmL, pmR);
            agencyMeter.add(AgencyFamily::Rhythm, pmL, pmR);
        }

        // Sinal de blefe: as tres vozes tocadas centradas (o proprio
        // conteudo -- se o gesto de confirmacao soa ou nao, e o quanto a
        // respondente responde -- e o que carrega significado aqui, nao a
        // posicao espacial). O sinal em si soa IDENTICO genuino ou blefe
        // (mesmo oscilador, mesma frequencia, mesmo envelope, disparado
        // do mesmo jeito) -- informacao genuinamente incompleta no
        // momento do disparo.
        if (bluffSignalEnabled) {
            float sig = bluffSignalOsc.tick() * bluffSignalEnv.tick() * 0.16f;
            float conf = bluffConfirmOsc.tick() * bluffConfirmEnv.tick() * 0.16f;
            float resp = bluffRespondentOsc.tick() * bluffRespondentEnv.tick() * 0.14f * bluffRespondentGain;
            float bluffMix = (sig + conf + resp)
                * static_cast<float>(materialAgency.gain(AgencyFamily::Voice)
                    * directedGain(OrganismStem::Bluff));
            wetL += bluffMix;
            wetR += bluffMix;
            organismStemAudit.addPreReverb(
                OrganismStem::Bluff, bluffMix, bluffMix);
            agencyMeter.add(AgencyFamily::Voice, bluffMix, bluffMix);
        }

        if (footnoteEnabled && footnoteArmed) {
            // Once armed at a weak metrical position, the gesture's own
            // internal note-to-note pacing runs on its own timer --
            // deliberately NOT locked to the step clock that got it
            // started, a private aside that stops following the piece's
            // own beat once it starts speaking.
            if (footnoteNoteGapCountdown > 0) {
                --footnoteNoteGapCountdown;
            } else if (footnoteNotesRemaining > 0) {
                int pcIdx = static_cast<int>(unit(rng) * footnoteComplementCount) % footnoteComplementCount;
                int footnotePc = footnoteComplementPcs[static_cast<size_t>(pcIdx)];
                int footnoteOctaveJitter = static_cast<int>(unit(rng) * 3.0) - 1;
                int footnoteMidiNote = pieceRootMidiNote + footnotePc + footnoteOctaveJitter * 12;
                footnotePluck.trigger(midiToFreq(footnoteMidiNote));
                ++footnoteEventCount;
                --footnoteNotesRemaining;
                footnoteNoteGapCountdown = static_cast<int>((0.15 + unit(rng) * 0.2) * sampleRate);
            } else if (!footnotePluck.isActive()) {
                footnoteArmed = false; // whole gesture finished, never re-arms (footnoteFired stays true)
            }
            float footnoteRaw = footnotePluck.tick();
            float footnoteWetL = footnoteReverbL.process(footnoteRaw);
            float footnoteWetR = footnoteReverbR.process(footnoteRaw);
            const float instrumentGain = static_cast<float>(
                materialAgency.gain(AgencyFamily::Instrument)
                * directedGain(OrganismStem::Footnote));
            const float footnoteL = (footnoteRaw * 0.5f + footnoteWetL * 0.6f)
                * instrumentGain;
            const float footnoteR = (footnoteRaw * 0.5f + footnoteWetR * 0.6f)
                * instrumentGain;
            wetL += footnoteL;
            wetR += footnoteR;
            organismStemAudit.addPreReverb(
                OrganismStem::Footnote, footnoteL, footnoteR);
            agencyMeter.add(AgencyFamily::Instrument, footnoteL, footnoteR);
        }

        agencyMeter.finishFrame();
        organismStemAudit.closePreReverb(wetL, wetR);

        const bool thermalLowpassActive = thermalFadeEnabled && arc.inCoda();
        double thermalAlpha = 1.0;
        if (thermalLowpassActive) {
            // Harmonics close off with the same envelope the gain fade
            // below uses -- "losing harmonics" as a real filter closing,
            // not just an EQ label. Cutoff floor (thermalLpMin) rather
            // than going all the way to DC, so what's left right at the
            // end is a dull thump dissolving into the static swell above,
            // not a mathematically perfect silence with nothing between.
            double lpCutoff = thermalLpMin + (20000.0 - thermalLpMin) * thermalGain;
            double rc = 1.0 / (2.0 * 3.141592653589793 * lpCutoff);
            thermalAlpha = (1.0 / sampleRate) / (rc + 1.0 / sampleRate);
            thermalLpStateL += static_cast<float>(thermalAlpha) * (wetL - thermalLpStateL);
            thermalLpStateR += static_cast<float>(thermalAlpha) * (wetR - thermalLpStateR);
            wetL = thermalLpStateL;
            wetR = thermalLpStateR;
        }

        float withReverbL = reverb.process(wetL);
        float withReverbR = reverbR.process(wetR);

        // Fase 169 -- first audible spatial consumer. Only the expressive
        // A/B may widen the existing stereo field; the mid component remains
        // untouched, so mono summation is preserved. The plan is bounded and
        // does not replace the engine's own pan identity.
        if (cfg.expressiveActuationEnabled) {
            const auto widened = SpatialExpressionField::applyWidth({withReverbL, withReverbR},
                                                                     lastSpatialExpressionPlan.width);
            withReverbL = static_cast<float>(widened.left);
            withReverbR = static_cast<float>(widened.right);
        }

        // The leveler reacts to a mono sum of both channels (not called
        // once per channel) so its gain correction is identical on both
        // sides -- otherwise two independently-drifting gains would
        // narrow or wobble the stereo image instead of just taming a loud
        // passage. See SignalLeveler::currentGain().
        leveler.process((withReverbL + withReverbR) * 0.5f);
        float gain = leveler.currentGain();

        // Equal-power pan across a slow LFO -- panLfo's depth (0.7) keeps
        // this a drift, never swinging hard to one side. panScale
        // (sqrt(2)) keeps the centre position's loudness matched to what
        // a mono signal would have measured here before stereo existed,
        // since equal-power gains are ~0.707 each at dead centre.
        constexpr float kPi = 3.14159265358979323846f;
        float panPos = panLfo.tick();
        if (cfg.expressiveActuationEnabled) {
            panPos = std::clamp(panPos + static_cast<float>(
                0.12 * lastSpatialExpressionPlan.pan), -1.0f, 1.0f);
        }
        if (barBoundary) {
            observeExpressiveEvent(
                ExpressiveEventSource::Noise,
                ExpressiveScale::Form,
                i,
                -1,
                noiseClimate,
                0.0,
                1.0 / sampleRate,
                state.density,
                state.resonance,
                noiseClimate,
                expressiveBit(ExpressiveDomain::Material),
                expressiveBit(ExpressiveCause::NoiseField),
                noiseClimate);
            observeExpressiveEvent(
                ExpressiveEventSource::Space,
                ExpressiveScale::Form,
                i,
                -1,
                std::fabs(static_cast<double>(panPos)),
                0.0,
                1.0 / sampleRate,
                state.density,
                state.resonance,
                noiseClimate,
                expressiveBit(ExpressiveDomain::Space),
                expressiveBit(ExpressiveCause::Relation),
                0.0,
                std::fabs(static_cast<double>(panPos)));
        }
        float panAngle = (panPos + 1.0f) * 0.25f * kPi;
        float panScale = 1.41421356f;
        float gainL = std::cos(panAngle) * panScale;
        float gainR = std::sin(panAngle) * panScale;

        // Thermal fade's gain multiply happens here, after the leveler
        // has already computed its own gain from the un-faded signal --
        // applying it any earlier would let the leveler read "got quiet"
        // and push its own gain back up to compensate, fighting the fade
        // instead of just riding along on top of it.
        float preL = (withReverbL * gain * 0.7f * gainL) * static_cast<float>(thermalGain) + thermalStaticL;
        float preR = (withReverbR * gain * 0.7f * gainR) * static_cast<float>(thermalGain) + thermalStaticR;
        organismStemAudit.processPostSpace(
            preL, preR, reverbMix, reverbDampingL, reverbDampingR,
            gain, gainL, gainR, thermalGain, thermalLowpassActive,
            thermalAlpha, thermalStaticL, thermalStaticR,
            cfg.expressiveActuationEnabled ? lastSpatialExpressionPlan.width : 0.0);

        if (parenthesisEnabled) {
            // feed() always gets the real live signal, never the blended
            // result below -- so a later parenthesis can't accidentally
            // capture an earlier parenthesis's own stutter.
            parenthesis.feed(preL, preR);
            float parenOutL, parenOutR;
            float parenEnv = parenthesis.tick(parenOutL, parenOutR);
            if (parenEnv > 0.0f) {
                preL = preL + parenEnv * (parenOutL - preL);
                preR = preR + parenEnv * (parenOutR - preR);
            }
        }

        // Fase 84 -- Curto-Circuito's hard gate: a genuine discontinuity
        // (~3ms linear ramp, never a fade curve) closing to true silence
        // for the hold, then reopening the same way -- the "corte seco"
        // signature a real reference waveform showed. A short noise
        // "spark" rides only the two ~3ms transition edges (never the
        // silent hold itself), envelope-shaped so it peaks mid-transition
        // and returns to zero at both ends -- literalizing "curto
        // circuito" as an actual electrical-sounding artifact instead of
        // a silent mute.
        if (curtoCircuitoEnabled && curtoCircuitoSamplesIntoEvent >= 0) {
            constexpr double kPiD = 3.14159265358979323846;
            int t = curtoCircuitoSamplesIntoEvent;
            int totalSamples = curtoCircuitoRampSamples * 2 + curtoCircuitoHoldSamples;
            float sparkL = 0.0f, sparkR = 0.0f;
            if (t < curtoCircuitoRampSamples) {
                double phase = (double)t / curtoCircuitoRampSamples;
                curtoCircuitoGateEnv = 1.0 - phase;
                float sparkShape = static_cast<float>(std::sin(kPiD * phase)) * 0.2f;
                sparkL = sparkShape * static_cast<float>(unit(rng) * 2.0 - 1.0);
                sparkR = sparkShape * static_cast<float>(unit(rng) * 2.0 - 1.0);
            } else if (t < curtoCircuitoRampSamples + curtoCircuitoHoldSamples) {
                curtoCircuitoGateEnv = 0.0;
            } else if (t < totalSamples) {
                double phase = (double)(t - curtoCircuitoRampSamples - curtoCircuitoHoldSamples) / curtoCircuitoRampSamples;
                curtoCircuitoGateEnv = phase;
                float sparkShape = static_cast<float>(std::sin(kPiD * phase)) * 0.2f;
                sparkL = sparkShape * static_cast<float>(unit(rng) * 2.0 - 1.0);
                sparkR = sparkShape * static_cast<float>(unit(rng) * 2.0 - 1.0);
            } else {
                curtoCircuitoGateEnv = 1.0;
                curtoCircuitoSamplesIntoEvent = -1;
                // Fase 85 stage 3 -- G: the rupture's rhythmic aftermath,
                // right as the gate finishes reopening -- see voiceOn()
                // above and its decrement at each new bar below.
                postGlitchFullKitBars = 2;
                ++postGlitchFullKitEvents;
            }
            if (curtoCircuitoSamplesIntoEvent >= 0) ++curtoCircuitoSamplesIntoEvent;
            preL = preL * static_cast<float>(curtoCircuitoGateEnv) + sparkL;
            preR = preR * static_cast<float>(curtoCircuitoGateEnv) + sparkR;
        }

        float dcBlockedL = dcBlockL.process(preL);
        float dcBlockedR = dcBlockR.process(preR);

        // Everything below remains part of the creative master until the
        // single limiter call near the end. The old order limited here and
        // then added montage/tear/trim afterward, forcing a permanent tanh
        // safety colour over every sample. One final safety boundary is both
        // cleaner and actually safer.
        float finalL = dcBlockedL * static_cast<float>(cfg.outputGainTrim);
        float finalR = dcBlockedR * static_cast<float>(cfg.outputGainTrim);

        // "Arquivo apocrifo" tear: ducks the live signal and substitutes a
        // reversed, bit-crushed quote of this same render's own much-
        // earlier output, read straight out of outSamples (already-limited/
        // saturated history, so the quoted material itself is already
        // safe-range) -- applied before the overload/true-peak stages so the
        // combined result still passes through the same final safety boundary
        // as everything else.
        if (apocryphalEnabled) {
            float tearL = 0.0f, tearR = 0.0f, duck = 1.0f;
            apocryphal.tick(outSamples, tearL, tearR, duck);
            finalL = finalL * duck + tearL;
            finalR = finalR * duck + tearR;
        }

        // "Corte de montagem" -- a real jump cut, not a blend: while
        // active, this REPLACES finalL/finalR outright (see
        // MontageEditCut.hpp for why both edges are genuinely click-free
        // hard cuts rather than a fade). Reads from outSamples (already
        // safe/limited history, same reasoning apocryphal tear above
        // already relies on) and only ever substitutes the already-safe
        // finalL/finalR, so it still passes through the overload protector,
        // true-peak limiter and artistic mute path like everything else.
        if (montageCutEnabled) {
            float montL, montR;
            if (montageCut.tick(finalL, finalR, outSamples, montL, montR)) {
                finalL = montL;
                finalR = montR;
            }
        }

        // Artistic mute precedes safety. Exact zero remains exact zero, while
        // every non-zero route -- including quoted history and jump cuts --
        // is now measured and protected by the same final stage.
        finalL *= muteGateSmoothed;
        finalR *= muteGateSmoothed;

        float protectedL = 0.0f, protectedR = 0.0f;
        overloadProtector.process(finalL, finalR, protectedL, protectedR);
        finalL = protectedL;
        finalR = protectedR;

        float limitedL = 0.0f, limitedR = 0.0f;
        limiter.process(finalL, finalR, limitedL, limitedR);
        finalL = limitedL;
        finalR = limitedR;

        if (selfFeedbackEnabled) {
            // Slow follower on the just-finished sample's loudness -- next
            // sample's Lorenz rate (see above) reacts to this. Reads the
            // signal before the stochastic mute gate below, deliberately
            // -- a few-millisecond artistic silence isn't a real change in
            // how loud the piece has actually been, and this follower is
            // slow enough (coefficient 0.0008) that a brief mute wouldn't
            // meaningfully move it either way.
            float instant = std::max(std::fabs(finalL), std::fabs(finalR));
            outputFeedbackEnergy += (instant - outputFeedbackEnergy) * 0.0008f;
        }

        if (crossDomainContagionEnabled) {
            // Fast attack (jumps straight to a new peak), moderate ~120ms
            // decay -- a real peak/transient reading, deliberately much
            // faster than self-feedback's slow loudness average just
            // above, since this is checking for a genuinely different
            // thing (a loud moment) rather than a trend.
            float instantPeak = std::max(std::fabs(finalL), std::fabs(finalR));
            audioPeakFollower = (instantPeak > audioPeakFollower)
                ? instantPeak
                : audioPeakFollower * peakFollowerRelease;
        }

        outSamples.push_back(finalL);
        outSamples.push_back(finalR);

        if (onProgress && (i % progressEvery == 0)) {
            renderProgress.completedFrames = i + 1;
            renderProgress.t = plannedCompositionFrames > 0
                ? std::min(1.0, static_cast<double>(i + 1) / plannedCompositionFrames) : 0.0;
            renderProgress.seconds = arc.elapsedSeconds();
            renderProgress.arcState = state;
            renderProgress.ringAmount = ringAmount;
            onProgress(renderProgress);
        }
    }

    if (onProgress) {
        renderProgress.completedFrames = outSamples.size() / 2;
        renderProgress.seconds = arc.elapsedSeconds();
        renderProgress.t = 1.0;
        renderProgress.phase = PieceRenderPhase::Finalizing;
        onProgress(renderProgress); // composition done, file not ready yet
    }

    // The last section has no following bar boundary to close it. Keep the
    // orchestral memory honest by committing that tail before the final
    // fades and delivery measurement. This remains optional shadow analysis:
    // it never changes the rendered master.
    if (organismStemAudit.enabled()) {
        const OrganismSectionSnapshot finalStemSnapshot =
            organismStemAudit.sectionSnapshot();
        const auto finalSection = [&]() {
            OrchestralSectionReport report;
            orchestralSectionMemory.beginSection();
            for (int stem = 0; stem < static_cast<int>(OrganismStem::Count); ++stem) {
                const auto organism = static_cast<OrganismStem>(stem);
                const auto &metrics = finalStemSnapshot[
                    static_cast<std::size_t>(stem)];
                const auto profile = baseOrchestralProfile(organism, identity);
                const bool active = metrics.audibleRatio > 0.004;
                orchestralSectionMemory.observe(organism,
                    {metrics.rms, metrics.lowBandRms, metrics.midBandRms,
                     metrics.highBandRms, metrics.audibleRatio,
                     profile.registerBand == RegisterBand::Low ? 43.0
                         : profile.registerBand == RegisterBand::High ? 78.0 : 60.0,
                     active});
            }
            return orchestralSectionMemory.finishSection();
        }();
        ++orchestralMemorySectionCount;
        orchestralHighConflictCount += finalSection.highConflictCount;
        if (finalSection.repeatedLeader) ++orchestralRepeatedLeaderCount;
        orchestralLastLeader = finalSection.strongest;
        if (narrativeAttentionActive && renderedArcState) {
            narrativeAttentionDecision = narrativeAttention.observe(
                finalStemSnapshot, narrativeAttentionProfiles,
                {lastRenderedArcState.density, lastRenderedArcState.resonance,
                 std::max(gamePlan.memory, relationalChamber.memory()),
                 false, OrganismStem::Other});
        }
    }

    // Fade the final buffer's very start/end -- cheap insurance against a
    // click regardless of what any given module's envelope happens to be
    // doing at those exact boundaries. The coda's own decay isn't
    // guaranteed to reach true silence before the walk ends (continuous
    // textures like Gendyn/granular/the reverb tail don't know to duck for
    // it), so this is applied directly to the finished stereo buffer,
    // independent of the internal mix, rather than trying to make every
    // module's envelope land on exactly zero.
    //
    // Real symptom the user heard and this fixes: the original 20ms/250ms
    // was sized as pure click-insurance, not a musical fade -- fine for
    // the ~40% of pieces where thermalFadeEnabled (fase 45) already glides
    // the ending down gracefully over 2.5-6.5s before this net even
    // engages, but for the other ~60% (thermalFadeEnabled false) this was
    // the ONLY thing shaping the ending, and 250ms reads as an abrupt chop
    // -- the mix is still at whatever level the walk happened to be at
    // when the coda's rolled length ran out, cut in a quarter second. Same
    // logic at the start: ~65% of pieces open already-present (see
    // GenerativeArc's openWithPresence, a deliberate choice, not a bug),
    // and 20ms is short enough to read as a sudden onset rather than an
    // actual fade-in.
    {
        // Seed-varied, not a fixed constant -- a single hardcoded duration
        // here would just trade "abrupt" for a new equally-recognizable
        // fingerprint every piece shares. Real compositions don't all fade
        // out over the identical stopwatch length either.
        const double fadeInSeconds = 0.05 + unit(rng) * 0.09;   // 50-140ms, was a flat 20ms
        const double fadeOutSeconds = thermalFadeEnabled
            ? (0.25 + unit(rng) * 0.30)   // thermal death already did the graceful work; short varied net
            : (1.0 + unit(rng) * 1.6);    // the only thing shaping this ending -- needs real range
        const size_t fadeInFrames = static_cast<size_t>(fadeInSeconds * sampleRate);
        const size_t fadeOutFrames = static_cast<size_t>(fadeOutSeconds * sampleRate);
        const size_t totalFrames = outSamples.size() / 2;
        size_t fadeIn = std::min(fadeInFrames, totalFrames);
        size_t fadeOut = std::min(fadeOutFrames, totalFrames);
        for (size_t f = 0; f < fadeIn; ++f) {
            float g = static_cast<float>(f) / static_cast<float>(fadeIn);
            outSamples[f * 2] *= g;
            outSamples[f * 2 + 1] *= g;
        }
        for (size_t f = 0; f < fadeOut; ++f) {
            float g = static_cast<float>(f) / static_cast<float>(fadeOut);
            size_t frameIdx = totalFrames - 1 - f;
            outSamples[frameIdx * 2] *= g;
            outSamples[frameIdx * 2 + 1] *= g;
        }
    }

    // Delivery follows all fades/edits. The normative 4x FIR is retained for
    // reporting; a separate longer 8x bank guards near-Nyquist overshoots
    // that Planches exposed. Same -1.8 target, constant linked gain, no second
    // compressor or blanket level change. This remains a finite estimate,
    // not certification for every reconstruction/filter. Poll cancellation
    // during the added offline work instead of blocking the closing worker.
    const auto delivery = protectOfflineDelivery(outSamples, sampleRate, [&] {
        if (onProgress) onProgress(renderProgress);
    });
    const float preDeliveryTruePeak = delivery.preNormativePeak;
    const float deliveryGain = delivery.gain;
    const float finalTruePeak = delivery.finalNormativePeak;
    const double deliveryTrimDb = deliveryGain < 1.0f
        ? 20.0 * std::log10(static_cast<double>(deliveryGain)) : 0.0;

    const std::vector<std::string> &visitedSections = arc.history();
    auto linearToDb = [](double value) {
        return value > 1.0e-12 ? 20.0 * std::log10(value) : -120.0;
    };

    const ExpressiveEventSummary expressiveSummary = expressiveEventLedger.summary();

    std::ostringstream configJson;
    configJson << "{";
    musicalDirection.writeJson(configJson);
    musicalRelations.writeJson(configJson);
    bassPhraseTimbre.writeJson(configJson);
    bassExecutionFeedback.writeJson(configJson);
    bassKickInteraction.writeJson(configJson);
    grooveDialogue.writeJson(configJson);
    selectionTrace.writeJson(configJson);
    if (cfg.albumId >= 0) {
        configJson << "\"albumId\":" << cfg.albumId << ","
                   << "\"albumTrackIndex\":" << cfg.albumTrackIndex << ","
                   << "\"albumTrackCount\":" << cfg.albumTrackCount << ","
                   << "\"albumTheme\":\"" << cfg.albumTheme << "\",";
    }
    configJson << "\"seed\":" << cfg.seed << ","
               << "\"bpm\":" << cfg.bpm << ","
               << "\"durationSeconds\":" << arc.elapsedSeconds() << ","
               << "\"targetDurationSeconds\":" << cfg.targetDurationSeconds << ","
               << "\"narrativeDurationScale\":" << narrativeDurationScale << ","
               << "\"seedLineage\":" << (cfg.seedLineageEnabled ? "true" : "false") << ","
               << "\"seedLineageParentA\":" << cfg.seedLineageParentA << ","
               << "\"seedLineageParentB\":" << cfg.seedLineageParentB << ","
               << "\"seedLineageGeneration\":" << cfg.seedLineageGeneration << ","
               << "\"seedLineageParentBWeight\":" << cfg.seedLineageParentBWeight << ","
               << "\"seedLineageMutationProbability\":"
               << cfg.seedLineageMutationProbability << ","
               << "\"seedLineageInheritedBMask\":" << cfg.seedLineageInheritedBMask << ","
               << "\"seedLineageBlendedMask\":" << cfg.seedLineageBlendedMask << ","
               << "\"seedLineageMutationMask\":" << cfg.seedLineageMutationMask << ","
               << "\"materialFocus\":\"" << materialFocusName(identity.focus) << "\","
               << "\"scaleFamily\":\"" << scaleFamilyName(identity.scaleFamily) << "\","
               << "\"coreVoiceModel\":\"" << coreVoiceModelName(identity.coreVoice) << "\","
               << "\"sequenceRhythmModel\":\"" << sequenceRhythmModelName(identity.rhythmModel) << "\","
               << "\"leadVoiceModel\":\"" << leadVoiceModelName(identity.leadVoice) << "\","
               << "\"harmonicLanguage\":\"" << harmonicLanguageName(identity.harmonicLanguage) << "\","
               << "\"ensembleModel\":\"" << ensembleModelName(identity.ensembleModel) << "\","
               << "\"gameStrategy\":\"" << gameStrategyName(gamePlan.strategy) << "\","
               << "\"gameRule\":\"" << gameRuleName(gamePlan.rule) << "\","
               << "\"gameEnergy\":" << gamePlan.energy << ","
               << "\"gameMemory\":" << gamePlan.memory << ","
               << "\"gameRisk\":" << gamePlan.risk << ","
               << "\"gameFreedom\":" << gamePlan.freedom << ","
               << "\"gameHiddenInformation\":" << gamePlan.hiddenInformation << ","
               << "\"rasgarMotion\":\"" << rasgarMotionName(rasgarGesture.motion()) << "\","
               << "\"rasgarFinalPhase\":\"" << rasgarPhaseName(rasgarGesture.phase()) << "\","
               << "\"rasgarTearCount\":" << rasgarGesture.tearCount() << ","
               << "\"rasgarUnstableBars\":" << rasgarGesture.unstableBars() << ","
               << "\"rasgarMaxInstability\":" << rasgarGesture.maxInstability() << ","
               << "\"rasgarMeanPulseDeviation\":" << rasgarGesture.meanPulseDeviation() << ","
               << "\"topologyRegionCount\":" << topology.regions().size() << ","
               << "\"topologyPassageCount\":" << topology.passages().size() << ","
               << "\"topologyTraversalCount\":" << topology.traversalCount() << ","
               << "\"topologyRerouteCount\":" << topology.rerouteCount() << ","
               << "\"topologySplitCount\":" << topology.splitCount() << ","
               << "\"topologyArchivePageCount\":" << moduleArchive.size() << ","
               << "\"archiveReadCount\":" << moduleArchive.count(ArchiveAction::Read) << ","
               << "\"archiveRereadCount\":" << moduleArchive.count(ArchiveAction::Reread) << ","
               << "\"archiveReinterpretCount\":" << moduleArchive.count(ArchiveAction::Reinterpret) << ","
               << "\"archiveRecreateCount\":" << moduleArchive.count(ArchiveAction::Recreate) << ","
               << "\"topologyHarmonicShiftCount\":" << topologyHarmonicShiftCount << ","
               << "\"relationalOrchestra\":" << (relationalOrchestraActive ? "true" : "false") << ","
               << "\"relationalVerb\":\"" << relationVerbName(relationalOrchestra.verb()) << "\","
               << "\"relationalRhythmObservationCount\":" << relationalOrchestra.rhythmObservationCount() << ","
               << "\"relationalThereminResponseCount\":" << relationalOrchestra.thereminResponseCount() << ","
               << "\"relationalHarmonyResponseCount\":" << relationalOrchestra.harmonyResponseCount() << ","
               << "\"relationalHarmonyReconfigurationCount\":" << relationalAppliedHarmonyShiftCount << ","
               << "\"relationalRhythmFeedbackCount\":" << relationalOrchestra.rhythmFeedbackCount() << ","
               << "\"relationalResistanceCount\":" << relationalOrchestra.resistanceCount() << ","
               << "\"relationalChamber\":" << (relationalChamberActive ? "true" : "false") << ","
               << "\"relationalChamberAuditReady\":"
               << (relationalChamberAuditReady ? "true" : "false") << ","
               << "\"relationalChamberRelationCount\":" << relationalChamber.relationCount() << ","
               << "\"relationalChamberConsequenceCount\":" << relationalChamberConsequenceCount << ","
               << "\"relationalChamberDecisionCount\":" << relationalChamber.decisionCount() << ","
               << "\"relationalChamberCiteCount\":" << relationalChamberCiteCount << ","
               << "\"relationalChamberResistCount\":" << relationalChamberResistCount << ","
               << "\"relationalChamberDeformCount\":" << relationalChamberDeformCount << ","
               << "\"relationalChamberScarCount\":" << relationalChamberScarCount << ","
               << "\"relationalChamberSpatialCrossingCount\":" << relationalChamberSpatialCrossingCount << ","
               << "\"relationalChamberLastManifestedBodies\":" << relationalChamberLastManifestedBodies << ","
               << "\"relationalChamberFluteGain\":" << relationalChamberResponse.fluteGain << ","
               << "\"relationalChamberTubaPressure\":" << relationalChamberResponse.tubaPressure << ","
               << "\"relationalChamberVoiceGain\":" << relationalChamberResponse.voiceGain << ","
               << "\"relationalChamberSpatialPolarity\":" << relationalChamberResponse.spatialPolarity << ","
               << "\"relationalChamberResponseValid\":"
               << (relationalChamberResponse.valid() ? "true" : "false") << ","
               << "\"expressiveMelodicActuation\":"
               << (cfg.expressiveActuationEnabled ? "true" : "false") << ","
               << "\"expressiveMelodicLivingMidi\":"
               << fmFluteInterpretation.livingMidi << ","
               << "\"expressiveMelodicConnection\":"
               << fmFluteInterpretation.connection << ","
               << "\"expressiveMelodicGestureMemory\":"
               << fmFluteInterpretation.gestureMemory << ","
               << "\"expressiveMaterialPlan\":{"
               << "\"spectralEnergy\":" << lastMaterialExpressionPlan.spectralEnergy << ","
               << "\"noiseAmount\":" << lastMaterialExpressionPlan.noiseAmount << ","
               << "\"harmonicity\":" << lastMaterialExpressionPlan.harmonicity << ","
               << "\"degradation\":" << lastMaterialExpressionPlan.degradation << ","
               << "\"air\":" << lastMaterialExpressionPlan.air << "},"
               << "\"expressiveSpatialPlan\":{"
               << "\"pan\":" << lastSpatialExpressionPlan.pan << ","
               << "\"width\":" << lastSpatialExpressionPlan.width << ","
               << "\"depth\":" << lastSpatialExpressionPlan.depth << ","
               << "\"driftRate\":" << lastSpatialExpressionPlan.driftRate << "},"
               << "\"relationalChamberLastAction\":\"" << relationalChamberLastAction
               << static_cast<char>(34) << ","
               << "\"narrativeAttention\":"
               << (narrativeAttentionActive ? "true" : "false") << ","
               << "\"narrativeAttentionObservedSectionCount\":"
               << narrativeAttention.observedSections() << ","
               << "\"narrativeAttentionFocusChangeCount\":"
               << narrativeAttention.focusChangeCount() << ","
               << "\"narrativeAttentionEventFocusCount\":"
               << narrativeAttention.eventFocusCount() << ","
               << "\"narrativeAttentionManifestedBodies\":"
               << narrativeAttention.lastDecision().manifestedBodies << ","
               << "\"narrativeAttentionLastValue\":"
               << narrativeAttention.lastDecision().value << ","
               << "\"narrativeAttentionMaximumValue\":"
               << narrativeAttention.maximumValue() << ","
               << "\"narrativeAttentionFinalFocus\":\""
               << (narrativeAttention.lastDecision().valid
                   ? organismStemName(narrativeAttention.lastDecision().focus) : "none")
               << "\"" << ","
               << "\"narrativeAttentionFinalReason\":\""
               << narrativeAttentionReasonName(narrativeAttention.lastDecision().reason)
               << "\","
               << "\"narrativeMix\":"
               << (narrativeMixActive ? "true" : "false") << ","
               << "\"narrativeMixDirectiveCount\":"
               << narrativeMix.decisionCount() << ","
               << "\"narrativeMixHysteresisHoldCount\":"
               << narrativeMix.hysteresisHoldCount() << ","
               << "\"narrativeMixEventDirectiveCount\":"
               << narrativeMix.eventDirectiveCount() << ","
               << "\"narrativeMixLastGainDb\":"
               << narrativeMix.lastDirective().gainDb << ","
               << "\"narrativeMixMaximumGainDb\":"
               << narrativeMix.maximumGainDb() << ","
               << "\"narrativeMixFinalFocus\":\""
               << (narrativeMix.lastDirective().valid
                   ? organismStemName(narrativeMix.lastDirective().focus) : "none")
               << "\","
               << "\"narrativeMixFinalReason\":\""
               << narrativeAttentionReasonName(narrativeMix.lastDirective().reason)
               << "\","
               << "\"relationalThereminPresentBarCount\":" << relationalOrchestra.thereminPresentBarCount() << ","
               << "\"relationalThereminSilentBarCount\":" << relationalOrchestra.thereminSilentBarCount() << ","
               << "\"relationalThereminEntryCount\":" << relationalOrchestra.thereminEntryCount() << ","
               << "\"relationalThereminWithdrawalCount\":" << relationalOrchestra.thereminWithdrawalCount() << ","
               << "\"relationalMemory\":" << relationalOrchestra.memory() << ","
               << "\"relationalResistance\":" << relationalOrchestra.resistance() << ","
               << "\"relationalThereminPresence\":" << relationalOrchestra.currentThereminPresence() << ","
               << "\"relationalThereminTargetMidi\":" << relationalOrchestra.currentThereminTargetMidi() << ","
               << "\"relationalThereminPitchChangeCount\":" << relationalThereminPitchChangeCount << ","
               << "\"relationalThereminRepeatedPitchCount\":" << relationalThereminRepeatedPitchCount << ","
               << "\"relationalThereminLongestRepeatedBars\":" << relationalThereminLongestRepeatedBars << ","
               << "\"relationalThereminPitchMin\":" << relationalThereminPitchMin << ","
               << "\"relationalThereminPitchMax\":" << relationalThereminPitchMax << ","
               << "\"materialAgency\":" << (materialAgency.enabled() ? "true" : "false") << ","
               << "\"agencyInitialForeground\":\"" << agencyInitialForeground << "\","
               << "\"agencyFinalForeground\":\"" << materialAgency.foregroundLabel() << "\","
               << "\"agencyForegroundChangeCount\":" << materialAgency.foregroundChangeCount() << ","
               << "\"agencyDispositionChangeCount\":" << materialAgency.dispositionChangeCount() << ","
               << "\"agencyLeaderlessSectionCount\":" << materialAgency.leaderlessSectionCount() << ","
               << "\"agencyPluralSectionCount\":" << materialAgency.pluralSectionCount() << ","
               << "\"agencyObservedSectionCount\":" << materialAgency.observedSectionCount() << ","
               << "\"agencyRelationalReactionCount\":" << materialAgency.relationalReactionCount() << ","
               << "\"agencyForegroundHistory\":[";
    for (std::size_t i = 0; i < materialAgency.foregroundHistory().size(); ++i) {
        configJson << "\"" << materialAgency.foregroundHistory()[i] << "\"";
        if (i + 1 < materialAgency.foregroundHistory().size()) configJson << ",";
    }
    configJson << "],\"agencyClaimantHistory\":[";
    for (std::size_t section = 0; section < materialAgency.claimantHistory().size(); ++section) {
        configJson << "[";
        const auto &claimants = materialAgency.claimantHistory()[section];
        for (std::size_t i = 0; i < claimants.size(); ++i) {
            configJson << "\"" << agencyFamilyName(claimants[i]) << "\"";
            if (i + 1 < claimants.size()) configJson << ",";
        }
        configJson << "]";
        if (section + 1 < materialAgency.claimantHistory().size()) configJson << ",";
    }
    configJson << "],\"agencyFinalDispositions\":{";
    for (int family = 0; family < static_cast<int>(AgencyFamily::Count); ++family) {
        const auto agencyFamily = static_cast<AgencyFamily>(family);
        configJson << "\"" << agencyFamilyName(agencyFamily) << "\":\""
                   << agencyDispositionName(materialAgency.disposition(agencyFamily)) << "\"";
        if (family + 1 < static_cast<int>(AgencyFamily::Count)) configJson << ",";
    }
    configJson << "},\"agencyFamilyMetrics\":{";
    for (int family = 0; family < static_cast<int>(AgencyFamily::Count); ++family) {
        const auto agencyFamily = static_cast<AgencyFamily>(family);
        configJson << "\"" << agencyFamilyName(agencyFamily) << "\":{"
                   << "\"rms\":" << agencyMeter.rms(agencyFamily) << ","
                   << "\"peak\":" << agencyMeter.peak(agencyFamily) << ","
                   << "\"audibleRatio\":" << agencyMeter.audibleRatio(agencyFamily) << "}";
        if (family + 1 < static_cast<int>(AgencyFamily::Count)) configJson << ",";
    }
    configJson << "},"
               << "\"organismAudibility\":"
               << (organismAudibility.enabled() ? "true" : "false") << ","
               << "\"organismAudibilityMovementCount\":"
               << organismAudibility.movementCount() << ","
               << "\"organismAudibilityEmergenceCount\":"
               << organismAudibility.emergenceCount() << ","
               << "\"organismAudibilityYieldCount\":"
               << organismAudibility.yieldCount() << ","
               << "\"organismSpectralRoleResponseCount\":"
               << organismAudibility.spectralRoleResponseCount() << ","
               << "\"organismAudibilityGains\":{";
    for (int stem = 0; stem < static_cast<int>(OrganismStem::Count); ++stem) {
        const auto organism = static_cast<OrganismStem>(stem);
        configJson << "\"" << organismStemName(organism) << "\":{"
                   << "\"min\":" << organismAudibility.minimumGain(organism) << ","
                   << "\"max\":" << organismAudibility.maximumGain(organism) << ","
                   << "\"final\":" << directedGain(organism) << "}";
        if (stem + 1 < static_cast<int>(OrganismStem::Count)) configJson << ",";
    }
    configJson << "},"
               << "\"orchestralProfiles\":{";
    for (int stem = 0; stem < static_cast<int>(OrganismStem::Count); ++stem) {
        const auto organism = static_cast<OrganismStem>(stem);
        const auto profile = baseOrchestralProfile(organism, identity);
        configJson << "\"" << organismStemName(organism) << "\":{"
                   << "\"role\":\"" << orchestralRoleName(profile.role) << "\","
                   << "\"register\":\"" << registerBandName(profile.registerBand) << "\","
                   << "\"spectralOccupation\":\""
                   << spectralOccupationName(profile.spectralOccupation) << "\","
                   << "\"presenceMode\":\"" << presenceModeName(profile.presenceMode) << "\"}";
        if (stem + 1 < static_cast<int>(OrganismStem::Count)) configJson << ",";
    }
    configJson << "},"
               << "\"organismStemAudit\":"
               << (organismStemAudit.enabled() ? "true" : "false") << ","
               << "\"organismStemMaxReconstructionError\":"
               << organismStemAudit.maxPostReconstructionError() << ","
               << "\"organismLifeStates\":{";
    for (int stem = 0; stem < static_cast<int>(OrganismStem::Count); ++stem) {
        const auto organism = static_cast<OrganismStem>(stem);
        ModuleLifeRecord life;
        life.mark(ModuleLifeState::Available);
        const auto metrics = organismStemAudit.metrics(organism);
        const bool manifested = metrics.preSpaceRms > 1.0e-9
            || metrics.postSpaceRms > 1.0e-9;
        const bool audible = metrics.postSpaceRms > 1.0e-9
            && metrics.postAudibleRatio > 0.004;
        if (manifested) {
            life.mark(ModuleLifeState::Selected);
            life.mark(ModuleLifeState::Manifested);
        }
        if (audible) {
            life.mark(ModuleLifeState::Audible);
        }
        const auto mask = life.mask();
        const auto importedLife = ModuleLifeRecord::fromMask(mask);
        configJson << "\"" << organismStemName(organism) << "\":{"
                   << "\"mask\":" << static_cast<int>(mask) << ","
                   << "\"highest\":\"" << moduleLifeStateName(importedLife.highest())
                   << "\"}";
        if (stem + 1 < static_cast<int>(OrganismStem::Count)) configJson << ",";
    }
    configJson << "},"
               << "\"organismStemMetrics\":{";
    for (int stem = 0; stem < static_cast<int>(OrganismStem::Count); ++stem) {
        const auto organism = static_cast<OrganismStem>(stem);
        const auto metrics = organismStemAudit.metrics(organism);
        configJson << "\"" << organismStemName(organism) << "\":{"
                   << "\"preSpaceRms\":" << metrics.preSpaceRms << ","
                   << "\"postSpaceRms\":" << metrics.postSpaceRms << ","
                   << "\"preSpacePeak\":" << metrics.preSpacePeak << ","
                   << "\"postSpacePeak\":" << metrics.postSpacePeak << ","
                   << "\"preAudibleRatio\":" << metrics.preAudibleRatio << ","
                   << "\"postAudibleRatio\":" << metrics.postAudibleRatio << ","
                   << "\"levelBuriedRatio\":" << metrics.levelBuriedRatio << ","
                   << "\"energyShare\":" << metrics.energyShare << ","
                   << "\"lowBandRms\":" << metrics.lowBandRms << ","
                   << "\"midBandRms\":" << metrics.midBandRms << ","
                   << "\"highBandRms\":" << metrics.highBandRms << ","
                   << "\"postToPreDb\":" << metrics.postToPreDb << "}";
        if (stem + 1 < static_cast<int>(OrganismStem::Count)) configJson << ",";
    }
    configJson << "},"
               << "\"spectralTechniqueCount\":" << identity.spectral.count() << ","
               << "\"inputTruePeakDbtp\":" << linearToDb(limiter.inputTruePeak()) << ","
               << "\"preDeliveryTruePeakDbtp\":" << linearToDb(preDeliveryTruePeak) << ","
               << "\"outputTruePeakDbtp\":" << linearToDb(finalTruePeak) << ","
               << "\"outputTruePeakMethod\":\"" << TruePeakDetector::methodId() << "\","
               << "\"deliveryTruePeakTargetDbtp\":-1.8,"
               << "\"publicTruePeakCeilingDbtp\":-1.0,"
               << "\"deliveryTruePeakTrimDb\":" << deliveryTrimDb << ","
               << "\"deliveryGuardMethod\":\"" << DeliveryPeakGuard::methodId() << "\","
               << "\"preDeliveryGuardDbtp\":" << linearToDb(delivery.preGuardPeak) << ","
               << "\"deliveryGuardProjectedDbtp\":" << linearToDb(delivery.projectedGuardPeak) << ","
               << "\"limiterMaxGainReductionDb\":" << limiter.maxGainReductionDb() << ","
               << "\"overloadMaxGainReductionDb\":" << overloadProtector.maxReductionDb() << ","
               << "\"overloadFrameCount\":" << overloadProtector.overloadFrames() << ","
               << "\"outputNonFiniteCount\":" << limiter.nonFiniteCount() << ","
               << "\"narrativeTemplate\":\"" << arc.templateDescription() << "\","
               << "\"voiceColor\":\"" << (voiceColor == VoiceColor::Tape ? "tape" : voiceColor == VoiceColor::Tube ? "tube" : voiceColor == VoiceColor::Fracture ? "fracture" : "none") << "\","
               << "\"loFiDelay\":" << (loFiDelay ? "true" : "false") << ","
               << "\"staticPop\":" << (staticPopEnabled ? "true" : "false") << ","
               << "\"granularTexture\":" << (granularTextureEnabled ? "true" : "false") << ","
               << "\"gendynTexture\":" << (gendynTextureEnabled ? "true" : "false") << ","
               << "\"coloredNoiseBed\":" << (coloredNoiseBedEnabled ? "true" : "false") << ","
               << "\"noiseClimateMin\":" << noiseClimateMin << ","
               << "\"noiseClimateMax\":" << noiseClimateMax << ","
               << "\"noiseClimateMean\":"
               << (noiseClimateSamples > 0 ? noiseClimateSum / static_cast<double>(noiseClimateSamples) : 0.0) << ","
               << "\"noiseColor\":\"" << noiseColorName(activeNoiseColor) << "\","
               << "\"noiseColorChangeCount\":" << noiseColorChangeCount << ","
               << "\"snareNoiseColor\":\"" << snareNoiseColorName << "\","
               << "\"clapNoiseColor\":\"" << clapNoiseColorName << "\","
               << "\"maracasNoiseColor\":\"" << maracasNoiseColorName << "\","
               << "\"selfFeedback\":" << (selfFeedbackEnabled ? "true" : "false") << ","
               << "\"crossFmDepth\":" << crossFmDepth << ","
               << "\"sequenceLockAmount\":" << sequencer.lockAmount() << ","
               << "\"sequenceJumpiness\":" << sequencer.jumpiness() << ","
               << "\"melodicPitchChangeCount\":" << melodicPitchChangeCount << ","
               << "\"melodicRepeatedPitchCount\":" << melodicRepeatedPitchCount << ","
               << "\"melodicLongestRepeatedSteps\":" << melodicLongestRepeatedSteps << ","
               << "\"melodicPitchMin\":" << melodicPitchMin << ","
               << "\"melodicPitchMax\":" << melodicPitchMax << ","
               << "\"fmFlutePitchChangeCount\":" << fmFlutePitchChangeCount << ","
               << "\"fmFluteRepeatedPitchCount\":" << fmFluteRepeatedPitchCount << ","
               << "\"fmFluteLongestRepeatedNotes\":" << fmFluteLongestRepeatedNotes << ","
               << "\"fmFlutePitchMin\":" << fmFlutePitchMin << ","
               << "\"fmFlutePitchMax\":" << fmFlutePitchMax << ","
               << "\"melodicContourDirection\":" << sequencer.melodicContourDirection() << ","
               << "\"melodicContourApexBar\":" << sequencer.melodicContourApexBar() << ","
               << "\"ochdBreathing\":" << (ochdEnabled ? "true" : "false") << ","
               << "\"hihatBranchMode\":\"" << (hihatBranch.mode() == BernoulliGate::Mode::Toggle ? "toggle" : "bernoulli") << "\","
               << "\"hihatOpenProbability\":" << hihatBranch.probability() << ","
               << "\"drumsPresence\":" << drumsPresence << ","
               << "\"drumsPresenceFloorApplied\":" << (drumsPresenceFloorApplied ? "true" : "false") << ","
               << "\"drumsProtagonist\":" << (rhythmicProtagonist ? "true" : "false") << ","
               << "\"drumsPeak\":" << drumsPeak << ","
               << "\"drumsRms\":" << (drumsMeasuredFrames > 0 ? std::sqrt(drumsEnergySum / static_cast<double>(drumsMeasuredFrames)) : 0.0) << ","
               << "\"drumVoiceRms\":["
               << (drumsMeasuredFrames > 0 ? std::sqrt(drumVoiceEnergySum[0] / static_cast<double>(drumsMeasuredFrames)) : 0.0) << ","
               << (drumsMeasuredFrames > 0 ? std::sqrt(drumVoiceEnergySum[1] / static_cast<double>(drumsMeasuredFrames)) : 0.0) << ","
               << (drumsMeasuredFrames > 0 ? std::sqrt(drumVoiceEnergySum[2] / static_cast<double>(drumsMeasuredFrames)) : 0.0) << ","
               << (drumsMeasuredFrames > 0 ? std::sqrt(drumVoiceEnergySum[3] / static_cast<double>(drumsMeasuredFrames)) : 0.0) << ","
               << (drumsMeasuredFrames > 0 ? std::sqrt(drumVoiceEnergySum[4] / static_cast<double>(drumsMeasuredFrames)) : 0.0) << ","
               << (drumsMeasuredFrames > 0 ? std::sqrt(drumVoiceEnergySum[5] / static_cast<double>(drumsMeasuredFrames)) : 0.0) << ","
               << (drumsMeasuredFrames > 0 ? std::sqrt(drumVoiceEnergySum[6] / static_cast<double>(drumsMeasuredFrames)) : 0.0) << ","
               << (drumsMeasuredFrames > 0 ? std::sqrt(drumVoiceEnergySum[7] / static_cast<double>(drumsMeasuredFrames)) : 0.0) << "],"
               << "\"drumVoicePeak\":["
               << drumVoicePeak[0] << "," << drumVoicePeak[1] << ","
               << drumVoicePeak[2] << "," << drumVoicePeak[3] << ","
               << drumVoicePeak[4] << "," << drumVoicePeak[5] << ","
               << drumVoicePeak[6] << "," << drumVoicePeak[7] << "],"
               << "\"snareBodyRms\":"
               << (drumsMeasuredFrames > 0 ? std::sqrt(snareBodyEnergySum / static_cast<double>(drumsMeasuredFrames)) : 0.0) << ","
               << "\"snareNoiseRms\":"
               << (drumsMeasuredFrames > 0 ? std::sqrt(snareNoiseEnergySum / static_cast<double>(drumsMeasuredFrames)) : 0.0) << ","
               << "\"snareBodyPeak\":" << snareBodyPeak << ","
               << "\"snareNoisePeak\":" << snareNoisePeak << ","
               << "\"drumsAudibleFrameRatio\":" << (drumsMeasuredFrames > 0 ? static_cast<double>(drumsAudibleFrames) / static_cast<double>(drumsMeasuredFrames) : 0.0) << ","
               << "\"drumsEnabled\":" << (drumsEnabled ? "true" : "false") << ","
               << "\"drumHitFailureCount\":" << drumHitFailureCount << ","
               << "\"drumArchetype\":" << (drumArchetypeEnabled ? "true" : "false") << ","
               << "\"drumArchetypeName\":\"" << (drumArchetypeEnabled ? currentArchetypeName() : "") << "\","
               << "\"rhythmSieveUsed\":" << ((drumArchetypeEnabled && currentArchetypeIndex == rhythmSieveSlotIndex) ? "true" : "false") << ","
               << "\"rhythmGenAksakUsed\":" << ((drumArchetypeEnabled && currentArchetypeIndex == rhythmAksakSlotIndex) ? "true" : "false") << ","
               << "\"rhythmGenEuclidUsed\":" << ((drumArchetypeEnabled && currentArchetypeIndex == rhythmEuclidSlotIndex) ? "true" : "false") << ","
               << "\"ghostHihatCount\":" << ghostHihatCount << ","
               << "\"percussionTimbreVariation\":" << (percussionTimbreVariationEnabled ? "true" : "false") << ","
               << "\"percussionTimbreDepth\":" << percussionTimbreDepth << ","
               << "\"snareWhiteVariationCompensation\":" << (snareWhiteVariationCompensation ? "true" : "false") << ","
               << "\"snareColorBalanceCompensation\":" << (snareColorBalanceCompensation ? "true" : "false") << ","
               << "\"orchestrationChangeCount\":" << orchestrationChangeCount << ","
               << "\"orchestralMemorySectionCount\":" << orchestralMemorySectionCount << ","
               << "\"orchestralHighConflictCount\":" << orchestralHighConflictCount << ","
               << "\"orchestralRepeatedLeaderCount\":" << orchestralRepeatedLeaderCount << ","
               << "\"orchestralRoleResponseCount\":" << orchestralRoleResponseCount << ","
               << "\"orchestralLastLeader\":\""
               << organismStemName(orchestralLastLeader) << "\","
               << "\"postGlitchFullKitEvents\":" << postGlitchFullKitEvents << ","
               << "\"meterVariance\":" << (meterVarianceEnabled ? "true" : "false") << ","
               << "\"meterChangeCount\":" << meterChangeCount << ","
               << "\"finalBarLength\":" << currentBarLength << ","
               << "\"drumArchetypeChangeCount\":" << archetypeChangeCount << ","
               << "\"apocryphalTear\":" << (apocryphal.firedThisPiece() ? "true" : "false") << ","
               << "\"apocryphalSourceSeconds\":" << apocryphal.lastSourceSeconds() << ","
               << "\"thereminVoice\":" << (thereminEnabled ? "true" : "false") << ","
               << "\"montageCut\":" << (montageCutEnabled && montageCut.firedThisPiece() ? "true" : "false") << ","
               << "\"montageCutSourceSeconds\":" << montageCut.lastSourceSeconds() << ","
               << "\"atropelo\":" << ((atropeloEnabled && atropelo.eventCount() > 0) ? "true" : "false") << ","
               << "\"atropeloEventCount\":" << atropelo.eventCount() << ","
               << "\"atropeloInterruptionCount\":" << atropelo.interruptionCount() << ","
               << "\"atropeloResumedCount\":" << atropelo.resumedEventCount() << ","
               << "\"atropeloForgottenCount\":" << atropelo.forgottenCount() << ","
               << "\"atropeloSilenceInvasionCount\":" << atropeloSilenceInvasionCount << ","
               << "\"atropeloNestingCount\":" << atropeloNestingCount << ","
               << "\"tempoDrift\":" << (tempoDriftEnabled ? "true" : "false") << ","
               << "\"tempoDriftDepth\":" << tempoDriftDepth << ","
               << "\"tempoChangeCount\":" << tempoChangeCount << ","
               << "\"codaRitardando\":" << (codaRitardandoEnabled ? "true" : "false") << ","
               << "\"accelerando\":" << (accelerandoEnabled ? "true" : "false") << ","
               << "\"parenthesis\":" << (parenthesisEnabled ? "true" : "false") << ","
               << "\"parenthesisCount\":" << parenthesisCount << ","
               << "\"parenthesisLastMode\":\"" << parenthesisLastMode << "\","
               << "\"parenthesisLastDurationSeconds\":" << parenthesisLastDurationSeconds << ","
               << "\"parenthesisLastLoopSeconds\":" << parenthesisLastLoopSeconds << ","
               << "\"curtoCircuito\":" << ((curtoCircuitoEnabled && curtoCircuitoFired) ? "true" : "false") << ","
               << "\"curtoCircuitoGatecrashCount\":" << curtoCircuitoGatecrashCount << ","
               << "\"curtoCircuitoGatecrashed\":\"" << curtoCircuitoGatecrashedNames << "\","
               << "\"agon\":" << (agonEnabled ? "true" : "false") << ","
               << "\"agonRusgaCount\":" << agonRusgaCount << ","
               << "\"estranho\":" << (estranhoEnabled ? "true" : "false") << ","
               << "\"estranhoCount\":" << estranhoCount << ","
               << "\"foreignSignalUsed\":" << ((foreignSignalEnabled && foreignReading.available) ? "true" : "false") << ","
               << "\"foreignSignalDescription\":\"" << (foreignReading.available ? foreignReading.description : "") << "\","
               << "\"wildMode\":" << (cfg.wildMode ? "true" : "false") << ","
               << "\"wildReseedCount\":" << wildReseedCount << ","
               << "\"fixedOverflow\":" << (fixedOverflowEnabled ? "true" : "false") << ","
               << "\"fixedOverflowCount\":" << fixedOverflowCount << ","
               << "\"spectralGhost\":" << (spectralGhostEnabled ? "true" : "false") << ","
               << "\"spectralGhostCount\":" << spectralGhostCount << ","
               << "\"sampleHoldGlitch\":" << (sampleHoldGlitchEnabled ? "true" : "false") << ","
               << "\"sampleHoldGlitchCount\":" << sampleHoldGlitchCount << ","
               << "\"tomRatchet\":" << (tomRatchetEnabled ? "true" : "false") << ","
               << "\"tomRatchetCount\":" << tomRatchetCount << ","
               << "\"cymbalSustain\":" << (cymbalSustainEnabled ? "true" : "false") << ","
               << "\"cymbalSustainCount\":" << cymbalSustainCount << ","
               << "\"euclideanCowbell\":" << (euclideanCowbellEnabled ? "true" : "false") << ","
               << "\"euclideanCowbellPattern\":\"" << euclideanDescription << "\","
               << "\"harmonicWeight\":" << (sequencer.harmonicWeightEnabled() ? "true" : "false") << ","
               << "\"strongBeatPull\":" << sequencer.strongBeatPull() << ","
               << "\"tonalModulation\":" << (tonalModulationEnabled ? "true" : "false") << ","
               << "\"modulationCount\":" << modulationCount << ","
               << "\"finalRootMidiNote\":" << sequencer.rootMidiNote() << ","
               << "\"modalShift\":" << (modalShiftEnabled ? "true" : "false") << ","
               << "\"modalShiftCount\":" << modalShiftCount << ","
               << "\"chordProgression\":" << (sequencer.chordProgressionEnabled() ? "true" : "false") << ","
               << "\"neoRiemannian\":" << (sequencer.neoRiemannianEnabled() ? "true" : "false") << ","
               << "\"neoRiemannianOpCount\":" << sequencer.neoRiemannianOpCount() << ","
               << "\"voiceLeading\":" << (sequencer.voiceLeadingEnabled() ? "true" : "false") << ","
               << "\"rhythmHumanize\":" << (rhythmHumanizeEnabled ? "true" : "false") << ","
               << "\"expressiveActuation\":"
               << (cfg.expressiveActuationEnabled ? "true" : "false") << ","
               << "\"expressiveActuationEventCount\":"
               << expressiveActuationEventCount << ","
               << "\"humanizeMaxSamples\":" << humanizeMaxSamples << ","
               << "\"humanizeApplicationCount\":" << humanizeApplicationCount << ","
               << "\"swingApplicationCount\":" << swingApplicationCount << ","
               << "\"percFxColoredCount\":" << percFxColoredCount << ","
               << "\"percFxCharacterVcaCount\":" << percFxCharacterVcaCount << ","
               << "\"hocketing\":" << (hocketEnabled ? "true" : "false") << ","
               << "\"hocketCount\":" << hocketCount << ","
               << "\"hocketCascade\":" << (hocketCascadeEnabled ? "true" : "false") << ","
               << "\"hocketInfluenceLinks\":" << (static_cast<int>(hocket1ToOsc1) + static_cast<int>(hocket1ToOsc2)
                                                  + static_cast<int>(hocket2ToOsc1) + static_cast<int>(hocket2ToOsc2)) << ","
               << "\"freeIntonation\":" << (freeIntonationEnabled ? "true" : "false") << ","
               << "\"heisenberg\":" << (heisenbergEnabled ? "true" : "false") << ","
               << "\"heisenbergBalance\":" << heisenbergBalance << ","
               << "\"freeIntonationDepth\":" << freeIntonationDepth << ","
               << "\"rungler\":" << (runglerEnabled ? "true" : "false") << ","
               << "\"leibnizRectifier\":" << (leibnizEnabled ? "true" : "false") << ","
               << "\"tubaVoice\":" << (tubaVoiceEnabled ? "true" : "false") << ","
               << "\"chaosContagion\":" << (chaosContagionEnabled ? "true" : "false") << ","
               << "\"thermalFade\":" << (thermalFadeEnabled ? "true" : "false") << ","
               << "\"thermalAbruptDeath\":" << (thermalAbruptDeath ? "true" : "false") << ","
               << "\"rosslerCollapse\":" << (rosslerEnabled ? "true" : "false") << ","
               << "\"selfEpitaph\":" << (selfEpitaphEnabled ? "true" : "false") << ","
               << "\"stochasticMute\":" << (stochasticMuteEnabled ? "true" : "false") << ","
               << "\"walshOscillator\":" << (walshOscEnabled ? "true" : "false") << ","
               << "\"vosim\":" << (vosimEnabled ? "true" : "false") << ","
               << "\"hardSync\":" << (hardSyncEnabled ? "true" : "false") << ","
               << "\"binaural\":" << (binauralEnabled ? "true" : "false") << ","
               << "\"binauralOffsetHz\":" << binauralOffsetHz << ","
               << "\"asphyxia\":" << (asphyxiaEnabled ? "true" : "false") << ","
               << "\"asphyxiaCount\":" << asphyxiaCount << ","
               << "\"polivoks\":" << (polivoksEnabled ? "true" : "false") << ","
               << "\"polivoksVca\":" << (polivoksVcaEnabled ? "true" : "false") << ","
               << "\"acidBassline\":" << (acidBasslineEnabled ? "true" : "false") << ","
               << "\"acidBasslineAccentCount\":" << acidBasslineAccentCount << ","
               << "\"acidBasslineSlideCount\":" << acidBasslineSlideCount << ","
               << "\"bassBehaviour\":" << (bassBehaviourEnabled ? "true" : "false") << ","
               << "\"bassPlanCount\":" << bassPlanCount << ","
               << "\"bassEventCount\":" << bassEventCount << ","
               << "\"bassTubaEventCount\":" << bassTubaEventCount << ","
               << "\"bassAcidEventCount\":" << bassAcidEventCount << ","
               << "\"bassCoreSubEventCount\":" << bassCoreSubEventCount << ","
               << "\"bassGrooveApplicationCount\":" << bassGrooveApplicationCount << ","
               << "\"drumGrooveApplicationCount\":" << drumGrooveApplicationCount << ","
               << "\"bassEarlyTimingClampCount\":" << bassEarlyTimingClampCount << ","
               << "\"bassFinalRole\":\"" << bassFunctionRoleName(bassFinalRole) << "\","
               << "\"bassFinalBody\":\"" << bassBodyName(bassFinalBody) << "\","
               << "\"egLoopPulse\":" << (egLoopPulseEnabled ? "true" : "false") << ","
               << "\"egLoopPulseCount\":" << egLoopPulseCount << ","
               << "\"footnoteGesture\":" << (footnoteFired ? "true" : "false") << ","
               << "\"footnoteEventCount\":" << footnoteEventCount << ","
               << "\"galoCall\":" << (galoFired ? "true" : "false") << ","
               << "\"scatVoice\":" << (scatEnabled ? "true" : "false") << ","
               << "\"scatEventCount\":" << scatEventCount << ","
               << "\"choir\":" << (choirEnabled ? "true" : "false") << ","
               << "\"choirPhraseCount\":" << choirPhraseCount << ","
               << "\"dejaVu\":" << (dejaVuEnabled ? "true" : "false") << ","
               << "\"dejaVuEventCount\":" << dejaVuEventCount << ","
               << "\"apocryphalReading\":" << ((selfEpitaphEnabled && !apocryphalNearMisses.empty()) ? "true" : "false") << ","
               << "\"polivoksSelfOscillationCount\":" << polivoksSelfOscillationCount << ","
               << "\"vectorSynth\":" << (vectorSynthEnabled ? "true" : "false") << ","
               << "\"harmonaig\":" << (harmonaigEnabled ? "true" : "false") << ","
               << "\"harmonaigChordCount\":" << harmonaigChordCount << ","
               << "\"sievePolyphony\":" << (sievePolyphonyEnabled ? "true" : "false") << ","
               << "\"sievePolyphonyPlasmaCount\":" << sievePolyphonyPlasmaCount << ","
               << "\"sievePitchChangeCount\":[" << sievePitchChangeCount[0] << "," << sievePitchChangeCount[1] << ","
               << sievePitchChangeCount[2] << "," << sievePitchChangeCount[3] << "],"
               << "\"sieveRepeatedPitchCount\":[" << sieveRepeatedPitchCount[0] << "," << sieveRepeatedPitchCount[1] << ","
               << sieveRepeatedPitchCount[2] << "," << sieveRepeatedPitchCount[3] << "],"
               << "\"sieveLongestRepeatedNotes\":[" << sieveLongestRepeatedNotes[0] << "," << sieveLongestRepeatedNotes[1] << ","
               << sieveLongestRepeatedNotes[2] << "," << sieveLongestRepeatedNotes[3] << "],"
               << "\"sievePitchMin\":[" << sievePitchMin[0] << "," << sievePitchMin[1] << ","
               << sievePitchMin[2] << "," << sievePitchMin[3] << "],"
               << "\"sievePitchMax\":[" << sievePitchMax[0] << "," << sievePitchMax[1] << ","
               << sievePitchMax[2] << "," << sievePitchMax[3] << "],"
               << "\"sieveWaterArticulationChangeCount\":" << sieveWaterArticulationChangeCount << ","
               << "\"sieveWaterDensityMin\":" << sieveWaterDensityMin << ","
               << "\"sieveWaterDensityMax\":" << sieveWaterDensityMax << ","
               << "\"sieveWaterGrainDurationMin\":" << sieveWaterGrainDurationMin << ","
               << "\"sieveWaterGrainDurationMax\":" << sieveWaterGrainDurationMax << ","
               << "\"sieveWaterPitchJitterMin\":" << sieveWaterPitchJitterMin << ","
               << "\"sieveWaterPitchJitterMax\":" << sieveWaterPitchJitterMax << ","
               << "\"bloom\":" << (bloomEnabled ? "true" : "false") << ","
               << "\"bloomLevelSwitchCount\":" << bloomLevelSwitchCount << ","
               << "\"gameWeightAlea\":" << gameWeightAlea << ","
               << "\"gameWeightAgon\":" << gameWeightAgon << ","
               << "\"gameWeightIlinx\":" << gameWeightIlinx << ","
               << "\"gameWeightMimicry\":" << gameWeightMimicry << ","
               << "\"crossDomainContagion\":" << (crossDomainContagionEnabled ? "true" : "false") << ","
               << "\"crossDomainContagionCount\":" << crossDomainContagionCount << ","
               << "\"generatedScale\":" << (generatedScaleEnabled ? "true" : "false") << ","
               << "\"generatedScaleDescription\":\"" << generatedScaleDescription << "\","
               << "\"fmFlute\":" << (fmFluteEnabled ? "true" : "false") << ","
               << "\"fmFluteNoteCount\":" << fmFluteNoteCount << ","
               << "\"phasing\":" << (phasingEnabled ? "true" : "false") << ","
               << "\"phasingEventCount\":" << phasingEventCount << ","
               << "\"microtonal\":" << (microtonalEnabled ? "true" : "false") << ","
               << "\"microtonalSystem\":\"" << (microtonalEnabled ? microtonalTuning.description() : "") << "\","
               << "\"spectralObject\":" << (spectralObjectEnabled ? "true" : "false") << ","
               << "\"spectralObjectAdmittedPartials\":" << (spectralObjectEnabled ? spectralObject.admittedCount() : 0) << ","
               << "\"polymeter\":" << (polymeterEnabled ? "true" : "false") << ","
               << "\"metricModulation\":" << (metricModulationEnabled ? "true" : "false") << ","
               << "\"bluffSignal\":" << (bluffSignalEnabled ? "true" : "false") << ","
               << "\"rasgoFracture\":" << (rasgoFractureEnabled ? "true" : "false") << ","
               << "\"coherenceCollapse\":" << (coherenceCollapseEnabled ? "true" : "false") << ","
               << "\"chaosStepAddress\":" << (chaosStepAddressEnabled ? "true" : "false") << ","
               << "\"chaosStepAddressCount\":" << chaosStepAddressCount << ","
               << "\"intensityGuaranteeTriggered\":" << (intensityGuaranteeTriggered ? "true" : "false") << ","
               << "\"initialScale\":\"" << scaleName(initialScale) << "\","
               << "\"scale\":\"" << sequencer.scaleDescription() << "\","
               << "\"rootMidiNote\":" << pieceRootMidiNote << ","
               << "\"sections\":[";
    for (size_t i = 0; i < visitedSections.size(); ++i) {
        configJson << "\"" << visitedSections[i] << "\"";
        if (i + 1 < visitedSections.size()) configJson << ",";
    }
    configJson << "],\"expressiveTemporal\":{";
    configJson << "\"schema\":" << expressiveTemporalSchemaVersion << ","
               << "\"enabled\":" << (expressiveSummary.enabled ? "true" : "false") << ","
               << "\"eventCount\":" << expressiveSummary.eventCount << ","
               << "\"early\":" << expressiveSummary.earlyEventCount << ","
               << "\"onGrid\":" << expressiveSummary.onGridEventCount << ","
               << "\"late\":" << expressiveSummary.lateEventCount << ","
               << "\"ghost\":" << expressiveSummary.ghostEventCount << ","
               << "\"ratchet\":" << expressiveSummary.ratchetEventCount << ","
               << "\"clamped\":" << expressiveSummary.clampedEventCount << ","
               << "\"invalid\":" << expressiveSummary.invalidValueCount << ","
               << "\"timingSamples\":{";
    configJson << "\"min\":" << expressiveSummary.minimumTimingSamples << ","
               << "\"max\":" << expressiveSummary.maximumTimingSamples << ","
               << "\"mean\":" << expressiveSummary.meanTimingSamples << ","
               << "\"meanAbsolute\":" << expressiveSummary.meanAbsoluteTimingSamples
               << "},\"velocity\":{";
    configJson << "\"min\":" << expressiveSummary.minimumVelocity << ","
               << "\"max\":" << expressiveSummary.maximumVelocity
               << "},\"pitchMidi\":{";
    configJson << "\"min\":" << expressiveSummary.minimumPitchMidi << ","
               << "\"max\":" << expressiveSummary.maximumPitchMidi
               << "},\"availableDomains\":" << expressiveSummary.availableDomains << ","
               << "\"manifestedDomains\":" << expressiveSummary.manifestedDomains << ","
               << "\"unusedDomains\":" << expressiveSummary.unusedDomains << ","
               << "\"sourceCounts\":[";
    for (std::size_t i = 0; i < expressiveSummary.sourceCounts.size(); ++i) {
        configJson << expressiveSummary.sourceCounts[i];
        if (i + 1 < expressiveSummary.sourceCounts.size()) configJson << ",";
    }
    configJson << "],\"scaleCounts\":[";
    for (std::size_t i = 0; i < expressiveSummary.scaleCounts.size(); ++i) {
        configJson << expressiveSummary.scaleCounts[i];
        if (i + 1 < expressiveSummary.scaleCounts.size()) configJson << ",";
    }
    configJson << "],\"domainCounts\":[";
    for (std::size_t i = 0; i < expressiveSummary.domainCounts.size(); ++i) {
        configJson << expressiveSummary.domainCounts[i];
        if (i + 1 < expressiveSummary.domainCounts.size()) configJson << ",";
    }
    configJson << "],\"causeCounts\":[";
    for (std::size_t i = 0; i < expressiveSummary.causeCounts.size(); ++i) {
        configJson << expressiveSummary.causeCounts[i];
        if (i + 1 < expressiveSummary.causeCounts.size()) configJson << ",";
    }
    configJson << "]},"
               << "\"marketPrices\":{";
    {
        size_t i = 0;
        for (const auto &entry : market.prices()) {
            configJson << "\"" << entry.first << "\":" << entry.second;
            if (++i < market.prices().size()) configJson << ",";
        }
    }
    configJson << "},"
               << "\"tempoTimeline\":[";
    for (size_t i = 0; i < tempoTimeline.size(); ++i) {
        configJson << "{\"t\":" << tempoTimeline[i].atSeconds << ",\"bpm\":" << tempoTimeline[i].bpm << "}";
        if (i + 1 < tempoTimeline.size()) configJson << ",";
    }
    configJson << "],"
               << "\"engine\":\"rasgo-synth-core phase118 integrated (markov generative arc w/ blendable templates + blendable scales (39-scale researched repertoire, seed-picked, no manual dial) + stereo pan + nine independently implemented chaotic maps + morph osc + multiband bloom + original four-channel lfo bank (Kosmische was an early conceptual reference) + buchla leveler/lag + modal resonator + granular cloud + lunetta polyrhythm + tr-808-inspired kick/snare/hihat/cowbell/clap/toms/cymbal/maracas w/ per-piece timbral variation + rhythmic humanization + per-voice tape/tube fx + hocketing + sparse hocket-to-voice cross-influence network + free intonation (chaos-driven departure from the scale) + intensity guarantee (every piece reaches a real limit through at least one risk-taking mechanism) + chamberlin state-variable filter + ks randomizer probability bias + gendyn texture + ay-3-8910 arpeggio + turing-machine-style seq lock/jump + audio-rate self-feedback + cross-fm + colored noise bed + ochd organic lfo ensemble + branches bernoulli hihat routing + rebalanced drum mix + apocryphal tear + seed-derived tempo drift + functional-harmony strong-beat weighting + real tonal-centre modulation + modal shift + bar-level chord progressions + fracture folder (buchla/serge wavefolding pushed past clean periodicity into jittered tearing) + rungler (hordijk/benjolin shift-register pitch instability) + leibniz rectifier (seed-modulated rectification + xaoc-leibniz-inspired bit corruption, resonance-tracked presence) + tuba/breath voice (second modal resonator blown instead of struck, tracks the piece's root note two-to-three octaves down) + chaos contagion (kick/snare decay times jittered per-hit straight from lato/standard chaos maps) + thermal fade (whole-mix ending shaped by the arc's own coda, gain fade + harmonic darkening + static swell, slow power-loss or fast clean cutoff) + rossler collapse (a slow spiral that erupts into a filter-cutoff kick when it swings past threshold during a still-minimal section) + self-epitaph (a plain-text account of the piece's own seed and section path, written into its own trailing RIFF chunk, legible in a text/hex editor, invisible to any audio player) + stochastic mute (rare, millisecond-scale true silences on their own dedicated probability roll, minimum-gap enforced so they never cluster) + per-album output subdirectories + editable title/artist metadata with a persisted artist preset + discrete jump-and-hold tempo change events at arc transitions, 20-220bpm range + coda ritardando (real gradual-slowing glide, not a jump, exponential curve toward a slower target) + a queryable tempo timeline for UI live-bpm display + agon (merit-based mix conflict between four voices with seed-derived power that solidifies over the piece's lived time, triggered only when the melody itself goes stale, each voice ceding in proportion to its own standing rather than a flat split -- a rusga) + elemento estranho (rare, unmotivated raw amplitude-comparator clash between the main voice and the atari texture, deliberately left un-smoothed sample-to-sample -- a genuine foreign intrusion, not another well-mannered mechanism) + foreign signal (real, once-per-render CPU temperature or load-average reading from the actual host machine nudging the latoocarfian map's initial condition -- the one place in this whole engine where the same seed can honestly produce a different piece, exposed plainly in this metadata rather than hidden) + fixed-point overflow (real Q1.15 integer wraparound, the exact way 80s samplers/chips misbehaved on overflow -- bounded in magnitude like every waveshaper here, but deliberately discontinuous rather than smooth, crossfaded in/out over its rare brief window) + spectral ghost (real FFT-domain processing, magnitude kept but phase replaced with an independently, slowly drifting one -- a rare, sustained, unpitched blur genuinely different in kind from every time-domain mechanism above, windowed and overlap-added correctly at 50% hop, verified against an isolated round-trip test before being wired in) + per-piece companion .txt score file (a human-readable account of this specific piece's own trajectory, tempo history, and which optional mechanisms actually manifested, written as a sibling of the WAV) + sample-hold glitch (real sample-rate corruption -- decimation, holding stale values for an irregular chaos-jittered span instead of updating every sample, corrupting the time axis specifically rather than amplitude or frequency) + expanded drum dynamics (hihat joins kick/snare's per-hit chaos contagion on its own chaos source; toms occasionally ratchet 1-3 fast extra hits like a real fill; cymbal occasionally rings out 2-4.5x longer than its usual decay, a per-hit choice not a per-piece constant) + euclidean cowbell (real Bjorklund/Toussaint onset distribution, an alternative to the polyrhythm-divider clock for this one voice -- two of the k values honestly named where the correspondence to a real tradition (tresillo/cinquillo) is exact and verified, the rest left as plain math with no claimed cultural name; kept deliberately sparse (k<=10 of 16) after a user listening pass caught the original wider range making cowbell audibly busier than every other drum voice) + walsh oscillator (a fourth, genuinely distinct synthesis principle: additive combination of orthogonal +-1 Walsh/Hadamard functions, sequency-ordered via bit-reversed Gray code over a closed-form Hadamard matrix -- verified by an isolated test for exact orthogonality and correct sequency ordering before wiring in, since an earlier continuous-sampling attempt looked plausible but failed orthogonality -- density-driven background presence, seed-gated, tracking the main voice's own pitch in unison) + cross-domain contagion (the piece's own recent audio-domain peak, not a slow average, reaching into the CLOCK domain -- one extra pulse fed to the shared XOR clock divider whenever a fast peak follower spikes past a per-piece threshold with a minimum step cooldown elapsed, permanently shifting the phase of every downstream drum pattern rather than nudging a continuous parameter) + sieve-generated experimental scale (Xenakis sieve theory -- a pitch-class set built from the union of a handful of modulus/residue conditions instead of a catalog lookup, rejection-sampled against the real 39-scale repertoire so 'uncatalogued' is a checked property, verified at zero collisions across 2000 draws in an isolated test before wiring in) + real voice leading (mutually exclusive with the existing degree-index jumpiness weighting -- a three-bucket mixture over actual semitone distance from the previous note across several octaves, 80% a 0-2 semitone step, 15% a 3-6 semitone move, 5% a genuine 7-18 semitone leap, DIY-modular Brownian-motion-melody territory rather than the existing exponential scale-degree weighting) + polyphonic cascade (a real Intellijel-Shifty-style shift register for a fraction of hocket-enabled pieces -- every gated note shifts the previous hocket voice's pitch/envelope down into the second hocket oscillator instead of resetting it, so several recent notes genuinely overlap, diode-mixed against each other through the same nonlinearity the main voice uses rather than a plain linear sum) + chaos-addressed step reading (Bastl Popcorn-style: the melodic read pointer's own index is picked by a dedicated chaotic source instead of always advancing 0,1,2...15 in lockstep with the master clock -- percussion/euclidean-cowbell/tom-cycle stay on the untouched linear clock, only the melody's own note lookup becomes an addressable jump table) + neo-riemannian triadic transformations (real chromatic P/L/R over the 24 major/minor triads -- parsimonious voice leading, each operation changes exactly one pitch class of the current triad, verified by an isolated test for the canonical textbook examples/involution property/shared-tone property before wiring in -- a fraction of chord-progression pieces get a wandering triad instead of a fixed roman-numeral list) + drums as a real instrument (a user listening pass across a full album, not a compile check: kick/snare given a higher gain floor since the random per-voice multiplier could roll them too quiet, presence now rides the arc's own density instead of one constant for the whole piece -- receded in quiet passages, true silence below density 0.12, full strength at the climax -- a real fraction of pieces (12%) has no drums at all, and a fraction of the rest (35%) has real per-hit failures, the pattern's own structure staying intact while some hits simply don't sound) + real rhythmic archetypes (Marcha/Soul-backbeat/Metal-blast-beat/Jazz-swing/Samba-inspirado -- explicit 16-step onset grids replacing the XOR clock's kick/snare/hihat triggers for a fraction of pieces, re-drawn at every genuine arc section change so a piece can move between feels within itself, each grounded in a real documented rhythmic characteristic rather than a fabricated cultural correspondence) + accelerando/stringendo (the coda ritardando's own pair at the other end of the arc -- a real exponential glide, armed once when the climax is first reached, easing tempo UP toward a faster target instead of down, yielding final say to the ritardando once the coda genuinely begins) + probability market (the eight most recent seed-gated mechanisms above draw their probability from a persisted, slowly-wandering 'price' -- mean-reverting random walk, bounded 0.03-0.97 -- instead of a fixed constant, the first engine state that genuinely persists between renders outside the already-documented foreign-signal exception; the exact price used for THIS render is exposed plainly in marketPrices below, never hidden) + parenteses (a real reroute of the master output tap to a captured, stuttering loop for a short bar-scale window -- the generative engine underneath keeps computing exactly as it always does, unaware anything is happening, so the piece resumes precisely on schedule when the window closes; distinct from ApocryphalTear's long-distance self-quotation and SampleHoldGlitch's continuous sample-scale decimation, verified by an isolated test of the envelope/capture/loop logic before wiring in) + heisenberg (rhythmic precision and pitch precision coupled INVERSELY for a fraction of pieces via one shared seed-derived balance, instead of the two independent rolls elsewhere -- a piece is never allowed both a tight, mechanical grid AND a locked-to-scale pitch at once, an uncertainty trade-off rather than two unrelated character choices) + vosim (a sixth, genuinely distinct synthesis principle -- discrete decaying sin^2 pips instead of any continuously-excited resonator already in this engine, pip width sets a formant peak independently of pip repetition rate setting the perceived pitch, tracking the main voice's own pitch in unison as a ghostly formant-shadow, verified by an isolated test for periodicity/decay-ratio/silence-gap before wiring in) + chaotic binaural beat (a real frequency divergence between L and R, not just pan/delay-tail differences -- two simple sine shadows, one per channel, uníssono with the main voice but offset by a chaos-modulated few Hz, verifiable objectively in the WAV's own L/R channels rather than depending on playback system/listener perception) + sidechain asphyxia envelope (a real percussion-triggered filter duck -- every kick hit closes the main filter's cutoff almost instantly then lets it recover slowly, the classic sidechain/ducking shape from techno/EDM production, a genuinely different modulation target from the chaos/LFO-driven cutoff already in place) + polivoks filter (a third filter colour alongside the moog ladder and clean chamberlin svf -- deliberately unstable damping that goes slightly negative near the top of its resonance range, caught and bounded by a tanh saturation into a real, sustained, harshly-nonlinear self-oscillation instead of a textbook-clean sine, verified by an isolated stability sweep and a genuine non-decaying self-oscillation test before wiring in -- mutually exclusive with chamberlin svf per piece, same crossfade slot) + vector synthesis (prophet-vs-inspired real 2D crossfade -- a dedicated bank of four fresh instances of existing texture classes (gendyn/walsh/atari punk/colored noise), bilinearly blended by a genuine continuous X/Y position from two already-ticking chaos values, a structurally new modulation topology kept fully separate from the existing independently-mixed instances of those same classes to avoid touching already-validated behaviour) + harmonaig (inspired by the instruo harmonaig four-voice chord module -- a dedicated bank of four fresh sine+envelope voices sounding the piece's own current chord as a sustained pad, root/third/fifth/seventh matching the real module's own voicing, seed-derived inversion and voicing spread, re-voiced every real chord change -- additive layer only for pieces that already have a genuine chord progression running, kept fully separate from the fixed osc1/osc2 cross-fm pair and the sparse hocket oscillators to avoid touching either's already-validated character) + xenakis sieve polyphony (four independent channels, each with its own generated sieve scale and its own chaos-modulated clock -- wood/breath and metal/glass via two fresh modalresonator instances with near-harmonic and clearly inharmonic mode ratios respectively, water via a granular texture with pitch-jitter standing in for a chirp, and a reactive plasma channel whose own scale is the set complement of the other three's union, silent until their currently-sounding pitch classes collide into too consonant a combination, then firing its own closest tone to a true tritone to break it -- contrapuntal texture from statistical collision between independently-clocked channels, not written chords) + bloom (inspired by qu-bit electronix bloom, andrew ikenberry -- a short seed-derived trunk of five semitone offsets, and a branch level built by literally re-applying the trunk's own pattern to itself, a genuine self-similar/recursive structure rather than a scripted variation; a path index walks between trunk and trunk-of-trunk each time the current level completes a full loop, pitches snapped to the piece's own active scale for harmonic consistency) + caillois game weights (four continuous weights -- alea/agon/ilinx/mimicry -- sampled uniformly from the 4-simplex via stick-breaking, biasing eleven already-existing mechanisms in three curated groups instead of adding new isolated ones, a mechanical formalization of this project's own long-standing caillois lens) + xenakis rhythmic sieve (the same modulus/residue congruence theory already used for pitch, fase 61, applied here to points in time within the fixed 16-step drum cycle instead of pitch classes -- rejection-sampled against every named archetype's own kick pattern so 'not one of the catalogued rhythms' is a checked property, the same discipline that caught the pitch sieve's own real collision bug) + aksak additive rhythm (two real Balkan long/short beat-grouping cells, 2+2+2+3 and 2+2+3, concatenated to fill the same 16-step cycle exactly -- Brăiloiu/Dzhudzhev's aksak theory, kick marking every group start, snare doubling the slow/3-count groups specifically) + album game-weight trajectory (album cohesion no longer a shared forced scale -- a straight-line walk across the same caillois 4-simplex from one seed-derived point to another, one interpolated point per track, each track otherwise picking its own scale as freely as a standalone piece) + curto-circuito (a single real hard-gate dropout per piece, ~3ms linear ramps around true silence rather than any fade curve, a noise 'spark' riding only the two transition edges -- paired at the same instant with a curated handful of mechanisms the seed had already rolled OFF for this piece getting a real, elevated second chance to force their way in anyway, the exact same forced-activation idiom the fase-40 intensity guarantee already established, just invoked by a mid-piece rupture instead of a start-of-piece safety net, biased by ilinx)) + generalized rhythm generation stage 1 (two more generated-rhythm slots alongside the fixed catalog and the xenakis sieve: a generalized aksak grouping -- braniloiu/dzhudzhev additive theory extended from the two catalogued fixed cells to the full real space of long(3)/short(2) group orderings summing to 16, two or four long beats -- and a generalized euclidean pattern -- bjorklund/toussaint E(k,16) for any k from 2 to 11 with a free rotation, extended from the single curated cowbell-only use to any voice and any piece; first stage of a larger, staged rhythm-generation architecture, metrical-hierarchy weighting (lerdahl and jackendoff's generative theory of tonal music, computed as the 2-adic valuation of each step -- the real 'ruler sequence', oeis a001511 -- since binary metrical subdivision of a 16-step bar literally is that structure) implemented and unit-verified, now wired into dynamics) + generalized rhythm generation stage 2 (real continuous per-hit dynamics -- every drum voice's own last-triggered velocity, built from the stage-1 metrical weight plus arc density plus a phrase-level gesture, multiplying that voice's mix contribution instead of one flat per-piece gain; gesture itself reads an already-running ochd lfo channel once every four bars and holds it steady, standing in for the way a real player pushes or drags a whole phrase rather than wobbling note to note, timing-nudging every archetype-driven hit alongside the existing swing/humanization; repetition with difference in the deleuze sense already named in this project's own manifesto -- rather than mutating an archetype's own defining kick/snare pattern, every real repeat of the 16-step cycle gets a small bounded chance of one extra quiet ghost hihat at a freshly-chosen step, so no two repeats of the same identity ever sound perfectly identical) + generalized rhythm generation stage 3 (H: orchestration -- which of the eight percussion voices are actually 'in' a given section, re-decided at the same genuine arc-section boundary that already changes archetype, kick and hihat weighted as anchor voices real drumming rarely drops together, how many of the rest join scaling with the arc's own density; F: that same density now also scales how often repetition-with-difference's ghost hihat fires, drier in calm sections, more insistent near the climax; G: a real rhythmic aftermath for curto-circuito -- the full kit forced back in for two bars the instant the hard-gate finishes reopening, empirically confirmed to fire exactly when curto-circuito itself fires and never otherwise; a parallel full-kit override during an agon rusga, the same idiom answering a different real cause, a melodic power conflict instead of a rupture; cymbal and maracas retired from chaos zero-crossing triggering entirely and moved onto their own dedicated generalized-euclidean patterns from stage 1, a rare accent and a close subdivision-riding shaker respectively, both real checkable onset patterns instead of an emergent side effect of an unrelated chaos map's own period)) + theremin voice (a continuous, glide-only sine shadow of the melody's own current pitch that never re-triggers like every other voice here -- the one thing that actually makes something read as theremin rather than portamento synth, real hand-tremor vibrato via an already-existing lfo class, not a new oscillator) + corte de montagem (a real film-editing jump cut translated into audio -- an instantaneous splice to a genuinely distant moment of this same render's own past, played once straight through with no reverse/bit-crush/loop, then an instantaneous splice back to wherever the live signal has gone by then; both edges land on real zero-crossings found in the already-rendered history and in the live signal's own next zero-crossing, the same real technique analog tape editors used a razor blade for, so a genuinely instant cut never clicks; distinct from apocryphal tear's warped blended quotation, parenteses' recent looped stutter, and curto-circuito's real silence) + real meter change (a section's bar length can genuinely differ from the engine's default 16 steps -- 12/20/14, real compound/asymmetric meters at the same sixteenth-note rate, chosen at a genuine arc-section boundary; only the three generated rhythm slots from stage 1 above are eligible once the meter isn't 16, since the fixed named-archetype catalog was hand-drawn specifically for that length; the melodic step sequencer's own note grid resizes along with it, preserving its rng stream/scale/lock amount rather than resetting; metrical-hierarchy dynamics fall back to a real grouped-meter model (still lerdahl and jackendoff, just the compound-meter case their same theory already covers) whenever the bar isn't a power of two; cowbell/clap/tom/cymbal/maracas deliberately keep running at the engine's own default 16-step cycle regardless, an honest, real polymeter rather than an attempt to also stretch every other voice)) + atropelo (began as 'voz narrante' and grew, through design conversation with the user, into competing almost-voices instead -- real source-filter formant synthesis with three resonances deliberately shifted outside natural human vowel-formant space, so the result triggers the ear's speech pattern-matching and refuses to resolve into any real vowel; a real conflict idiom (same lineage as agon) where losing means a genuine hard interruption -- a click-free cut at the loser's own next zero-crossing, not a graceful mix cession; the first mechanism in this whole engine that remembers an interrupted phrase's exact unfinished state -- register, syllable position, and the precise point along its own intonation contour -- across non-adjacent activations within the same piece, may probabilistically resume that exact phrase later continuing the same glide, or may genuinely forget it if the small memory pool fills up first; bounded recursive nesting up to three levels reusing bloom's own self-application principle, each deeper level less likely than the last, never resolving; and a deliberate act of trespass -- forcing a real event the instant stochastic mute's own promised silence opens, invading a rest the engine elsewhere guarantees rather than coexisting peacefully with it)) + dual vca polivoks-architecture inspired (a real bipolar differential-pair transfer curve -- tanh of drive, the textbook translinear-circuit large-signal equation, not any specific schematic's code -- replacing an idealized linear multiply on the two hocketing voices own gain stage; quiet passages stay close to linear, loud ones compress/saturate against the pair own thermal-voltage-like limit instead of scaling forever, the real character a differential-pair vca has that a clean multiplier-based one does not; verified by an isolated test for near-linearity at low level, real compression at high level, bounded output under extreme drive, and both channels sharing the identical curve (a matched pair, not two independently-behaving vcas) -- mutually exclusive coloring choice, independent roll from hocketing itself) + acid bassline voice (tb-303-style all-in-one voice -- the first combination in this engine of the polivoks filter own self-oscillating character (fase 75) with its own dedicated fast filter envelope, plus the two real per-step performance controls a classic acid pattern is built from: accent (louder, deeper filter sweep) and slide (the next note ties to this one, no envelope retrigger, pitch glides continuously instead of jumping) -- verified by an isolated test confirming slide genuinely skips the retrigger and glides to the target rather than jumping, accent is genuinely louder, output stays bounded; a fixed 16-step pattern generated once per piece from the piece own catalog scale two octaves below the root, kept on its own honest polymeter cycle regardless of the melodic sequencer own bar length exactly like cowbell/clap/tom/cymbal/maracas already are; mixed at a real foreground presence level, the same bucket as the tuba voice, not a background texture) + envelope loop pulse (polivoks eg loop + end-of-attack functions, added to the shared envelope class itself and verified to leave every existing caller own default adsr behaviour completely unchanged unless explicitly opted into -- loop turns decay reaching its target into an automatic re-trigger of attack instead of settling into sustain, a real self-cycling ad source needing no external note-on, and end-of-attack is a genuine one-sample-wide pulse output at the exact instant attack completes, verified isolated against spurious re-firing; the only mechanism in this whole engine that self-triggers from an envelope completing its own cycle rather than any note/step/chaos-driven logic, patched here into striking its own short sine blip on every end-of-attack pulse, running on its own independent period unlocked from the beat) + hard sync vco (another genuinely distinct synthesis principle -- a slave oscillator whose phase is forcibly torn back to zero every time a separate master oscillator completes a cycle, regardless of where the slave own phase currently sits; output pitch stays locked to the master frequency -- verified by an isolated test confirming the output repeats exactly at the master own period even at non-integer slave/master ratios that would never be periodic free-running -- while the slave/master ratio itself is continuously swept by an already-ticking chaos value, giving the real hard-sync sweep character without any new chaos state; master tracks the main voice own pitch in unison like walsh/vosim above, same density-driven background bucket, not one of agon four named voices) + modo selvagem (an explicit, opt-in, honestly-disclosed break from this whole engine own same-seed-same-piece promise -- reseeding the shared rng with real std::random_device entropy at every bar boundary when the user turns this on, distinct from the still-planned hardware Performance console own live unpredictability; UI toggle deliberately colored as a warning) + real per-piece noise seeding (a genuine pre-existing bug, found while verifying modo selvagem own correctness: three drum voices noise bursts -- snare rattle, clap, maracas -- were drawing from the global C std::rand(), state that persists for the whole process rather than per render, so a second piece composed in the same long-running app session without a restart silently diverged from what the same seed should have produced, even with modo selvagem off; confirmed via a fresh-process/same-process comparison harness -- fresh-process renders of the same seed were always identical, only the same-process repeat diverged, which narrowed it to process-lifetime state rather than anything seed- or render-scoped; the noise oscillator now carries its own seeded rng, seeded from the render own seed-derived stream like every other stochastic choice here)) + nota de rodape (a short, isolated 2-5 note gesture, at most once per piece, armed only at a weak metrical position (fase 85's own metrical-strength weighting) and pitched entirely from the chromatic complement of the piece own active scale (the fase-78 plasma-channel technique, reused here for pitch selection only) -- every note it plays is, by construction, outside this piece own harmonic vocabulary; two deliberately inharmonic sine partials for its own small odd timbre, its own dedicated reverb pair (bigger room, more mix than anywhere else in the piece) so it reads as spatially distinct too; once armed its own note-to-note pacing runs on its own timer, no longer following the step clock that started it -- a private aside that stops keeping the piece own beat once it starts speaking)) + galo (a stylized synthesized rooster call, honestly labeled as synthesis rather than any claim of bioacoustic accuracy, registered from an analysis of the user own suno album -- reuses the already-validated modalresonator (fase 43/78) and breathexciter (the tuba voice own continuous exciter) driven by a real frequency-contour chirp, a pure testable function of elapsed time: several rising syllables followed by one longer sustained note with a genuine falling tail, verified isolated for monotonic rise then real fall and a sane bounded ratio range before wiring in; deliberately foreign to the piece own scale and register, same elemento-estranho spirit as a found-sound-like intrusion rather than a well-mannered layer; fires at most once per piece, same single-real-event idiom curto-circuito already established, excluded from the coda for the same reason)) + scat voice (the realistic half of 'real vocal synthesis', scoped down after weighing full intelligible speech against what is honestly achievable without a real voice recording or a trained ml model -- real source-filter formant synthesis, same documented technique as atropelo, fase 90, but tuned to peterson and barney own 1952 documented vowel-formant values instead of atropelo deliberately uncanny off-vowel placement; verified isolated for f1<f2<f3 ordering and a real documented vowel-frequency range on all five vowels, bounded output, correct syllable duration, working retrigger; only active alongside a genuine chord progression, same gating convention harmonaig already uses, so every syllable genuinely outlines this piece own harmony instead of an arbitrary pitch; wordless and meaning-free by honest design and limitation, not intelligible words -- a real, simple noise-burst consonant onset stands in for full articulatory phonetics, which stays out of scope)) + coral (real polyphony built from four scat voices in satb-like registers -- soprano/alto/tenor/bass, each voice-led into its own chord tone across a couple of octaves so the choir sings real vertical harmony instead of four voices doubling one pitch; triggered on the same real chord-change event harmonaig already uses, sharing one vowel across all four so it reads as one choir rather than four unrelated voices, but each voice carries its own onset-timing jitter (0-50ms) and dynamics variation -- the real imperfect-ensemble character a human choir has, singers never landing in perfect unison or identical loudness; verified isolated for bounded output across a real span of time and stability across twenty different root/vowel/chord trials; independent gating from the scat solo voice above, both mutually possible in the same piece)) + arquivo apocrifo aprofundado (a real design conversation with the user grounded in philosophical material on the concept of the apocryphal archive -- derrida/foucault/agamben/ricoeur/benjamin -- translated into three concrete, checkable pieces: linhagem/proveniencia, a new self-epitaph section listing, only for mechanisms that genuinely fired in THIS piece, the real external reference each one derives from, making explicit that the seed commands but never originates -- it activates a path through a genealogy it did not create, and the self-epitaph now says so plainly, naming this engine own version alongside the seed to acknowledge that even that command is historically situated, not eternal; deja vu, a real audio mechanism synthesizing a false memory from running statistical averages of the piece own recent pitch/brightness/density rather than replaying any real captured buffer -- distinct in kind from apocryphaltear (fase 25), which quotes something that genuinely happened; and leitura apocrifa, an explicitly-labeled counterfactual passage in the self-epitaph describing a real near-miss the piece own generative process had (a probability roll that came close to a mechanism own threshold but did not cross it), benjamin own 'arquivo dos vencidos' applied to the piece own discarded rolls -- never touching or contradicting the factual account elsewhere, fiction declared as fiction rather than a lie inserted into the record)) + ending fade rework (start/end safety fade was a flat 20ms/250ms click-insurance value regardless of the mix's own level; now seed-varied and, for the ~60% of pieces without the thermal-death coda, long enough to read as a real musical wind-down instead of an abrupt cut) + drum archetype price raised 0.35->0.6 (a 40-seed pattern audit found the generic pre-fase-67 fallback drum logic used in 80% of pieces, the real source of cross-seed rhythmic sameness the user reported -- task #29) + objeto de melodia/flauta (task #31, real 2-operator FM synthesis phrasing over its own independent harmonic engine -- one of six real, documented harmonic-movement techniques (Coltrane changes, tritone substitution, non-functional chromatic mediants, modal interchange, modal jazz, backdoor ii-V), never depends on the base sequencer's own chord progression, which is only active in ~50% of pieces by design) + voicing bank (task #31 Eixo B, MelodyVoicingBank.hpp -- five real vertical-structure principles under the flute's phrases, independent of Eixo A's harmonic movement: quartal (McCoy Tyner/Tigran Hamasyan, no 3rd so the same shape reads ambiguously major or minor, seventh seed-varied between real minor 7th and major 7th), parallel planing (Debussy, a fixed augmented-triad shape moved rigidly with the key centre), bitonal (Stravinsky/Milhaud, two triads a tritone apart), classical triad (control condition, real scale-degree 3rd/5th), and Armenian modal (a real drone -- the duduk's own dam drone -- held at the piece's original root under an open 5th of the current key centre plus a quartal colour tone, the same maj7/min7 ambiguity fused with the drone/modal idiom Komitas Vardapet's own folk collection is built on, the real jazz-modal fusion Tigran Hamasyan's own practice sounds like)) + dissonance treatment (task #31 Eixo C, DissonanceTreatment.hpp -- four real ways each new phrase note relates to the scale degree it was drawn from, independent of Eixo A and Eixo B: tone-as-colour (the control condition, real modal-jazz/impressionist stance -- extensions never need resolving), a real bounded chance of an unresolved chromatic clash a semitone off the real target (Stravinsky/Poulenc), a real bebop chromatic approach tone landing ON the beat before resolving to the target a fraction later, and a real classical suspension holding the previous note's own pitch past the moment a new one was due before resolving down to the target)) + sequencer wild timing (task #30 -- the rhythmic/melodic pattern regeneration, always locked to a perfectly regular once-per-bar clock before, now optionally fires on a real zero-crossing of a dedicated chaotic map instead, gated by a real minimum gap -- a pattern can hold for many bars building real expectation, or refresh twice within a single bar, genuine irregular timing driven by real chaotic dynamics rather than an arbitrary probability roll, while the chord progression keeps its own steady one-per-bar harmonic anchor regardless) + real dead-air fix (a bug the user caught by ear: thermalAbruptDeath -- fase 45's abrupt-cutoff thermal-fade variant, tau as low as 0.15s -- crushed the mix to near silence within 1-2 real seconds of the coda starting exactly as intended, but the coda's own section length is rolled independently (8-20s) with no idea the mix already went silent, so the render kept going for however many real seconds were left on that unrelated roll -- several seconds of genuine dead air tacked onto an ending that was supposed to read as abrupt, not silent-then-nothing. fixed by actually ending the render once the abrupt gain has genuinely decayed away, confirmed empirically: the worst measured case dropped from 87% silent in the final 15 real seconds to 2%) + Atari Punk Console wild clock (task #30, user suggestion: give this voice's own real chaotic clock hiccups -- Forrest Mims Stepped Tone Generators real circuit-bent behaviour when pushed hard, the two 555 timers hiccuping to an unrelated division/multiple for a moment -- instead of always tracking the melody pitch smoothly, gated by the same zero-crossing chaotic event idiom as the sequencer wild timing above) + expanded chord progression catalog (task #29 -- an audit found the sequencer only had 6 fixed progressions, so the same one -- often the two most cliche in pop/rock harmony -- recurred constantly across different pieces; the original 6 stay in place for reproducibility, 8 more real and genuinely distinct progressions added -- a real jazz turnaround and its rotation, a real descending circle-of-fifths fragment through the leading-tone diminished degree that never appeared at all before, a real stepwise descending bass walk, and Axis-progression rotations starting on a different chord so the first beat a listener anchors to varies too, not just which four chords cycle underneath) + phasing (Steve Reich -- Piano Phase/Clapping Music: a second voice reads the exact same live sequencer pattern as the main voice but on its own clock, 0.6%-2.5% faster or slower; the phase relationship between the two identical lines drifts continuously through every offset and, over a typical piece's length, usually back toward alignment -- built from two independent clocks genuinely disagreeing, not from any harmonic or rhythmic device already in this engine) + microtonality (real historical tuning systems -- 5-limit just intonation, Ptolemy/Partch, and Pythagorean 3-limit pure fifths -- retuning the main voice's own baseFreq in real cents relative to the piece's own tonic, plus an original extension of the same Xenakis sieve technique already used for 12-tone scale generation (fase 61) to a finer equal division of the octave (19/22/31-EDO), generating a genuinely new per-piece microtonal inflection instead of copying a historical table; just and pythagorean tunings additionally accumulate real comma drift across the piece's own genuine tonal modulations -- chaining each modulation's pure-ratio interval instead of resetting to equal temperament, the same real phenomenon that makes a cappella just-intonation ensembles drift in absolute pitch over a long piece, reproduced here as an honest mathematical consequence rather than a scripted effect; a fourth, quarter-tone 24-EDO option (Haba/Wyschnegradsky) rounds out the choice, deliberately not claimed as a reproduction of any real maqam's own non-equal intonation) + spectral object (a single synthesized partial-number DNA -- full harmonic series, odd-only closed-tube series, or a prime-weighted subset, real acoustic/number-theoretic rationale for each -- driving pitch AND rhythm from the SAME integers: partial p sounds p times faster than partial 1, not two separate systems; partial ratios stretch from pure integer multiples toward a real audible departure via the same inharmonicity formula documented for piano strings and bells (Fletcher & Rossing, f_p=f0*p*sqrt(1+B*p^2)), driven by the arc's own density so the piece's form directly pilots acoustic stability; which partials sound at all is gated by the same Xenakis sieve modulus/residue technique already used for pitch-class scales (fase 61) and microtonal tuning (fase 112), now applied to which harmonics are present instead of which pitches) + polymeter/metric-modulation object (metric as a real system of time organization, not just which time signature -- four percussion voices with genuinely different, pairwise-coprime cycle lengths in real steps (not shared-clock XOR divisions like the existing polyrhythm divider), each realigning at a real computable macroperiod (the true LCM of the chosen lengths), plus a real non-isochronous pulse inside each voice's own cycle (Justin London's documented non-isochronous meter -- freely unequal onset gaps summing exactly to the cycle length, distinct from this engine's own fixed-cell aksak); and a real Elliott-Carter-style metric modulation mode for the existing tempo-change mechanism, picking the change ratio from real small-tuplet proportions (3:2, 4:3, 5:4 and their inverses) instead of an arbitrary continuous multiplier, so a tempo change reads as a genuine common-subdivision relationship rather than an unmotivated jump) + bluff signal (game-as-composition, not gamification -- a genuine deception mechanism, per the real definition treated as its material: incomplete information plus expectation plus risk. A signal sounds identically every time it fires, genuine or bluff -- real incomplete information, no way to tell which by ear alone; each firing is internally decided genuine or bluff by a per-piece truthfulness rate, a genuine signal is followed after a real expectation window by a distinct confirmation gesture, a bluff by silence; a third respondent voice answers every new signal at a gain scaled by real accumulated trust (an exponential moving average of past outcomes, updated only when each signal resolves), and goes silent for good after a real betrayal -- trust that was genuinely built (crossed a real threshold) and then collapsed, since betrayal presupposes prior trust rather than being definable from bluffing alone; an optional album-wide truthfulness trajectory lets a whole album's own reliability arc drift track to track instead of each piece rolling independently) + rasgo espectral (a real fiber bundle model -- Pierce 1926, Daniels 1945's founding statistical theory of bundled-filament strength -- applied to the spectral object's own 6 partials instead of a generic glitch: each partial is a fiber with a real Weibull-distributed breaking threshold, a real ambient stress grows with the arc's own climax proximity, and when a fiber's accumulated load exceeds its threshold it tears -- using local load sharing, Harlow and Phoenix 1978, the physically correct model for directional fracture propagation through an ordered structure, so released load goes only to the immediate still-intact neighbors and can cascade into a real, unscripted avalanche; the fundamental fiber carries effectively infinite resistance, a real permanent survivor thread through any tear; once a fiber tears it never recovers, a permanent self-looping noise texture at that partial's own real frequency) + coherence collapse (rupture by excess coherence, not chaos -- the inverse of every other instability mechanism in this engine, which all push toward MORE variety: real Shannon entropy, Shannon 1948, over the last 24 pitch classes the main voice actually played; when the piece stays too coherent/repetitive for real consecutive bars, it suffers a genuine collapse -- an audible crack plus a temporary forced reorganization of the sequencer at high density and extreme register, reusing only the sequencer's own already-public API, then settling back to normal behavior after a real cooldown)\""
               << "}";

    // The exhaustive engine genealogy below is intentionally append-only and
    // still begins with the version in which that register was created.  The
    // emitted metadata, however, must identify the renderer that produced this
    // particular file rather than exposing that historical literal as the
    // current version.
    std::string configJsonText = configJson.str();
    const std::string historicalVersion = "rasgo-synth-core phase118 integrated";
    if (const size_t versionAt = configJsonText.find(historicalVersion);
        versionAt != std::string::npos) {
        configJsonText.replace(versionAt, historicalVersion.size(),
                               "rasgo-synth-core phase136 integrated");
    }
    const std::string oldCatalogLabel = "39-scale";
    size_t catalogAt = 0;
    while ((catalogAt = configJsonText.find(oldCatalogLabel, catalogAt))
           != std::string::npos) {
        configJsonText.replace(catalogAt, oldCatalogLabel.size(), "45-scale");
        catalogAt += 9;
    }

    WavMetadata meta;
    meta.title = !cfg.titleOverride.empty() ? cfg.titleOverride : "Rasgo Synth Core - integrated piece";
    meta.artist = !cfg.artistOverride.empty() ? cfg.artistOverride : "Lucio de Araujo";
    meta.comment = std::move(configJsonText);
    if (selfEpitaphEnabled) {
        std::ostringstream epitaphOs;
        epitaphOs << "Rasgo Synth -- uma vida entre infinitas possiveis a partir da mesma origem.\n"
                  << "Semente: " << cfg.seed << ". Motor: phase136.\n"
                  << "Percurso: ";
        for (size_t i = 0; i < visitedSections.size(); ++i) {
            epitaphOs << visitedSections[i];
            if (i + 1 < visitedSections.size()) epitaphOs << " -> ";
        }
        epitaphOs << ".\n";

        // Fase 101 -- "arquivo apocrifo aprofundado": conversa de design
        // com o usuario a partir de material filosofico real sobre o
        // conceito de arquivo apocrifo (Derrida/Foucault/Agamben/Ricoeur/
        // Benjamin). Linhagem/Proveniencia: pra cada mecanismo que
        // genuinamente disparou NESTA peca, sua referencia real -- torna
        // explicito que a seed e comando (decide o que acontece), nunca
        // origem (nao inventa nada, so ativa um caminho por uma
        // genealogia que ela nao criou). Curado, nao exaustivo -- so os
        // mecanismos com atribuicao mais clara e citavel.
        {
            std::vector<std::string> linhagem;
            auto cite = [&](bool active, const std::string &text) { if (active) linhagem.push_back(text); };
            cite(polivoksEnabled, "filtro Polivoks -- sintetizador sovietico Polivoks");
            cite(polivoksVcaEnabled, "Dual VCA -- equacao classica de par diferencial bipolar (Gilbert, circuitos translineares)");
            cite(hardSyncEnabled, "hard sync -- VCO Sync de sintetizadores analogicos reais (ex: Polivoks VCO3)");
            cite(acidBasslineEnabled, "voz acid -- arquitetura Roland TB-303");
            cite(scatEnabled, "voz de scat -- formantes vocalicos documentados por Peterson & Barney (1952)");
            cite(choirEnabled, "coral -- reaproveita a voz de scat, arquitetura vocal SATB");
            cite(galoEnabled, "chamado de Galo -- estilizacao sintetica, nao reconstrucao bioacustica");
            cite(walshOscEnabled, "oscilador Walsh -- funcoes ortogonais de Walsh/Hadamard");
            cite(vosimEnabled, "VOSIM -- sintese de Kaegi & Tempelaars");
            cite(tubaVoiceEnabled, "voz de tuba -- distincao Blow/Strike de excitacao (Mutable Instruments Elements)");
            cite(thereminEnabled, "voz de theremin -- heterodinagem de RF do theremin real");
            cite(harmonaigEnabled, "Harmonaig -- modulo real da Instruo (Jason Lim)");
            cite(sievePolyphonyEnabled, "polifonia de peneiras -- teoria de peneiras de Xenakis");
            cite(bloomEnabled, "Bloom -- modulo real da Qu-Bit Electronix (Andrew Ikenberry)");
            cite(runglerEnabled, "Rungler -- shift register de Rob Hordijk (Benjolin)");
            cite(leibnizEnabled, "Leibniz Rectifier -- Xaoc Devices Leibniz");
            cite(euclideanCowbellEnabled, "cowbell euclidiano -- algoritmo de Bjorklund/Toussaint");
            cite(vectorSynthEnabled, "Vector Synthesis -- Sequential Circuits Prophet VS");
            cite(egLoopPulseEnabled, "envelope em loop -- funcao EG do Polivoks");
            cite(sequencer.neoRiemannianEnabled(), "transformacoes Neo-Riemannianas -- teoria musical P/L/R");
            if (!linhagem.empty()) {
                epitaphOs << "Linhagem (referencias reais dos mecanismos que dispararam aqui, nao invencao desta peca):\n";
                for (size_t i = 0; i < linhagem.size(); ++i) {
                    epitaphOs << "  - " << linhagem[i] << "\n";
                }
            }
            epitaphOs << "Esta seed, sob o motor phase101, comandou esta peca especifica; sob outra versao, "
                      << "a mesma seed comandaria outra coisa inteiramente -- a seed e comando, nunca origem.\n";
        }

        // Leitura apocrifa: um paragrafo CONTRAFACTUAL, explicitamente
        // rotulado como tal -- nunca uma mentira inserida no relato
        // factual acima, que continua intocado. Descreve um caminho real
        // que a peca quase tomou (Benjamin, "arquivo dos vencidos") --
        // uma rolagem de probabilidade que chegou perto do limiar mas
        // nao passou, capturada sem consumir nenhum sorteio extra do rng.
        if (!apocryphalNearMisses.empty()) {
            const auto &nearest = apocryphalNearMisses.front();
            epitaphOs << "\nLeitura apocrifa (contrafactual, nao aconteceu -- a peca quase tomou este caminho): "
                      << "se a moeda tivesse caido diferente por uma margem pequena, esta peca teria armado "
                      << nearest.mechanism << ". Nao aconteceu. Mas quase.\n";
        }

        epitaphOs << "Se arrisca, e soa.";
        meta.epitaph = epitaphOs.str();
    }

    // "Partitura" companion text -- a human-readable account of this
    // specific piece's own history (trajectory, tempo, which of the many
    // optional mechanisms actually manifested), written as a sibling .txt
    // next to the WAV. Deliberately prose-formatted, not another JSON dump
    // (the JSON index already exists for machine consumption) -- this one
    // is for a person to read directly, "como se fosse uma partitura da
    // música". Only lists mechanisms that actually fired/were enabled for
    // THIS piece, not every possible flag regardless of relevance -- a
    // biography of what happened, not an exhaustive parameter dump.
    auto formatMinSec = [](double seconds) {
        int totalSec = static_cast<int>(seconds + 0.5);
        std::ostringstream os;
        os << (totalSec / 60) << ":" << std::setfill('0') << std::setw(2) << (totalSec % 60);
        return os.str();
    };

    std::ostringstream score;
    score << "================================================================\n"
          << "RASGO SYNTH -- partitura gerativa\n"
          << "================================================================\n\n"
          << "Titulo:      " << meta.title << "\n"
          << "Artista:     " << meta.artist << "\n"
          << "Semente:     " << cfg.seed << "\n"
          << "Linhagem:    " << (cfg.seedLineageEnabled ? "ativa" : "seed original")
          << " | pais " << cfg.seedLineageParentA << "/" << cfg.seedLineageParentB
          << " | geracao " << cfg.seedLineageGeneration
          << " | B " << cfg.seedLineageParentBWeight
          << " | mutacao " << cfg.seedLineageMutationProbability
          << " | mascaras B/blend/mut " << cfg.seedLineageInheritedBMask
          << "/" << cfg.seedLineageBlendedMask
          << "/" << cfg.seedLineageMutationMask << "\n"
          << "Motor:       phase136\n"
          << "Andamento inicial: " << cfg.bpm << " bpm\n"
          << "Identidade:  foco " << materialFocusName(identity.focus)
          << ", voz " << leadVoiceModelName(identity.leadVoice)
          << ", harmonia " << harmonicLanguageName(identity.harmonicLanguage)
          << ", conjunto " << ensembleModelName(identity.ensembleModel) << "\n"
          << "Escala inicial: " << scaleName(initialScale)
          << " (familia " << scaleFamilyName(identity.scaleFamily) << ")\n"
          << "Escala final:   " << sequencer.scaleDescription() << "\n"
          << "Contorno:       "
          << (sequencer.melodicContourDirection() > 0 ? "ascendente" : "descendente")
          << " / ápice no compasso " << sequencer.melodicContourApexBar() << "\n"
          << "Duracao:     " << formatMinSec(arc.elapsedSeconds()) << "\n"
          << "Jogo (Caillois): Alea " << std::fixed << std::setprecision(2) << gameWeightAlea
          << " / Agon " << gameWeightAgon << " / Ilinx " << gameWeightIlinx
          << " / Mimicry " << gameWeightMimicry << "\n"
          << "Estrategia:  " << gameStrategyName(gamePlan.strategy)
          << " | regra: " << gameRuleName(gamePlan.rule) << "\n"
          << "Recursos:    energia " << gamePlan.energy
          << " / memoria " << gamePlan.memory
          << " / risco " << gamePlan.risk
          << " / liberdade " << gamePlan.freedom
          << " / informacao oculta " << gamePlan.hiddenInformation << "\n"
          << "Rasgar:      movimento " << rasgarMotionName(rasgarGesture.motion())
          << " / " << rasgarGesture.tearCount() << " cismo(s)"
          << " / desvio medio de pulso " << rasgarGesture.meanPulseDeviation() << "\n"
          << "Cartografia: " << topology.regions().size() << " regioes / "
          << topology.passages().size() << " passagens / "
          << topology.traversalCount() << " travessias / "
          << moduleArchive.size() << " paginas de arquivo\n"
          << "Arquiviagem: " << moduleArchive.count(ArchiveAction::Read) << " leitura(s) / "
          << moduleArchive.count(ArchiveAction::Reread) << " releitura(s) / "
          << moduleArchive.count(ArchiveAction::Reinterpret) << " reinterpretacao(oes) / "
          << moduleArchive.count(ArchiveAction::Recreate) << " recriacao(oes)\n"
          << "Agencia:     ";
    for (std::size_t i = 0; i < materialAgency.foregroundHistory().size(); ++i) {
        score << materialAgency.foregroundHistory()[i];
        const auto &claimants = materialAgency.claimantHistory()[i];
        if (claimants.size() > 1) {
            score << "[";
            for (std::size_t j = 0; j < claimants.size(); ++j) {
                score << agencyFamilyName(claimants[j]);
                if (j + 1 < claimants.size()) score << "+";
            }
            score << "]";
        }
        if (i + 1 < materialAgency.foregroundHistory().size()) score << " -> ";
    }
    score << " (" << materialAgency.foregroundChangeCount() << " mudanca(s), "
          << materialAgency.leaderlessSectionCount() << " trecho(s) sem reivindicacao, "
          << materialAgency.pluralSectionCount() << " trecho(s) plural(is), "
          << materialAgency.relationalReactionCount() << " reacao(oes) entre familias)\n"
          << "Contribuicoes pre-espaco:\n";
    for (int family = 0; family < static_cast<int>(AgencyFamily::Count); ++family) {
        const auto agencyFamily = static_cast<AgencyFamily>(family);
        score << "  " << agencyFamilyName(agencyFamily)
              << " -- RMS " << agencyMeter.rms(agencyFamily)
              << " / pico " << agencyMeter.peak(agencyFamily)
              << " / audivel " << agencyMeter.audibleRatio(agencyFamily) * 100.0 << "%\n";
    }
    if (organismAudibility.enabled()) {
        score << "Ecologia movel de audibilidade: "
              << organismAudibility.movementCount() << " movimento(s), "
              << organismAudibility.emergenceCount() << " abertura(s) e "
              << organismAudibility.yieldCount() << " recuo(s); "
              << "ganho move presenca, nunca dispara nem obriga um modulo a tocar.\n"
              << "Respostas de papel ao conflito espectral: "
              << organismAudibility.spectralRoleResponseCount()
              << " (agudos cedem localmente; fundacao recebe abertura limitada).\n";
        score << std::setprecision(3);
        for (int stem = 0; stem < static_cast<int>(OrganismStem::Count); ++stem) {
            const auto organism = static_cast<OrganismStem>(stem);
            if (!audibilityActive[static_cast<std::size_t>(stem)]) continue;
            score << "  " << organismStemName(organism) << " -- ganho "
                  << organismAudibility.minimumGain(organism) << ".."
                  << organismAudibility.maximumGain(organism) << "\n";
        }
        score << std::setprecision(2);
    }
    if (organismStemAudit.enabled()) {
        score << std::setprecision(6)
              << "Auditoria de organismos pos-espaco (erro maximo de reconstrucao "
              << organismStemAudit.maxPostReconstructionError() << "):\n";
        for (int stem = 0; stem < static_cast<int>(OrganismStem::Count); ++stem) {
            const auto organism = static_cast<OrganismStem>(stem);
            const auto metrics = organismStemAudit.metrics(organism);
            if (metrics.preSpaceRms <= 1.0e-7
                && organism != OrganismStem::Other) continue;
            score << "  " << organismStemName(organism)
                  << " -- pre RMS " << metrics.preSpaceRms
                  << " / pos RMS " << metrics.postSpaceRms
                  << " / ocupacao " << metrics.preAudibleRatio * 100.0 << "% -> "
                  << metrics.postAudibleRatio * 100.0 << "%"
                  << " / soterrado por nivel " << metrics.levelBuriedRatio * 100.0 << "%"
                  << " / parcela " << metrics.energyShare * 100.0 << "%\n";
        }
        score << std::setprecision(2);
    }
    if (relationalOrchestraActive) {
        score << "Orquestra relacional (" << relationVerbName(relationalOrchestra.verb()) << "): "
              << relationalOrchestra.rhythmObservationCount() << " barra(s) de bateria ouvida(s), "
              << relationalOrchestra.thereminResponseCount() << " resposta(s) de theremin, "
              << relationalOrchestra.harmonyResponseCount() << " interpretacao(oes) harmonica(s), "
              << relationalAppliedHarmonyShiftCount << " reconfiguracao(oes), "
              << relationalOrchestra.resistanceCount() << " resistencia(s) e "
              << relationalOrchestra.rhythmFeedbackCount() << " retorno(s) ao ritmo; "
              << relationalOrchestra.thereminPresentBarCount() << " barra(s) em presenca, "
              << relationalOrchestra.thereminSilentBarCount() << " em latencia, "
              << relationalOrchestra.thereminEntryCount() << " entrada(s) e "
              << relationalOrchestra.thereminWithdrawalCount() << " retirada(s).\n"
              << "  Pitch do theremin: " << relationalThereminPitchChangeCount
              << " mudanca(s), " << relationalThereminRepeatedPitchCount
              << " repeticao(oes), maior serie "
              << relationalThereminLongestRepeatedBars << " barra(s), registro "
              << relationalThereminPitchMin << ".."
              << relationalThereminPitchMax << " MIDI.\n";
    } else {
        score << "Orquestra relacional: indisponivel nesta peca ou desligada para contrafactual.\n";
    }
    if (narrativeAttentionActive && narrativeAttention.lastDecision().valid) {
        score << "Atencao narrativa observacional: "
              << narrativeAttention.observedSections() << " secao(oes), "
              << narrativeAttention.focusChangeCount() << " mudanca(s) de foco e "
              << narrativeAttention.eventFocusCount() << " foco(s) em evento; ultimo foco "
              << organismStemName(narrativeAttention.lastDecision().focus)
              << " por "
              << narrativeAttentionReasonName(narrativeAttention.lastDecision().reason)
              << " (valor " << narrativeAttention.lastDecision().value
              << ").\n";
    } else {
        score << "Atencao narrativa observacional: indisponivel sem stems-audit ou desligada para contrafactual.\n";
    }
    if (narrativeMixActive && narrativeMix.lastDirective().valid) {
        score << "Mixagem narrativa limitada: " << narrativeMix.decisionCount()
              << " diretiva(s), " << narrativeMix.hysteresisHoldCount()
              << " retenção(oes), " << narrativeMix.eventDirectiveCount()
              << " evento(s); ultimo foco "
              << organismStemName(narrativeMix.lastDirective().focus)
              << " por "
              << narrativeAttentionReasonName(narrativeMix.lastDirective().reason)
              << " em +" << narrativeMix.lastDirective().gainDb
              << " dB (max +" << narrativeMix.maximumGainDb()
              << " dB). Sem ducking automatico.\n";
    } else if (narrativeAttentionActive) {
        score << "Mixagem narrativa limitada: desligada, sem evidencia suficiente ou sem audibilidade ativa.\n";
    }
    score << "\n"
          << "----------------------------------------------------------------\n"
          << "TRAJETORIA\n"
          << "----------------------------------------------------------------\n"
          << "A peca percorreu " << visitedSections.size() << " secoes, nesta ordem:\n";
    for (size_t i = 0; i < visitedSections.size(); ++i) {
        score << "  " << (i + 1) << ". " << visitedSections[i] << "\n";
    }
    musicalDirection.writeScore(score);
    musicalRelations.writeScore(score);
    bassPhraseTimbre.writeScore(score);
    bassExecutionFeedback.writeScore(score);
    bassKickInteraction.writeScore(score);
    grooveDialogue.writeScore(score);
    selectionTrace.writeScore(score);
    score << "\n----------------------------------------------------------------\n"
          << "ANDAMENTO\n"
          << "----------------------------------------------------------------\n";
    if (tempoTimeline.size() > 1) {
        score << "O andamento mudou " << (tempoTimeline.size() - 1) << " vez(es) ao longo da peca:\n";
        for (const auto &ev : tempoTimeline) {
            score << "  " << formatMinSec(ev.atSeconds) << " -- " << static_cast<int>(ev.bpm + 0.5) << " bpm\n";
        }
    } else {
        score << "Andamento constante em " << cfg.bpm << " bpm, sem mudancas.\n";
    }
    score << "\n----------------------------------------------------------------\n"
          << "AUTONOMIA DE PITCH (telemetria)\n"
          << "----------------------------------------------------------------\n"
          << "Melodia principal: " << melodicPitchChangeCount
          << " mudanca(s), " << melodicRepeatedPitchCount
          << " repeticao(oes) consecutiva(s), maior serie "
          << melodicLongestRepeatedSteps << " passo(s), registro "
          << melodicPitchMin << ".." << melodicPitchMax << " MIDI.\n";
    if (fmFluteEnabled) {
        score << "Flauta FM: " << fmFlutePitchChangeCount
              << " mudanca(s), " << fmFluteRepeatedPitchCount
              << " repeticao(oes) consecutiva(s), maior serie "
              << fmFluteLongestRepeatedNotes << " nota(s), registro "
              << fmFlutePitchMin << ".." << fmFlutePitchMax << " MIDI.\n";
    }
    score << "\n----------------------------------------------------------------\n"
          << "MECANISMOS QUE SE MANIFESTARAM NESTA PECA\n"
          << "----------------------------------------------------------------\n";
    int phasingFullCycles = phasingEnabled
        ? static_cast<int>(std::llabs(static_cast<long long>(phaseStepCounter) - static_cast<long long>(stepCounter)) / std::max(1, sequencer.numSteps()))
        : 0;
    {
        bool anyMechanism = false;
        auto line = [&](bool active, const std::string &text) {
            if (active) { score << "- " << text << "\n"; anyMechanism = true; }
        };
        line(voiceColor != VoiceColor::None,
             std::string("Cor de voz: ") + (voiceColor == VoiceColor::Tape ? "fita" : voiceColor == VoiceColor::Tube ? "valvula" : "fracture folder"));
        line(relationalOrchestra.thereminResponseCount() > 0,
             std::string("Orquestra relacional: bateria, theremin e campo harmonico atravessaram-se por ")
             + relationVerbName(relationalOrchestra.verb()) + "; "
             + std::to_string(relationalOrchestra.thereminResponseCount())
             + " resposta(s), " + std::to_string(relationalOrchestra.resistanceCount())
             + " resistencia(s), " + std::to_string(relationalOrchestra.rhythmFeedbackCount())
             + " consequencia(s) devolvida(s) ao ritmo.");
        line(rasgarGesture.tearCount() > 0,
             std::string("Descartografo: movimento ") + rasgarMotionName(rasgarGesture.motion())
             + " produziu " + std::to_string(rasgarGesture.tearCount())
             + " cismo(s), " + std::to_string(topology.traversalCount())
             + " travessia(s) e " + std::to_string(topologyHarmonicShiftCount)
             + " deslocamento(s) harmonico(s). ");
        line(selfFeedbackEnabled, "Self-feedback: a saida recente da peca realimentou a taxa do mapa de Lorenz.");
        line(freeIntonationEnabled, "Entonacao livre: a altura se desviou da escala guiada pelo caos.");
        line(heisenbergEnabled, "Heisenberg: precisao ritmica e de afinacao acopladas de forma inversa (balanco "
             + std::to_string(heisenbergBalance) + " -- " + (heisenbergBalance < 0.5 ? "ritmo preciso, afinacao solta" : "afinacao presa, ritmo solto") + ").");
        line(runglerEnabled, "Rungler ativo: instabilidade de afinacao ao estilo Benjolin/Hordijk.");
        line(leibnizEnabled, "Leibniz Rectifier: retificacao dinamica + corrupcao de bits.");
        line(tubaVoiceEnabled, "Voz de tuba/grave presente.");
        line(walshOscEnabled, "Oscilador Walsh: sintese aditiva real sobre funcoes ortogonais +-1 (base retangular, nao senoidal).");
        line(vosimEnabled, "VOSIM: sintese de formantes por rajadas de senoide ao quadrado decaindo, sexto principio de sintese do motor.");
        line(hardSyncEnabled, "Hard sync: VCO escravo com fase reiniciada a forca a cada ciclo do VCO mestre, principio de sintese genuinamente distinto dos demais do motor.");
        line(binauralEnabled, "Batimento binaural caotico: canais L/R divergem em frequencia real (~" + std::to_string(binauralOffsetHz) + " Hz de base), acelerando/desacelerando com o caos.");
        line(asphyxiaEnabled, "Envelope de asfixia: " + std::to_string(asphyxiaCount) + " batida(s) de bumbo fecharam o filtro por sidechain, reabrindo devagar.");
        line(polivoksEnabled, "Filtro Polivoks (terceira cor): " + std::to_string(polivoksSelfOscillationCount) + " vez(es) a peca cruzou de verdade pra auto-oscilacao real.");
        line(polivoksVcaEnabled, "Dual VCA Polivoks: par diferencial real (curva tanh) colorindo o estagio de ganho das duas vozes de hocketing.");
        line(acidBasslineEnabled, "Voz de baixo acid (estilo TB-303): filtro Polivoks auto-oscilante + envelope proprio + accent/slide reais -- " + std::to_string(acidBasslineAccentCount) + " accent(s), " + std::to_string(acidBasslineSlideCount) + " slide(s).");
        line(bassBehaviourEnabled, "Bass Behaviour Engine: "
             + std::to_string(bassPlanCount) + " plano(s), "
             + std::to_string(bassEventCount) + " evento(s); corpos tuba/acid/core-sub "
             + std::to_string(bassTubaEventCount) + "/"
             + std::to_string(bassAcidEventCount) + "/"
             + std::to_string(bassCoreSubEventCount) + "; groove aplicado "
             + std::to_string(bassGrooveApplicationCount) + " vez(es); papel final "
             + bassFunctionRoleName(bassFinalRole) + ", corpo final "
             + bassBodyName(bassFinalBody) + ".");
        line(drumGrooveApplicationCount > 0, "Temporal Groove da bateria: "
             + std::to_string(drumGrooveApplicationCount)
             + " aplicação(oes) limitadas, preservando o arquétipo rítmico.");
        line(egLoopPulseEnabled, "Envelope em loop (EG Polivoks): fonte auto-ciclica real, disparando um blip proprio a cada fim-de-attack -- " + std::to_string(egLoopPulseCount) + " pulso(s).");
        line(footnoteFired, "Nota de rodape: gesto isolado de " + std::to_string(footnoteEventCount) + " nota(s), fora da escala da peca, numa posicao metrica fraca.");
        line(galoFired, "Galo: chamado de animal sintetizado (ressonador com chirp real -- silabas subindo, cauda descendo na ultima).");
        line(scatEnabled, "Voz de scat: silabas vocalicas reais (formantes Peterson & Barney), sem significado, sobre o acorde atual -- " + std::to_string(scatEventCount) + " silaba(s).");
        line(choirEnabled, "Coral: quatro vozes reais (soprano/contralto/tenor/baixo) voice-led no acorde atual, conjunto imperfeito real -- " + std::to_string(choirPhraseCount) + " frase(s).");
        line(dejaVuEnabled && dejaVuEventCount > 0, "Deja vu: memoria falsa sintetizada de estatisticas em execucao (pitch/brilho/densidade), nunca uma citacao real -- " + std::to_string(dejaVuEventCount) + " ocorrencia(s).");
        line(fmFluteEnabled, "Flauta FM: sintese FM real de 2 operadores fraseando sobre harmonia propria e independente (" + HarmonicWanderer::movementName(harmonicWanderer.movement()) + "), acompanhamento em " + MelodyVoicingBank::styleName(voicingBank.style()) + ", dissonancia: " + DissonanceTreatment::styleName(dissonanceTreatment.style()) + " -- " + std::to_string(fmFluteNoteCount) + " nota(s).");
        line(vectorSynthEnabled, "Vector Synthesis: crossfade 2D real entre Gendyn/Walsh/AtariPunk/ruido colorido, posicao ditada por dois mapas de caos independentes.");
        line(harmonaigEnabled, "Harmonaig: pad de acorde de 4 vozes reais (raiz/terca/quinta/setima), revozeado " + std::to_string(harmonaigChordCount) + " vez(es) ao longo da peca.");
        line(sievePolyphonyEnabled, "Polifonia de peneiras de Xenakis: 4 canais independentes (madeira/metal/agua/plasma), plasma interceptou consonancia excessiva " + std::to_string(sievePolyphonyPlasmaCount) + " vez(es); trocas de pitch [" + std::to_string(sievePitchChangeCount[0]) + "/" + std::to_string(sievePitchChangeCount[1]) + "/" + std::to_string(sievePitchChangeCount[2]) + "/" + std::to_string(sievePitchChangeCount[3]) + "], repeticoes [" + std::to_string(sieveRepeatedPitchCount[0]) + "/" + std::to_string(sieveRepeatedPitchCount[1]) + "/" + std::to_string(sieveRepeatedPitchCount[2]) + "/" + std::to_string(sieveRepeatedPitchCount[3]) + "]; maior serie [" + std::to_string(sieveLongestRepeatedNotes[0]) + "/" + std::to_string(sieveLongestRepeatedNotes[1]) + "/" + std::to_string(sieveLongestRepeatedNotes[2]) + "/" + std::to_string(sieveLongestRepeatedNotes[3]) + "]; articulacoes de agua " + std::to_string(sieveWaterArticulationChangeCount) + ", densidade " + std::to_string(sieveWaterDensityMin) + ".." + std::to_string(sieveWaterDensityMax) + ", jitter " + std::to_string(sieveWaterPitchJitterMin) + ".." + std::to_string(sieveWaterPitchJitterMax) + ".");
        line(bloomEnabled, "Bloom: melodia fractal (Trunk/Branch/Path), trocou de nivel " + std::to_string(bloomLevelSwitchCount) + " vez(es) ao longo da peca.");
        line(crossDomainContagionEnabled, "Contaminacao cruzada: " + std::to_string(crossDomainContagionCount) + " vez(es) o pico de audio da propria peca cutucou a fase do divisor de clock.");
        line(generatedScaleEnabled, generatedScaleDescription + ", nao vinda do catalogo fixo de 45 escalas.");
        line(chaosStepAddressEnabled, "Endereco de passo caotico: " + std::to_string(chaosStepAddressCount) + " vez(es) o ponteiro de leitura da melodia pulou pra um indice diferente do clock linear.");
        line(sequencerWildTimingEnabled, "Timing selvagem do sequencer: o padrao ritmico/melodico regenerou " + std::to_string(sequencerWildRegenCount) + " vez(es) por cruzamento real de zero de um mapa caotico, nao no relogio fixo de uma vez por compasso.");
        line(atariPunkGlitchEnabled, "Atari Punk Console selvagem: o clock hicupou pra uma divisao/multiplo real " + std::to_string(atariPunkGlitchCount) + " vez(es), o mesmo comportamento real do circuito de dois 555 timers levado ao limite.");
        line(phasingEnabled, "Phasing (Steve Reich): segunda voz lendo o mesmo padrao do sequencer principal " + std::to_string(phasingEventCount) + " vez(es), num relogio " + std::to_string(static_cast<int>(std::round(phasingDelta * 1000.0))) + " promil " + (phasingDelta >= 0.0 ? "mais rapido" : "mais lento") + "; a diferenca de fase completou " + std::to_string(phasingFullCycles) + " volta(s) inteira(s) pelo padrao.");
        {
            std::ostringstream microtonalLine;
            microtonalLine << "Microtonalidade: " << microtonalTuning.description();
            if (microtonalTuning.system() == TuningSystem::JustIntonation5Limit
                || microtonalTuning.system() == TuningSystem::Pythagorean) {
                microtonalLine << ". Deriva real de comma acumulada pelas modulacoes tonais da peca: "
                                << std::fixed << std::setprecision(2) << microtonalTuning.commaDrift() << " cents.";
            } else {
                microtonalLine << ".";
            }
            line(microtonalEnabled, microtonalLine.str());
        }
        line(spectralObjectEnabled, "Objeto espectral: " + spectralObject.description() + ", "
            + std::to_string(spectralObject.admittedCount()) + "/" + std::to_string(SpectralObject::kNumPartials)
            + " parciais soando, " + std::to_string(spectralObject.totalPulseCount())
            + " pulsos ao todo (cada parcial p pulsando p vezes mais rapido que o fundamental -- altura e ritmo lendo a mesma proporcao).");
        line(polymeterEnabled, "Ensemble polimetrico: quatro vozes com " + polymeterEnsemble.description()
            + ", realinharam (todas de volta ao proprio passo 0 juntas) " + std::to_string(polymeterEnsemble.realignmentCount())
            + " vez(es) reais ao longo da peca -- cada voz tem seu proprio ciclo de verdade, nao uma subdivisao do mesmo relogio.");
        line(metricModulationEnabled, "Modulacao metrica real: " + std::to_string(metricModulationEventCount)
            + " troca(s) de andamento via razao real de tercina/quintina (estilo Elliott Carter -- uma subdivisao do pulso antigo vira o novo pulso), nao um salto continuo arbitrario de bpm.");
        {
            std::ostringstream bluffLine;
            bluffLine << "Sinal de blefe: veracidade configurada " << std::fixed << std::setprecision(0) << (bluffSignal.truthfulness() * 100.0)
                       << "%, " << bluffSignal.signalCount() << " sinal(is) disparado(s) (" << bluffSignal.genuineCount()
                       << " genuino(s), " << bluffSignal.bluffCount() << " blefe(s)), confianca final "
                       << std::setprecision(2) << bluffSignal.trust()
                       << (bluffSignal.betrayed() ? " -- confianca foi construida e depois TRAIDA, a voz respondente parou de reagir." : ".");
            line(bluffSignalEnabled, bluffLine.str());
        }
        line(rasgoFractureEnabled, "Rasgo espectral: " + std::to_string(rasgoFracture.totalBreaks()) + "/"
            + std::to_string(RasgoFracture::kNumFibers - 1) + " fibra(s) rasgada(s) (fio sobrevivente: o fundamental nunca rasga), "
            + "modelo de feixe de fibras com local load sharing -- cada ruptura permanente, propagando pra fibras vizinhas.");
        line(coherenceCollapseEnabled && coherenceCollapse.collapseCount() > 0, "Ruptura por excesso de coerencia: " + std::to_string(coherenceCollapse.collapseCount())
            + " colapso(s) real(is) -- entropia de Shannon da melodia recente ficou baixa demais por barras consecutivas, "
            + "disparando um estalo audivel e uma reorganizacao forcada e temporaria do sequencer.");
        line(chaosContagionEnabled, "Contagio do caos: decay do bumbo/caixa jitterizado por batida.");
        line(hocketEnabled && !hocketCascadeEnabled, "Hocketing: " + std::to_string(hocketCount) + " nota(s) pulou/pularam pra vozes extras.");
        line(hocketCascadeEnabled, "Cascata polifonica: " + std::to_string(hocketCount) + " nota(s) empurraram o registro de deslocamento (linhas fantasmas se cruzando, diode-mixadas entre si).");
        line(tonalModulationEnabled, "Modulacao tonal real: " + std::to_string(modulationCount) + " modulacao(oes), raiz final MIDI " + std::to_string(sequencer.rootMidiNote()) + ".");
        line(modalShiftEnabled, "Modulacao modal: " + std::to_string(modalShiftCount) + " troca(s) de modo mantendo a raiz.");
        line(sequencer.chordProgressionEnabled() && !sequencer.neoRiemannianEnabled(), "Progressao de acordes real, um acorde por compasso.");
        line(sequencer.neoRiemannianEnabled(), "Neo-Riemanniano (P/L/R): " + std::to_string(sequencer.neoRiemannianOpCount()) + " transformacao(oes) de triade, triade atual " + sequencer.neoRiemannianDescription() + ".");
        line(sequencer.voiceLeadingEnabled(), "Conducao de voz real: proxima nota escolhida por distancia real em semitons da anterior (maioria passo curto, raro salto dramatico), nao por indice de grau da escala.");
        line(rhythmHumanizeEnabled, "Humanizacao ritmica ativa: " + std::to_string(humanizeApplicationCount) + " aplicacoes; swing aplicado: " + std::to_string(swingApplicationCount) + ".");
        line(tempoDriftEnabled, "Andamento variavel habilitado: " + std::to_string(tempoChangeCount) + " mudanca(s) discreta(s).");
        line(codaRitardandoEnabled, "Ritardando real na coda.");
        line(accelerandoEnabled, "Accelerando/stringendo real rumo ao climax.");
        line(parenthesisEnabled, "Parenteses: " + std::to_string(parenthesisCount)
             + " vez(es), ultima leitura " + parenthesisLastMode
             + "; o audio final foi desviado pra um loop capturado e retornou ao fluxo.");
        line(curtoCircuitoEnabled && curtoCircuitoFired,
             "Curto-circuito: um corte seco real interrompeu a peca, e " + std::to_string(curtoCircuitoGatecrashCount)
                 + " mecanismo(s) que a semente tinha deixado de fora entraram de penetra (" + curtoCircuitoGatecrashedNames + ").");
        line(apocryphal.firedThisPiece(), "Arquivo apocrifo: a peca citou e distorceu um instante ~" + formatMinSec(apocryphal.lastSourceSeconds()) + " de si mesma.");
        line(thereminEnabled,
             relationalOrchestraActive
                ? "Voz de theremin: interprete em glide, com alvo e grau de presenca proprios derivados da historia bateria-harmonia; pode sustentar-se, silenciar ou mudar sem roteiro obrigatorio."
                : "Voz de theremin: sombra continua em glide sobre o pitch da melodia, nunca re-disparada.");
        line(montageCutEnabled && montageCut.firedThisPiece(),
             "Corte de montagem: um corte seco real, limpo e sem processamento, pra um instante ~"
                 + formatMinSec(montageCut.lastSourceSeconds()) + " de si mesma, e de volta.");
        line(atropeloEnabled && atropelo.eventCount() > 0,
             "Atropelo: " + std::to_string(atropelo.eventCount()) + " evento(s), "
                 + std::to_string(atropelo.interruptionCount()) + " interrupcao(oes) seca(s), "
                 + std::to_string(atropelo.resumedEventCount()) + " retomada(s) real(is) da mesma frase, "
                 + std::to_string(atropelo.forgottenCount()) + " esquecida(s) de verdade, "
                 + std::to_string(atropeloSilenceInvasionCount) + " invasao(oes) real(is) do silencio do stochastic mute.");
        line(rosslerEnabled, "Rossler collapse: espiral que erupcionou num chute de filtro.");
        line(thermalFadeEnabled, thermalAbruptDeath ? "Morte termica: corte abrupto no final." : "Morte termica: esmaecimento lento no final.");
        line(stochasticMuteEnabled, "Stochastic mute: silencios cirurgicos raros e reais.");
        line(agonEnabled && agonRusgaCount > 0, "Agon ativo: " + std::to_string(agonRusgaCount) + " rusga(s) disparada(s) entre as quatro vozes.");
        line(estranhoEnabled && estranhoCount > 0, "Elemento estranho: comparador cru apareceu " + std::to_string(estranhoCount) + " vez(es).");
        line(foreignSignalEnabled && foreignReading.available,
             "Sinal estranho literal: " + foreignReading.description + " -- esta peca nao e 100% reprodutivel a partir da seed.");
        line(cfg.wildMode,
             "MODO SELVAGEM: entropia real da maquina reinjetada " + std::to_string(wildReseedCount)
                 + " vez(es) ao longo da peca -- esta peca NAO pode ser reproduzida a partir da seed.");
        line(fixedOverflowEnabled && fixedOverflowCount > 0, "Overflow de ponto fixo: " + std::to_string(fixedOverflowCount) + " wraparound(s) real(is).");
        line(spectralGhostEnabled && spectralGhostCount > 0, "Spectral ghost: " + std::to_string(spectralGhostCount) + " borrao(oes) espectral(is) sustentado(s).");
        line(sampleHoldGlitchEnabled && sampleHoldGlitchCount > 0, "Sample-hold glitch: " + std::to_string(sampleHoldGlitchCount) + " corrupcao(oes) de sample-rate.");
        line(tomRatchetEnabled, "Ratchet nos toms: " + std::to_string(tomRatchetCount) + " roll(s) disparado(s).");
        line(cymbalSustainEnabled, "Prato com sustain estendido: " + std::to_string(cymbalSustainCount) + " batida(s) tocaram mais longo que o normal.");
        line(euclideanCowbellEnabled, "Cowbell segue ritmo euclidiano real: " + euclideanDescription + ".");
        line(!drumsEnabled, "Bateria ausente nesta peca -- escolha real da semente, nao um bug.");
        line(drumsEnabled && hitFailureRate > 0.0, "Falhas reais de bateria: " + std::to_string(drumHitFailureCount) + " batida(s) do padrao nao soaram.");
        line(drumArchetypeEnabled, "Identidade ritmica real: " + currentArchetypeName()
             + " (" + std::to_string(archetypeChangeCount) + " passagem(ns) de feel ao longo da peca).");
        line(ghostHihatCount > 0, "Repeticao com diferenca: " + std::to_string(ghostHihatCount)
             + " ghost hihat(s) real(is) variando o ciclo a cada repeticao de compasso.");
        line(orchestrationChangeCount > 0, "Orquestracao real: " + std::to_string(orchestrationChangeCount)
             + " reconfiguracao(oes) de quais vozes de percussao estao presentes ao longo da peca.");
        line(postGlitchFullKitEvents > 0, "Kit completo forcado apos o Curto-Circuito: " + std::to_string(postGlitchFullKitEvents)
             + " vez(es).");
        line(meterChangeCount > 0, "Mudanca real de compasso: " + std::to_string(meterChangeCount)
             + " troca(s) real(is) de metrica ao longo da peca (compasso final: " + std::to_string(currentBarLength) + " passos).");
        line(intensityGuaranteeTriggered, "Garantia de limite: nenhum dos mecanismos de risco disparou por acaso, um foi forcado.");
        line(selfEpitaphEnabled, "Self-epitaph: um relato em texto puro da propria peca vive no proprio arquivo WAV.");
        if (!anyMechanism) score << "(nenhum mecanismo opcional se manifestou nesta peca especifica -- ainda uma escolha real da semente, nao um erro.)\n";
    }
    score << "\n----------------------------------------------------------------\n"
          << "MERCADO DE PROBABILIDADES (precos usados nesta renderizacao)\n"
          << "----------------------------------------------------------------\n"
          << "Oito mecanismos abaixo nao tem mais uma probabilidade fixa -- cada\n"
          << "um tem um preco que sofre random walk com reversao a media,\n"
          << "persistido entre renders. Esta peca pode nao ser 100% reprodutivel\n"
          << "a partir so da semente se o arquivo de mercado mudar depois.\n";
    for (const auto &entry : market.prices()) {
        score << "  " << entry.first << ": " << std::fixed << std::setprecision(3) << entry.second << "\n";
    }
    score << "\n================================================================\n"
          << "Gerado por Rasgo Synth. \"Se arrisca, e soa.\"\n"
          << "================================================================\n";

    const auto recordingName = cfg.albumId >= 0
        ? "rasgo_integrated_track_" + std::to_string(cfg.albumTrackIndex + 1)
        : std::string("rasgo_integrated");
    std::string path;
    if (cfg.recordOutput) {
        Recorder recorder(cfg.outDir);
        path = recorder.record(recordingName, outSamples, 2, static_cast<int>(sampleRate), meta);
    }
    if (!path.empty()) {
        // WAV and index are already visible; companion publication is still
        // pending. Allow a normal progress/cancellation checkpoint here,
        // without claiming Complete or rolling back the published recording.
        if (onProgress) onProgress(renderProgress);
        publishRecordingScore(path, score.str());
    }

    PieceResult result;
    result.path = path;
    result.samples = std::move(outSamples);
    result.sampleRate = static_cast<int>(sampleRate);
    result.seed = cfg.seed;
    result.tempoTimeline = tempoTimeline;

    // Same values as the JSON block above, as real fields -- see
    // PieceModuleInfo's own comment.
    result.info.materialFocus = materialFocusName(identity.focus);
    result.info.scaleFamily = scaleFamilyName(identity.scaleFamily);
    result.info.coreVoiceModel = coreVoiceModelName(identity.coreVoice);
    result.info.sequenceRhythmModel = sequenceRhythmModelName(identity.rhythmModel);
    result.info.leadVoiceModel = leadVoiceModelName(identity.leadVoice);
    result.info.harmonicLanguage = harmonicLanguageName(identity.harmonicLanguage);
    result.info.ensembleModel = ensembleModelName(identity.ensembleModel);
    result.info.compositionIdentityValue = identity;
    result.info.seedLineage = cfg.seedLineageEnabled;
    result.info.seedLineageParentA = cfg.seedLineageParentA;
    result.info.seedLineageParentB = cfg.seedLineageParentB;
    result.info.seedLineageGeneration = cfg.seedLineageGeneration;
    result.info.seedLineageParentBWeight = cfg.seedLineageParentBWeight;
    result.info.seedLineageMutationProbability = cfg.seedLineageMutationProbability;
    result.info.seedLineageInheritedBMask = cfg.seedLineageInheritedBMask;
    result.info.seedLineageBlendedMask = cfg.seedLineageBlendedMask;
    result.info.seedLineageMutationMask = cfg.seedLineageMutationMask;
    result.info.gameStrategy = gameStrategyName(gamePlan.strategy);
    result.info.gameRule = gameRuleName(gamePlan.rule);
    result.info.gameEnergy = gamePlan.energy;
    result.info.gameMemory = gamePlan.memory;
    result.info.gameRisk = gamePlan.risk;
    result.info.gameFreedom = gamePlan.freedom;
    result.info.gameHiddenInformation = gamePlan.hiddenInformation;
    result.info.rasgarMotion = rasgarMotionName(rasgarGesture.motion());
    result.info.rasgarFinalPhase = rasgarPhaseName(rasgarGesture.phase());
    result.info.rasgarTearCount = rasgarGesture.tearCount();
    result.info.rasgarUnstableBars = rasgarGesture.unstableBars();
    result.info.rasgarMaxInstability = rasgarGesture.maxInstability();
    result.info.rasgarMeanPulseDeviation = rasgarGesture.meanPulseDeviation();
    result.info.topologyRegionCount = static_cast<int>(topology.regions().size());
    result.info.topologyPassageCount = static_cast<int>(topology.passages().size());
    result.info.topologyTraversalCount = topology.traversalCount();
    result.info.topologyRerouteCount = topology.rerouteCount();
    result.info.topologySplitCount = topology.splitCount();
    result.info.topologyArchivePageCount = static_cast<int>(moduleArchive.size());
    result.info.archiveReadCount = static_cast<int>(moduleArchive.count(ArchiveAction::Read));
    result.info.archiveRereadCount = static_cast<int>(moduleArchive.count(ArchiveAction::Reread));
    result.info.archiveReinterpretCount = static_cast<int>(moduleArchive.count(ArchiveAction::Reinterpret));
    result.info.archiveRecreateCount = static_cast<int>(moduleArchive.count(ArchiveAction::Recreate));
    result.info.topologyHarmonicShiftCount = topologyHarmonicShiftCount;
    result.info.relationalOrchestra = relationalOrchestraActive;
    result.info.relationalVerb = relationVerbName(relationalOrchestra.verb());
    result.info.relationalRhythmObservationCount = relationalOrchestra.rhythmObservationCount();
    result.info.relationalThereminResponseCount = relationalOrchestra.thereminResponseCount();
    result.info.relationalHarmonyResponseCount = relationalOrchestra.harmonyResponseCount();
    result.info.relationalHarmonyReconfigurationCount = relationalAppliedHarmonyShiftCount;
    result.info.relationalRhythmFeedbackCount = relationalOrchestra.rhythmFeedbackCount();
    result.info.relationalResistanceCount = relationalOrchestra.resistanceCount();
    result.info.relationalChamber = relationalChamberActive;
    result.info.relationalChamberAuditReady = relationalChamberAuditReady;
    result.info.relationalChamberRelationCount =
        static_cast<int>(relationalChamber.relationCount());
    result.info.relationalChamberConsequenceCount = relationalChamberConsequenceCount;
    result.info.relationalChamberDecisionCount =
        static_cast<int>(relationalChamber.decisionCount());
    result.info.relationalChamberCiteCount = relationalChamberCiteCount;
    result.info.relationalChamberResistCount = relationalChamberResistCount;
    result.info.relationalChamberDeformCount = relationalChamberDeformCount;
    result.info.relationalChamberScarCount = relationalChamberScarCount;
    result.info.relationalChamberSpatialCrossingCount =
        relationalChamberSpatialCrossingCount;
    result.info.relationalChamberLastManifestedBodies =
        relationalChamberLastManifestedBodies;
    result.info.relationalChamberFluteGain = relationalChamberResponse.fluteGain;
    result.info.relationalChamberTubaPressure = relationalChamberResponse.tubaPressure;
    result.info.relationalChamberVoiceGain = relationalChamberResponse.voiceGain;
    result.info.relationalChamberSpatialPolarity = relationalChamberResponse.spatialPolarity;
    result.info.relationalChamberResponseValid = relationalChamberResponse.valid();
    result.info.relationalChamberLastAction = relationalChamberLastAction;
    result.info.narrativeAttention = narrativeAttentionActive;
    result.info.narrativeAttentionObservedSectionCount =
        narrativeAttention.observedSections();
    result.info.narrativeAttentionFocusChangeCount =
        narrativeAttention.focusChangeCount();
    result.info.narrativeAttentionEventFocusCount =
        narrativeAttention.eventFocusCount();
    result.info.narrativeAttentionManifestedBodies =
        narrativeAttention.lastDecision().manifestedBodies;
    result.info.narrativeAttentionLastValue =
        narrativeAttention.lastDecision().value;
    result.info.narrativeAttentionMaximumValue = narrativeAttention.maximumValue();
    result.info.narrativeAttentionFinalFocus = narrativeAttention.lastDecision().valid
        ? organismStemName(narrativeAttention.lastDecision().focus) : "none";
    result.info.narrativeAttentionFinalReason =
        narrativeAttentionReasonName(narrativeAttention.lastDecision().reason);
    result.info.narrativeMix = narrativeMixActive;
    result.info.narrativeMixDirectiveCount = narrativeMix.decisionCount();
    result.info.narrativeMixHysteresisHoldCount = narrativeMix.hysteresisHoldCount();
    result.info.narrativeMixEventDirectiveCount = narrativeMix.eventDirectiveCount();
    result.info.narrativeMixLastGainDb = narrativeMix.lastDirective().gainDb;
    result.info.narrativeMixMaximumGainDb = narrativeMix.maximumGainDb();
    result.info.narrativeMixFinalFocus = narrativeMix.lastDirective().valid
        ? organismStemName(narrativeMix.lastDirective().focus) : "none";
    result.info.narrativeMixFinalReason =
        narrativeAttentionReasonName(narrativeMix.lastDirective().reason);
    result.info.expressiveEventSummary = expressiveSummary;
    result.info.expressiveActuation = cfg.expressiveActuationEnabled;
    result.info.expressiveActuationEventCount = expressiveActuationEventCount;
    result.info.performanceActorCount = static_cast<int>(performanceActors.size());
    result.info.performanceHitActorCount = static_cast<int>(
        performanceActors.count(PerformanceGestureFamily::Hit));
    result.info.performanceTextureActorCount = static_cast<int>(
        performanceActors.count(PerformanceGestureFamily::Texture));
    result.info.performanceToneActorCount = static_cast<int>(
        performanceActors.count(PerformanceGestureFamily::Tone));
    for (std::size_t family = 0;
         family < result.info.performanceActorFamilyCounts.size(); ++family) {
        result.info.performanceActorFamilyCounts[family] = static_cast<int>(
            performanceActors.count(static_cast<PerformanceGestureFamily>(family)));
    }
    result.info.performanceTerritoryManifest = makeRasgoTerritoryManifest(
        static_cast<std::uint32_t>(cfg.seed), cfg.bpm, performanceActors, gamePlan);
    result.info.relationalThereminPresentBarCount = relationalOrchestra.thereminPresentBarCount();
    result.info.relationalThereminSilentBarCount = relationalOrchestra.thereminSilentBarCount();
    result.info.relationalThereminEntryCount = relationalOrchestra.thereminEntryCount();
    result.info.relationalThereminWithdrawalCount = relationalOrchestra.thereminWithdrawalCount();
    result.info.relationalMemory = relationalOrchestra.memory();
    result.info.relationalResistance = relationalOrchestra.resistance();
    result.info.relationalThereminPresence = relationalOrchestra.currentThereminPresence();
    result.info.relationalThereminTargetMidi = relationalOrchestra.currentThereminTargetMidi();
    result.info.relationalThereminPitchChangeCount = relationalThereminPitchChangeCount;
    result.info.relationalThereminRepeatedPitchCount = relationalThereminRepeatedPitchCount;
    result.info.relationalThereminLongestRepeatedBars = relationalThereminLongestRepeatedBars;
    result.info.relationalThereminPitchMin = relationalThereminPitchMin;
    result.info.relationalThereminPitchMax = relationalThereminPitchMax;
    result.info.organismStemAudit = organismStemAudit.enabled();
    result.info.organismStemMaxReconstructionError =
        organismStemAudit.maxPostReconstructionError();
    result.info.organismAudibility = organismAudibility.enabled();
    result.info.organismAudibilityMovementCount = organismAudibility.movementCount();
    result.info.organismAudibilityEmergenceCount = organismAudibility.emergenceCount();
    result.info.organismAudibilityYieldCount = organismAudibility.yieldCount();
    result.info.organismSpectralRoleResponseCount =
        organismAudibility.spectralRoleResponseCount();
    for (int stem = 0; stem < static_cast<int>(OrganismStem::Count); ++stem) {
        const auto organism = static_cast<OrganismStem>(stem);
        result.info.organismStemMetrics[static_cast<std::size_t>(stem)] =
            organismStemAudit.metrics(organism);
        ModuleLifeRecord life;
        life.mark(ModuleLifeState::Available);
        const auto &metrics = result.info.organismStemMetrics[static_cast<std::size_t>(stem)];
        const bool manifested = metrics.preSpaceRms > 1.0e-9
            || metrics.postSpaceRms > 1.0e-9;
        const bool audible = metrics.postSpaceRms > 1.0e-9
            && metrics.postAudibleRatio > 0.004;
        if (manifested) {
            life.mark(ModuleLifeState::Selected);
            life.mark(ModuleLifeState::Manifested);
        }
        if (audible) {
            life.mark(ModuleLifeState::Audible);
        }
        result.info.organismLifeMasks[static_cast<std::size_t>(stem)] =
            ModuleLifeRecord::fromMask(life.mask()).mask();
        result.info.organismAudibilityMinGain[static_cast<std::size_t>(stem)] =
            organismAudibility.minimumGain(organism);
        result.info.organismAudibilityMaxGain[static_cast<std::size_t>(stem)] =
            organismAudibility.maximumGain(organism);
        result.info.orchestralProfiles[static_cast<std::size_t>(stem)] =
            baseOrchestralProfile(organism, identity);
    }
    result.info.agencyInitialForeground = agencyInitialForeground;
    result.info.agencyFinalForeground = materialAgency.foregroundLabel();
    result.info.agencyForegroundChangeCount = materialAgency.foregroundChangeCount();
    result.info.agencyDispositionChangeCount = materialAgency.dispositionChangeCount();
    result.info.agencyLeaderlessSectionCount = materialAgency.leaderlessSectionCount();
    result.info.agencyPluralSectionCount = materialAgency.pluralSectionCount();
    result.info.agencyObservedSectionCount = materialAgency.observedSectionCount();
    result.info.agencyRelationalReactionCount = materialAgency.relationalReactionCount();
    for (int family = 0; family < static_cast<int>(AgencyFamily::Count); ++family) {
        const auto agencyFamily = static_cast<AgencyFamily>(family);
        result.info.agencyRms[static_cast<std::size_t>(family)] = agencyMeter.rms(agencyFamily);
        result.info.agencyPeak[static_cast<std::size_t>(family)] = agencyMeter.peak(agencyFamily);
        result.info.agencyAudibleRatio[static_cast<std::size_t>(family)] = agencyMeter.audibleRatio(agencyFamily);
    }
    result.info.initialRootMidiNote = pieceRootMidiNote;
    result.info.spectralTechniqueCount = identity.spectral.count();
    result.info.inputTruePeakDbtp = linearToDb(limiter.inputTruePeak());
    result.info.preDeliveryTruePeakDbtp = linearToDb(preDeliveryTruePeak);
    result.info.outputTruePeakDbtp = linearToDb(finalTruePeak);
    result.info.deliveryTruePeakTrimDb = deliveryTrimDb;
    result.info.preDeliveryGuardDbtp = linearToDb(delivery.preGuardPeak);
    result.info.deliveryGuardProjectedDbtp = linearToDb(delivery.projectedGuardPeak);
    result.info.limiterMaxGainReductionDb = limiter.maxGainReductionDb();
    result.info.overloadMaxGainReductionDb = overloadProtector.maxReductionDb();
    result.info.overloadFrameCount = overloadProtector.overloadFrames();
    result.info.outputNonFiniteCount = static_cast<int>(limiter.nonFiniteCount());
    result.info.narrativeTemplate = arc.templateDescription();
    result.info.scale = sequencer.scaleDescription();
    result.info.initialScaleValue = initialScale;
    result.info.voiceColor = voiceColor == VoiceColor::Tape ? "tape" : voiceColor == VoiceColor::Tube ? "tube" : voiceColor == VoiceColor::Fracture ? "fracture" : "none";
    result.info.loFiDelay = loFiDelay;
    result.info.staticPop = staticPopEnabled;
    result.info.granularTexture = granularTextureEnabled;
    result.info.gendynTexture = gendynTextureEnabled;
    result.info.coloredNoiseBed = coloredNoiseBedEnabled;
    result.info.noiseClimateMin = noiseClimateMin;
    result.info.noiseClimateMax = noiseClimateMax;
    result.info.noiseClimateMean = noiseClimateSamples > 0
        ? static_cast<float>(noiseClimateSum / static_cast<double>(noiseClimateSamples))
        : 0.0f;
    result.info.noiseColor = noiseColorName(activeNoiseColor);
    result.info.noiseColorChangeCount = noiseColorChangeCount;
    result.info.snareNoiseColor = snareNoiseColorName;
    result.info.clapNoiseColor = clapNoiseColorName;
    result.info.maracasNoiseColor = maracasNoiseColorName;
    result.info.selfFeedback = selfFeedbackEnabled;
    result.info.crossFmDepth = crossFmDepth;
    result.info.sequenceLockAmount = sequencer.lockAmount();
    result.info.sequenceJumpiness = sequencer.jumpiness();
    result.info.melodicPitchChangeCount = melodicPitchChangeCount;
    result.info.melodicRepeatedPitchCount = melodicRepeatedPitchCount;
    result.info.melodicLongestRepeatedSteps = melodicLongestRepeatedSteps;
    result.info.melodicPitchMin = melodicPitchMin;
    result.info.melodicPitchMax = melodicPitchMax;
    result.info.fmFlutePitchChangeCount = fmFlutePitchChangeCount;
    result.info.fmFluteRepeatedPitchCount = fmFluteRepeatedPitchCount;
    result.info.fmFluteLongestRepeatedNotes = fmFluteLongestRepeatedNotes;
    result.info.fmFlutePitchMin = fmFlutePitchMin;
    result.info.fmFlutePitchMax = fmFlutePitchMax;
    result.info.melodicContourDirection = sequencer.melodicContourDirection();
    result.info.melodicContourApexBar = sequencer.melodicContourApexBar();
    result.info.ochdBreathing = ochdEnabled;
    result.info.hihatBranchMode = hihatBranch.mode() == BernoulliGate::Mode::Toggle ? "toggle" : "bernoulli";
    result.info.hihatOpenProbability = hihatBranch.probability();
    result.info.drumsPresence = drumsPresence;
    result.info.drumsPresenceFloorApplied = drumsPresenceFloorApplied;
    result.info.drumsProtagonist = rhythmicProtagonist;
    result.info.drumsPeak = drumsPeak;
    result.info.drumsRms = drumsMeasuredFrames > 0
        ? std::sqrt(drumsEnergySum / static_cast<double>(drumsMeasuredFrames)) : 0.0;
    result.info.drumsAudibleFrameRatio = drumsMeasuredFrames > 0
        ? static_cast<double>(drumsAudibleFrames) / static_cast<double>(drumsMeasuredFrames) : 0.0;
    result.info.drumsEnabled = drumsEnabled;
    result.info.drumHitFailureCount = drumHitFailureCount;
    result.info.drumArchetype = drumArchetypeEnabled;
    result.info.drumArchetypeName = drumArchetypeEnabled ? currentArchetypeName() : "";
    result.info.rhythmSieveUsed = drumArchetypeEnabled && currentArchetypeIndex == rhythmSieveSlotIndex;
    result.info.rhythmGenAksakUsed = drumArchetypeEnabled && currentArchetypeIndex == rhythmAksakSlotIndex;
    result.info.rhythmGenEuclidUsed = drumArchetypeEnabled && currentArchetypeIndex == rhythmEuclidSlotIndex;
    result.info.ghostHihatCount = ghostHihatCount;
    result.info.percussionTimbreVariation = percussionTimbreVariationEnabled;
    result.info.percussionTimbreDepth = percussionTimbreDepth;
    result.info.snareWhiteVariationCompensation = snareWhiteVariationCompensation;
    result.info.snareColorBalanceCompensation = snareColorBalanceCompensation;
    result.info.orchestrationChangeCount = orchestrationChangeCount;
    result.info.orchestralMemorySectionCount = orchestralMemorySectionCount;
    result.info.orchestralHighConflictCount = orchestralHighConflictCount;
    result.info.orchestralRepeatedLeaderCount = orchestralRepeatedLeaderCount;
    result.info.orchestralRoleResponseCount = orchestralRoleResponseCount;
    result.info.orchestralLastLeader = organismStemName(orchestralLastLeader);
    result.info.postGlitchFullKitEvents = postGlitchFullKitEvents;
    result.info.meterVariance = meterVarianceEnabled;
    result.info.meterChangeCount = meterChangeCount;
    result.info.finalBarLength = currentBarLength;
    result.info.drumArchetypeChangeCount = archetypeChangeCount;
    result.info.apocryphalTear = apocryphal.firedThisPiece();
    result.info.apocryphalSourceSeconds = apocryphal.lastSourceSeconds();
    result.info.thereminVoice = thereminEnabled;
    result.info.montageCut = montageCutEnabled && montageCut.firedThisPiece();
    result.info.montageCutSourceSeconds = montageCut.lastSourceSeconds();
    result.info.atropelo = atropeloEnabled && atropelo.eventCount() > 0;
    result.info.atropeloEventCount = atropelo.eventCount();
    result.info.atropeloInterruptionCount = atropelo.interruptionCount();
    result.info.atropeloResumedCount = atropelo.resumedEventCount();
    result.info.atropeloForgottenCount = atropelo.forgottenCount();
    result.info.atropeloSilenceInvasionCount = atropeloSilenceInvasionCount;
    result.info.atropeloNestingCount = atropeloNestingCount;
    result.info.tempoDrift = tempoDriftEnabled;
    result.info.tempoDriftDepth = tempoDriftDepth;
    result.info.tempoChangeCount = tempoChangeCount;
    result.info.codaRitardando = codaRitardandoEnabled;
    result.info.accelerando = accelerandoEnabled;
    result.info.parenthesis = parenthesisEnabled;
    result.info.parenthesisCount = parenthesisCount;
    result.info.parenthesisLastMode = parenthesisLastMode;
    result.info.parenthesisLastDurationSeconds = parenthesisLastDurationSeconds;
    result.info.parenthesisLastLoopSeconds = parenthesisLastLoopSeconds;
    result.info.curtoCircuito = curtoCircuitoEnabled && curtoCircuitoFired;
    result.info.curtoCircuitoGatecrashCount = curtoCircuitoGatecrashCount;
    result.info.agon = agonEnabled;
    result.info.agonRusgaCount = agonRusgaCount;
    result.info.estranho = estranhoEnabled;
    result.info.estranhoCount = estranhoCount;
    result.info.foreignSignalUsed = foreignSignalEnabled && foreignReading.available;
    result.info.foreignSignalDescription = foreignReading.available ? foreignReading.description : "";
    result.info.wildMode = cfg.wildMode;
    result.info.wildReseedCount = wildReseedCount;
    result.info.fixedOverflow = fixedOverflowEnabled;
    result.info.fixedOverflowCount = fixedOverflowCount;
    result.info.spectralGhost = spectralGhostEnabled;
    result.info.spectralGhostCount = spectralGhostCount;
    result.info.sampleHoldGlitch = sampleHoldGlitchEnabled;
    result.info.sampleHoldGlitchCount = sampleHoldGlitchCount;
    result.info.tomRatchet = tomRatchetEnabled;
    result.info.tomRatchetCount = tomRatchetCount;
    result.info.cymbalSustain = cymbalSustainEnabled;
    result.info.cymbalSustainCount = cymbalSustainCount;
    result.info.euclideanCowbell = euclideanCowbellEnabled;
    result.info.euclideanCowbellPattern = euclideanDescription;
    result.info.harmonicWeight = sequencer.harmonicWeightEnabled();
    result.info.strongBeatPull = sequencer.strongBeatPull();
    result.info.tonalModulation = tonalModulationEnabled;
    result.info.modulationCount = modulationCount;
    result.info.finalRootMidiNote = sequencer.rootMidiNote();
    result.info.modalShift = modalShiftEnabled;
    result.info.modalShiftCount = modalShiftCount;
    result.info.chordProgression = sequencer.chordProgressionEnabled();
    result.info.neoRiemannian = sequencer.neoRiemannianEnabled();
    result.info.neoRiemannianOpCount = sequencer.neoRiemannianOpCount();
    result.info.voiceLeading = sequencer.voiceLeadingEnabled();
    result.info.rhythmHumanize = rhythmHumanizeEnabled;
    result.info.humanizeMaxSamples = humanizeMaxSamples;
    result.info.humanizeApplicationCount = humanizeApplicationCount;
    result.info.swingApplicationCount = swingApplicationCount;
    result.info.percFxColoredCount = percFxColoredCount;
    result.info.percFxCharacterVcaCount = percFxCharacterVcaCount;
    result.info.hocketing = hocketEnabled;
    result.info.hocketCount = hocketCount;
    result.info.hocketCascade = hocketCascadeEnabled;
    result.info.hocketInfluenceLinks = static_cast<int>(hocket1ToOsc1) + static_cast<int>(hocket1ToOsc2)
                                      + static_cast<int>(hocket2ToOsc1) + static_cast<int>(hocket2ToOsc2);
    result.info.freeIntonation = freeIntonationEnabled;
    result.info.freeIntonationDepth = freeIntonationDepth;
    result.info.heisenberg = heisenbergEnabled;
    result.info.heisenbergBalance = heisenbergBalance;
    result.info.rungler = runglerEnabled;
    result.info.fractureFold = (voiceColor == VoiceColor::Fracture);
    result.info.leibnizRectifier = leibnizEnabled;
    result.info.tubaVoice = tubaVoiceEnabled;
    result.info.chaosContagion = chaosContagionEnabled;
    result.info.thermalFade = thermalFadeEnabled;
    result.info.thermalAbruptDeath = thermalAbruptDeath;
    result.info.rosslerCollapse = rosslerEnabled;
    result.info.selfEpitaph = selfEpitaphEnabled;
    result.info.stochasticMute = stochasticMuteEnabled;
    result.info.walshOscillator = walshOscEnabled;
    result.info.vosim = vosimEnabled;
    result.info.hardSync = hardSyncEnabled;
    result.info.binaural = binauralEnabled;
    result.info.binauralOffsetHz = binauralOffsetHz;
    result.info.asphyxia = asphyxiaEnabled;
    result.info.asphyxiaCount = asphyxiaCount;
    result.info.polivoks = polivoksEnabled;
    result.info.polivoksVca = polivoksVcaEnabled;
    result.info.acidBassline = acidBasslineEnabled;
    result.info.acidBasslineAccentCount = acidBasslineAccentCount;
    result.info.acidBasslineSlideCount = acidBasslineSlideCount;
    result.info.bassBehaviour = bassBehaviourEnabled;
    result.info.bassPlanCount = bassPlanCount;
    result.info.bassEventCount = bassEventCount;
    result.info.bassTubaEventCount = bassTubaEventCount;
    result.info.bassAcidEventCount = bassAcidEventCount;
    result.info.bassCoreSubEventCount = bassCoreSubEventCount;
    result.info.bassExecutionFeedback = bassExecutionFeedback.enabled();
    result.info.bassFeedbackUpdates = bassExecutionFeedback.updates();
    result.info.bassObservedTriggers = bassExecutionFeedback.triggers();
    result.info.bassObservedActiveFrames = bassExecutionFeedback.activeFrames();
    result.info.bassKickInteraction = bassKickInteraction.enabled();
    result.info.bassKickInteractionTriggers = bassKickInteraction.triggers();
    result.info.bassKickReducedFrames = bassKickInteraction.reducedFrames();
    result.info.bassPhraseTimbre = bassPhraseTimbre.enabled();
    result.info.bassPhraseTimbreNotes = bassPhraseTimbre.notes();
    result.info.bassPhraseTimbreFrames = bassPhraseTimbre.frames();
    result.info.grooveDialogue = grooveDialogue.enabled();
    result.info.grooveDialogueBars = grooveDialogue.bars();
    result.info.grooveDialogueHitPlans = grooveDialogue.hits();
    result.info.grooveDialogueBassImprints = grooveDialogue.bassImprints();
    result.info.grooveSubdivisions = grooveDialogue.subdivisionsEnabled();
    result.info.grooveSubdivisionExtraPlans = grooveDialogue.subdivisionPlans();
    result.info.grooveSubdivisionExtraTriggers = grooveDialogue.subdivisionTriggers();
    result.info.bassGrooveApplicationCount = bassGrooveApplicationCount;
    result.info.drumGrooveApplicationCount = drumGrooveApplicationCount;
    result.info.bassEarlyTimingClampCount = bassEarlyTimingClampCount;
    result.info.bassFinalRole = bassFunctionRoleName(bassFinalRole);
    result.info.bassFinalBody = bassBodyName(bassFinalBody);
    result.info.egLoopPulse = egLoopPulseEnabled;
    result.info.egLoopPulseCount = egLoopPulseCount;
    result.info.footnoteGesture = footnoteFired;
    result.info.footnoteEventCount = footnoteEventCount;
    result.info.galoCall = galoFired;
    result.info.scatVoice = scatEnabled;
    result.info.scatEventCount = scatEventCount;
    result.info.choir = choirEnabled;
    result.info.choirPhraseCount = choirPhraseCount;
    result.info.dejaVu = dejaVuEnabled;
    result.info.dejaVuEventCount = dejaVuEventCount;
    result.info.apocryphalReading = selfEpitaphEnabled && !apocryphalNearMisses.empty();
    result.info.polivoksSelfOscillationCount = polivoksSelfOscillationCount;
    result.info.fmFlute = fmFluteEnabled;
    result.info.fmFluteNoteCount = fmFluteNoteCount;
    result.info.fmFluteHarmonicMovement = fmFluteEnabled ? HarmonicWanderer::movementName(harmonicWanderer.movement()) : "";
    result.info.fmFluteVoicingStyle = fmFluteEnabled ? MelodyVoicingBank::styleName(voicingBank.style()) : "";
    result.info.fmFluteDissonanceStyle = fmFluteEnabled ? DissonanceTreatment::styleName(dissonanceTreatment.style()) : "";
    result.info.vectorSynth = vectorSynthEnabled;
    result.info.harmonaig = harmonaigEnabled;
    result.info.harmonaigChordCount = harmonaigChordCount;
    result.info.sievePolyphony = sievePolyphonyEnabled;
    result.info.sievePolyphonyPlasmaCount = sievePolyphonyPlasmaCount;
    result.info.sievePitchChangeCount = sievePitchChangeCount;
    result.info.sieveRepeatedPitchCount = sieveRepeatedPitchCount;
    result.info.sieveLongestRepeatedNotes = sieveLongestRepeatedNotes;
    result.info.sievePitchMin = sievePitchMin;
    result.info.sievePitchMax = sievePitchMax;
    result.info.sieveWaterArticulationChangeCount = sieveWaterArticulationChangeCount;
    result.info.sieveWaterDensityMin = sieveWaterDensityMin;
    result.info.sieveWaterDensityMax = sieveWaterDensityMax;
    result.info.sieveWaterGrainDurationMin = sieveWaterGrainDurationMin;
    result.info.sieveWaterGrainDurationMax = sieveWaterGrainDurationMax;
    result.info.sieveWaterPitchJitterMin = sieveWaterPitchJitterMin;
    result.info.sieveWaterPitchJitterMax = sieveWaterPitchJitterMax;
    result.info.bloom = bloomEnabled;
    result.info.bloomLevelSwitchCount = bloomLevelSwitchCount;
    result.info.gameWeightAlea = gameWeightAlea;
    result.info.gameWeightAgon = gameWeightAgon;
    result.info.gameWeightIlinx = gameWeightIlinx;
    result.info.gameWeightMimicry = gameWeightMimicry;
    result.info.crossDomainContagion = crossDomainContagionEnabled;
    result.info.crossDomainContagionCount = crossDomainContagionCount;
    result.info.generatedScale = generatedScaleEnabled;
    result.info.chaosStepAddress = chaosStepAddressEnabled;
    result.info.sequencerWildTiming = sequencerWildTimingEnabled;
    result.info.sequencerWildRegenCount = sequencerWildRegenCount;
    result.info.atariPunkGlitch = atariPunkGlitchEnabled;
    result.info.atariPunkGlitchCount = atariPunkGlitchCount;
    result.info.chaosStepAddressCount = chaosStepAddressCount;
    result.info.intensityGuaranteeTriggered = intensityGuaranteeTriggered;
    result.info.phasing = phasingEnabled;
    result.info.phasingEventCount = phasingEventCount;
    result.info.phasingFullCycles = phasingFullCycles;
    result.info.microtonal = microtonalEnabled;
    if (microtonalEnabled) {
        result.info.microtonalSystem = microtonalTuning.description();
        result.info.microtonalCommaDriftCents = microtonalTuning.commaDrift();
    }
    result.info.spectralObject = spectralObjectEnabled;
    if (spectralObjectEnabled) {
        result.info.spectralObjectDescription = spectralObject.description();
        result.info.spectralObjectAdmittedPartials = spectralObject.admittedCount();
        result.info.spectralObjectPulseCount = spectralObject.totalPulseCount();
    }
    result.info.polymeter = polymeterEnabled;
    if (polymeterEnabled) {
        result.info.polymeterDescription = polymeterEnsemble.description();
        result.info.polymeterRealignmentCount = polymeterEnsemble.realignmentCount();
    }
    result.info.metricModulation = metricModulationEnabled;
    result.info.metricModulationEventCount = metricModulationEventCount;
    result.info.bluffSignal = bluffSignalEnabled;
    if (bluffSignalEnabled) {
        result.info.bluffTruthfulness = bluffSignal.truthfulness();
        result.info.bluffSignalCount = bluffSignal.signalCount();
        result.info.bluffGenuineCount = bluffSignal.genuineCount();
        result.info.bluffBluffCount = bluffSignal.bluffCount();
        result.info.bluffFinalTrust = bluffSignal.trust();
        result.info.bluffBetrayed = bluffSignal.betrayed();
    }
    result.info.rasgoFracture = rasgoFractureEnabled;
    if (rasgoFractureEnabled) {
        result.info.rasgoFractureBreaks = rasgoFracture.totalBreaks();
        result.info.rasgoFractureShapeK = rasgoFracture.shapeK();
    }
    result.info.coherenceCollapse = coherenceCollapseEnabled;
    if (coherenceCollapseEnabled) {
        result.info.coherenceCollapseCount = coherenceCollapse.collapseCount();
    }
    result.info.observableModuleActiveCount = observableModuleActiveCount(result.info);
    result.info.observableModuleCapacity = observableModuleCapacity();

    // Settle the market one step and persist it -- after every metadata
    // field above has already captured the prices actually USED this
    // render, so next time's read reflects this piece's own contribution.
    market.settle(rng);
    result.nextProbabilityMarket = market.continuation();
    if (cfg.recordOutput && !path.empty()) {
        try { market.save(marketPath); }
        catch (const std::runtime_error& error) {
            throw std::runtime_error(std::string(error.what())
                + "; completed WAV/index/score preserved; WAV: " + path
                + "; recovery folder: " + cfg.outDir);
        }
    }

    if (onProgress) {
        renderProgress.phase = PieceRenderPhase::Complete;
        onProgress(renderProgress);
    }
    return result;
}

} // namespace rasgo
